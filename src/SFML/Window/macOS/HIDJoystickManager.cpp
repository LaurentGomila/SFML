////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2025 Marco Antognini (antognini.marco@gmail.com),
//                         Laurent Gomila (laurent@sfml-dev.org)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Window/macOS/HIDInputManager.hpp>
#include <SFML/Window/macOS/HIDJoystickManager.hpp>

#include <array>

////////////////////////////////////////////////////////////
// Private data
////////////////////////////////////////////////////////////
namespace
{
// Using a custom run loop mode solve some issues that appears when SFML
// is used with Cocoa.
const CFStringRef runLoopMode = CFSTR("SFML_RUN_LOOP_MODE");
} // namespace


namespace sf::priv
{
////////////////////////////////////////////////////////////
HIDJoystickManager& HIDJoystickManager::getInstance()
{
    static HIDJoystickManager manager;
    return manager;
}


////////////////////////////////////////////////////////////
unsigned int HIDJoystickManager::getJoystickCount()
{
    update();
    return m_joystickCount;
}


////////////////////////////////////////////////////////////
CFPtr<CFSetRef> HIDJoystickManager::copyJoysticks()
{
    return CFPtr<CFSetRef>(IOHIDManagerCopyDevices(m_manager));
}


////////////////////////////////////////////////////////////
HIDJoystickManager::HIDJoystickManager()
{
    m_manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);

    const auto mask0 = HIDInputManager::copyDevicesMask(kHIDPage_GenericDesktop, kHIDUsage_GD_Joystick);

    const auto mask1 = HIDInputManager::copyDevicesMask(kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad);

    std::array maskArray = {mask0.get(), mask1.get()};
    const auto mask      = CFPtr<CFArrayRef>(
        CFArrayCreate(nullptr, reinterpret_cast<const void**>(maskArray.data()), maskArray.size(), nullptr));

    IOHIDManagerSetDeviceMatchingMultiple(m_manager, mask.get());


    IOHIDManagerRegisterDeviceMatchingCallback(m_manager, pluggedIn, this);
    IOHIDManagerRegisterDeviceRemovalCallback(m_manager, pluggedOut, this);

    IOHIDManagerScheduleWithRunLoop(m_manager, CFRunLoopGetCurrent(), runLoopMode);

    IOHIDManagerOpen(m_manager, kIOHIDOptionsTypeNone);
}


////////////////////////////////////////////////////////////
HIDJoystickManager::~HIDJoystickManager()
{
    IOHIDManagerUnscheduleFromRunLoop(m_manager, CFRunLoopGetCurrent(), runLoopMode);

    IOHIDManagerRegisterDeviceMatchingCallback(m_manager, nullptr, nullptr);
    IOHIDManagerRegisterDeviceRemovalCallback(m_manager, nullptr, nullptr);

    IOHIDManagerClose(m_manager, kIOHIDOptionsTypeNone);
}


////////////////////////////////////////////////////////////
void HIDJoystickManager::update()
{
    SInt32 status = kCFRunLoopRunHandledSource;

    while (status == kCFRunLoopRunHandledSource)
    {
        status = CFRunLoopRunInMode(runLoopMode, 0, true);
    }
}


////////////////////////////////////////////////////////////
void HIDJoystickManager::pluggedIn(void* context, IOReturn, void*, IOHIDDeviceRef)
{
    auto* manager = static_cast<HIDJoystickManager*>(context);
    ++manager->m_joystickCount;
}


////////////////////////////////////////////////////////////
void HIDJoystickManager::pluggedOut(void* context, IOReturn, void*, IOHIDDeviceRef)
{
    auto* manager = static_cast<HIDJoystickManager*>(context);
    --manager->m_joystickCount;
}


} // namespace sf::priv
