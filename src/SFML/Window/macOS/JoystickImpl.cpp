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
#include <SFML/Window/JoystickImpl.hpp>
#include <SFML/Window/macOS/AutoreleasePoolWrapper.hpp>
#include <SFML/Window/macOS/HIDInputManager.hpp>
#include <SFML/Window/macOS/HIDJoystickManager.hpp>

#include <SFML/System/Err.hpp>

#include <algorithm>
#include <ostream>


namespace
{

// Convert a CFStringRef to std::string
std::string stringFromCFString(CFStringRef cfString)
{
    const CFIndex     length = CFStringGetLength(cfString);
    std::vector<char> str(static_cast<std::size_t>(length));
    const CFIndex     maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);
    CFStringGetCString(cfString, str.data(), maxSize, kCFStringEncodingUTF8);
    return str.data();
}

// Get HID device property key as a string
std::string getDeviceString(IOHIDDeviceRef ref, CFStringRef prop, unsigned int index)
{
    CFTypeRef typeRef = IOHIDDeviceGetProperty(ref, prop);
    if (typeRef && (CFGetTypeID(typeRef) == CFStringGetTypeID()))
    {
        const auto* str = static_cast<CFStringRef>(typeRef);
        return stringFromCFString(str);
    }

    sf::err() << "Unable to read string value for property '" << stringFromCFString(prop) << "' for joystick at index "
              << index << std::endl;
    return "Unknown Joystick";
}


// Get HID device property key as an unsigned int
unsigned int getDeviceUint(IOHIDDeviceRef ref, CFStringRef prop, unsigned int index)
{
    CFTypeRef typeRef = IOHIDDeviceGetProperty(ref, prop);
    if (typeRef && (CFGetTypeID(typeRef) == CFNumberGetTypeID()))
    {
        SInt32 value = 0;
        CFNumberGetValue(static_cast<CFNumberRef>(typeRef), kCFNumberSInt32Type, &value);
        return static_cast<unsigned int>(value);
    }

    sf::err() << "Unable to read uint value for property '" << stringFromCFString(prop) << "' for joystick at index "
              << index << std::endl;
    return 0;
}
} // namespace


namespace sf::priv
{
////////////////////////////////////////////////////////////
void JoystickImpl::initialize()
{
    // Nothing to do
}


////////////////////////////////////////////////////////////
void JoystickImpl::cleanup()
{
    // Nothing to do
}


////////////////////////////////////////////////////////////
bool JoystickImpl::isConnected(unsigned int index)
{
    const AutoreleasePool pool;
    bool                  state = false; // Is the index-th joystick connected?

    // First, let's check if the device was previously detected:
    if (m_locationIDs[index] != 0)
    {
        state = true;
    }
    else
    {
        // Otherwise, let's check if it is now connected
        // i.e., m_locationIDs[index] == 0

        // if there is more connected joystick to the HID manager than
        // opened joystick devices then we find the new one.

        unsigned int openedCount = 0;
        for (const auto& locationID : m_locationIDs)
        {
            if (locationID != 0)
                ++openedCount;
        }


        const unsigned int connectedCount = HIDJoystickManager::getInstance().getJoystickCount();

        if (connectedCount > openedCount)
        {
            // Get all devices
            const auto devices = CFPtr<CFSetRef>(HIDJoystickManager::getInstance().copyJoysticks());

            if (devices != nullptr)
            {
                const CFIndex size = CFSetGetCount(devices.get());
                if (size > 0)
                {
                    std::vector<CFTypeRef> array(static_cast<std::size_t>(size)); // array of IOHIDDeviceRef
                    CFSetGetValues(devices.get(), array.data());

                    // If there exists a device d s.t. there is no j s.t.
                    // m_locationIDs[j] == d's location then we have a new device.

                    for (CFIndex didx(0); !state && didx < size; ++didx)
                    {
                        auto* d = static_cast<IOHIDDeviceRef>(const_cast<void*>(array[static_cast<std::size_t>(didx)]));
                        const Location dloc = HIDInputManager::getLocationID(d);

                        bool foundJ = false;
                        for (unsigned int j(0); !foundJ && j < Joystick::Count; ++j)
                        {
                            if (m_locationIDs[j] == dloc)
                                foundJ = true;
                        }

                        if (!foundJ)
                        {
                            // This is a new device
                            // We set it up for Open(..)
                            m_locationIDs[index] = dloc;
                            state                = true;
                        }
                    }
                }
            }
        }
    }

    return state;
}


////////////////////////////////////////////////////////////
bool JoystickImpl::open(unsigned int index)
{
    const AutoreleasePool pool;
    m_index                  = index;
    m_hat                    = nullptr;
    const Location deviceLoc = m_locationIDs[index]; // The device we need to load

    // Get all devices
    const auto devices = HIDJoystickManager::getInstance().copyJoysticks();
    if (devices == nullptr)
        return false;

    // Get a usable copy of the joysticks devices.
    const CFIndex          joysticksCount = CFSetGetCount(devices.get());
    std::vector<CFTypeRef> devicesArray(static_cast<std::size_t>(joysticksCount));
    CFSetGetValues(devices.get(), devicesArray.data());

    // Get the desired joystick.
    IOHIDDeviceRef self = nil;
    for (CFIndex i(0); self == nil && i < joysticksCount; ++i)
    {
        auto* d = static_cast<IOHIDDeviceRef>(const_cast<void*>(devicesArray[static_cast<std::size_t>(i)]));
        if (deviceLoc == HIDInputManager::getLocationID(d))
            self = d;
    }

    if (self == nil)
        return false;

    m_identification.name      = getDeviceString(self, CFSTR(kIOHIDProductKey), m_index);
    m_identification.vendorId  = getDeviceUint(self, CFSTR(kIOHIDVendorIDKey), m_index);
    m_identification.productId = getDeviceUint(self, CFSTR(kIOHIDProductIDKey), m_index);

    // Get a list of all elements attached to the device.
    const auto elements = CFPtr<CFArrayRef>(IOHIDDeviceCopyMatchingElements(self, nullptr, kIOHIDOptionsTypeNone));

    if (elements == nullptr)
        return false;

    // Go through all connected elements.
    const CFIndex elementsCount = CFArrayGetCount(elements.get());
    for (int i = 0; i < elementsCount; ++i)
    {
        auto element = std::shared_ptr(CFPtr<IOHIDElementRef>(
            static_cast<IOHIDElementRef>(const_cast<void*>(CFArrayGetValueAtIndex(elements.get(), i)))));
        switch (IOHIDElementGetUsagePage(element.get()))
        {
            case kHIDPage_GenericDesktop:
                switch (IOHIDElementGetUsage(element.get()))
                {
                    case kHIDUsage_GD_X:
                        m_axis[Joystick::Axis::X] = element;
                        break;
                    case kHIDUsage_GD_Y:
                        m_axis[Joystick::Axis::Y] = element;
                        break;
                    case kHIDUsage_GD_Z:
                        m_axis[Joystick::Axis::Z] = element;
                        break;
                    case kHIDUsage_GD_Rx:
                        m_axis[Joystick::Axis::U] = element;
                        break;
                    case kHIDUsage_GD_Ry:
                        m_axis[Joystick::Axis::V] = element;
                        break;
                    case kHIDUsage_GD_Rz:
                        m_axis[Joystick::Axis::R] = element;
                        break;

                    case kHIDUsage_GD_Hatswitch:
                        // From §4.3 MiscellaneousControls of HUT v1.12:
                        //
                        // > Hat Switch:
                        // >   A typical example is four switches that are capable of generating
                        // >   information about four possible directions in which the knob can be
                        // >   tilted. Intermediate positions can also be decoded if the hardware
                        // >   allows two switches to be reported simultaneously.
                        //
                        // We assume this model here as well. Hence, with 4 switches and intermediate
                        // positions we have 8 values (0-7) plus the "null" state (8).
                        {
                            const CFIndex min = IOHIDElementGetLogicalMin(element.get());
                            const CFIndex max = IOHIDElementGetLogicalMax(element.get());

                            if (min != 0 || max != 7)
                            {
                                sf::err() << std::hex << "Joystick (vendor/product id: 0x" << m_identification.vendorId
                                          << "/0x" << m_identification.productId << std::dec
                                          << ") range is an unexpected one: [" << min << ", " << max << "]" << std::endl;
                            }
                            else
                            {
                                m_hat = element;
                            }
                        }
                        break;

                    case kHIDUsage_GD_GamePad:
                        // We assume a game pad is an application collection, meaning it doesn't hold
                        // any values per say. They kind of "emit" the joystick's usages.
                        // See §3.4.3 Usage Types (Collection) of HUT v1.12
                        if (IOHIDElementGetCollectionType(element.get()) != kIOHIDElementCollectionTypeApplication)
                        {
                            sf::err() << std::hex << "Gamepage (vendor/product id: 0x" << m_identification.vendorId
                                      << "/0x" << m_identification.productId << ") is not an CA but a 0x"
                                      << IOHIDElementGetCollectionType(element.get()) << std::dec << std::endl;
                        }
                        break;

                    default:
#ifdef SFML_DEBUG
                        sf::err() << "Unexpected usage for element of Page Generic Desktop: 0x" << std::hex
                                  << IOHIDElementGetUsage(element.get()) << std::dec << std::endl;
#endif
                        break;
                }
                break;

            case kHIDPage_Button:
                if (m_buttons.size() < Joystick::ButtonCount) // If we have free slot...
                    m_buttons.push_back(element);             // ...we add this element to the list
                // Else: too many buttons. We ignore this one.
                break;

            default: /* No other page is expected because of the mask applied by the HID manager. */
                break;
        }
    }

    // Ensure that the buttons will be indexed in the same order as their
    // HID Usage (assigned by manufacturer and/or a driver).
    std::sort(m_buttons.begin(),
              m_buttons.end(),
              [](auto b1, auto b2) { return IOHIDElementGetUsage(b1.get()) < IOHIDElementGetUsage(b2.get()); });

    return true;
}


////////////////////////////////////////////////////////////
void JoystickImpl::close()
{
    const AutoreleasePool pool;

    m_buttons.clear();
    m_axis.clear();
    m_hat.reset();

    // And we unregister this joystick
    m_locationIDs[m_index] = 0;
}


////////////////////////////////////////////////////////////
JoystickCaps JoystickImpl::getCapabilities() const
{
    const AutoreleasePool pool;
    JoystickCaps          caps;

    // Buttons:
    caps.buttonCount = static_cast<unsigned int>(m_buttons.size());

    // Axis:
    for (const auto& [axis, iohidElementRef] : m_axis)
        caps.axes[axis] = true;

    if (m_hat != nullptr)
        caps.axes[Joystick::Axis::PovX] = caps.axes[Joystick::Axis::PovY] = true;

    return caps;
}


////////////////////////////////////////////////////////////
Joystick::Identification JoystickImpl::getIdentification() const
{
    const AutoreleasePool pool;
    return m_identification;
}


////////////////////////////////////////////////////////////
JoystickState JoystickImpl::update()
{
    const AutoreleasePool          pool;
    static constexpr JoystickState disconnectedState; // return this if joystick was disconnected
    JoystickState                  state;             // otherwise return that
    state.connected = true;

    // Note: free up is done in close() which is called, if required,
    //       by the joystick manager. So we don't release buttons nor axes here.

    // First, let's determine if the joystick is still connected
    const Location selfLoc = m_locationIDs[m_index];

    // Get all devices
    auto devices = HIDJoystickManager::getInstance().copyJoysticks();
    if (devices == nullptr)
        return disconnectedState;

    // Get a usable copy of the joysticks devices.
    const CFIndex          joysticksCount = CFSetGetCount(devices.get());
    std::vector<CFTypeRef> devicesArray(static_cast<std::size_t>(joysticksCount));
    CFSetGetValues(devices.get(), devicesArray.data());

    // Search for it
    bool found = false;
    for (CFIndex i(0); !found && i < joysticksCount; ++i)
    {
        auto* d = static_cast<IOHIDDeviceRef>(const_cast<void*>(devicesArray[static_cast<std::size_t>(i)]));
        if (selfLoc == HIDInputManager::getLocationID(d))
            found = true;
    }

    // If not found we consider it disconnected
    if (!found)
        return disconnectedState;

    // Update buttons' state
    for (std::size_t i = 0; i < m_buttons.size(); ++i)
    {
        IOHIDValueRef value = nil;
        IOHIDDeviceGetValue(IOHIDElementGetDevice(m_buttons[i].get()), m_buttons[i].get(), &value);

        // Check for plug out.
        if (!value)
        {
            return disconnectedState;
        }

        // 1 means pressed, others mean released
        state.buttons[i] = IOHIDValueGetIntegerValue(value) == 1;
    }

    // Update axes' state
    for (const auto& [axis, iohidElementRef] : m_axis)
    {
        IOHIDValueRef value = nil;
        IOHIDDeviceGetValue(IOHIDElementGetDevice(iohidElementRef.get()), iohidElementRef.get(), &value);

        // Check for plug out.
        if (!value)
        {
            return disconnectedState;
        }

        // We want to bind [physicalMin,physicalMax] to [-100=min,100=max].
        //
        // General formula to bind [a,b] to [c,d] with a linear progression:
        //
        // f: [a, b] -> [c, d]
        //        x  |->  (x-a)(d-c)/(b-a)+c
        //
        // This method might not be very accurate (the "0 position" can be
        // slightly shift with some device) but we don't care because most
        // of devices are so sensitive that this is not relevant.
        const auto   physicalMax   = static_cast<double>(IOHIDElementGetPhysicalMax(iohidElementRef.get()));
        const auto   physicalMin   = static_cast<double>(IOHIDElementGetPhysicalMin(iohidElementRef.get()));
        const double scaledMin     = -100;
        const double scaledMax     = 100;
        const double physicalValue = IOHIDValueGetScaledValue(value, kIOHIDValueScaleTypePhysical);
        const auto   scaledValue   = static_cast<float>(
            (((physicalValue - physicalMin) * (scaledMax - scaledMin)) / (physicalMax - physicalMin)) + scaledMin);
        state.axes[axis] = scaledValue;
    }

    // Update POV/Hat state. Assuming model described in `open`, values are:
    //
    //   North-West / 7    North / 0   North-East / 1
    //   West / 6          Null  / 8         East / 2
    //   South-West / 5    South / 4   South-East / 3
    //
    if (m_hat != nullptr)
    {
        IOHIDValueRef value = nil;
        IOHIDDeviceGetValue(IOHIDElementGetDevice(m_hat.get()), m_hat.get(), &value);

        // Check for plug out.
        if (!value)
        {
            return disconnectedState;
        }

        const CFIndex raw = IOHIDValueGetIntegerValue(value);

        // Load PovX
        switch (raw)
        {
            case 1:
            case 2:
            case 3:
                state.axes[Joystick::Axis::PovX] = +100;
                break;

            case 5:
            case 6:
            case 7:
                state.axes[Joystick::Axis::PovX] = -100;
                break;

            default:
                state.axes[Joystick::Axis::PovX] = 0;
                break;
        }

        // Load PovY
        switch (raw)
        {
            case 0:
            case 1:
            case 7:
                state.axes[Joystick::Axis::PovY] = +100;
                break;

            case 3:
            case 4:
            case 5:
                state.axes[Joystick::Axis::PovY] = -100;
                break;

            default:
                state.axes[Joystick::Axis::PovY] = 0;
                break;
        }
    }

    return state;
}

} // namespace sf::priv
