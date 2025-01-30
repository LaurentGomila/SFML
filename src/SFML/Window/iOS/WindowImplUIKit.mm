////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2025 Laurent Gomila (laurent@sfml-dev.org)
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
#include <SFML/Window/WindowEnums.hpp>
#include <SFML/Window/iOS/SFAppDelegate.hpp>
#include <SFML/Window/iOS/SFView.hpp>
#include <SFML/Window/iOS/SFViewController.hpp>
#include <SFML/Window/iOS/WindowImplUIKit.hpp>

#include <SFML/System/Err.hpp>

#include <UIKit/UIKit.h>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

namespace sf::priv
{
////////////////////////////////////////////////////////////
WindowImplUIKit::WindowImplUIKit(WindowHandle /* handle */)
{
    // Not implemented
}


////////////////////////////////////////////////////////////
WindowImplUIKit::WindowImplUIKit(VideoMode mode,
                                 const String& /* title */,
                                 std::uint32_t style,
                                 State         state,
                                 const ContextSettings& /* settings */)
{
    m_backingScale = static_cast<float>([SFAppDelegate getInstance].backingScaleFactor);

    // Apply the fullscreen flag
    [UIApplication sharedApplication].statusBarHidden = !(style & Style::Titlebar) || (state == State::Fullscreen);

    // Set the orientation according to the requested size
    if (mode.size.x > mode.size.y)
        [[UIApplication sharedApplication] setStatusBarOrientation:UIInterfaceOrientationLandscapeLeft];
    else
        [[UIApplication sharedApplication] setStatusBarOrientation:UIInterfaceOrientationPortrait];

    // Create the window
    const CGRect frame = [UIScreen mainScreen].bounds; // Ignore user size, it wouldn't make sense to use something else
    m_window           = [[UIWindow alloc] initWithFrame:frame];
    m_hasFocus         = true;

    // Assign it to the application delegate
    [SFAppDelegate getInstance].sfWindow = this;

    const CGRect viewRect = frame;

    // Create the view
    m_view = [[SFView alloc] initWithFrame:viewRect andContentScaleFactor:(static_cast<double>(m_backingScale))];
    [m_view resignFirstResponder];

    // Create the view controller
    m_viewController                      = [SFViewController alloc];
    m_viewController.view                 = m_view;
    m_viewController.orientationCanChange = style & Style::Resize;
    m_window.rootViewController           = m_viewController;

    // Make it the current window
    [m_window makeKeyAndVisible];
}


////////////////////////////////////////////////////////////
void WindowImplUIKit::processEvents()
{
    while (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.0001, true) == kCFRunLoopRunHandledSource)
        ;
}


////////////////////////////////////////////////////////////
WindowHandle WindowImplUIKit::getNativeHandle() const
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

    return (__bridge WindowHandle)m_window;

#pragma GCC diagnostic pop
}


////////////////////////////////////////////////////////////
Vector2i WindowImplUIKit::getPosition() const
{
    const CGPoint origin = m_window.frame.origin;
    return {static_cast<int>(origin.x * static_cast<double>(m_backingScale)),
            static_cast<int>(origin.y * static_cast<double>(m_backingScale))};
}


////////////////////////////////////////////////////////////
void WindowImplUIKit::setPosition(Vector2i /* position */)
{
}


////////////////////////////////////////////////////////////
Vector2u WindowImplUIKit::getSize() const
{
    const CGRect physicalFrame = m_window.frame;
    return {static_cast<unsigned int>(physicalFrame.size.width * static_cast<double>(m_backingScale)),
            static_cast<unsigned int>(physicalFrame.size.height * static_cast<double>(m_backingScale))};
}


////////////////////////////////////////////////////////////
void WindowImplUIKit::setSize(Vector2u size)
{
    // TODO ...

    // if these sizes are required one day, don't forget to scale them!
    // size.x /= m_backingScale;
    // size.y /= m_backingScale;

    // Set the orientation according to the requested size
    if (size.x > size.y)
        [[UIApplication sharedApplication] setStatusBarOrientation:UIInterfaceOrientationLandscapeLeft];
    else
        [[UIApplication sharedApplication] setStatusBarOrientation:UIInterfaceOrientationPortrait];
}


////////////////////////////////////////////////////////////
void WindowImplUIKit::setMinimumSize(const std::optional<Vector2u>& /* minimumSize */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplUIKit::setMaximumSize(const std::optional<Vector2u>& /* maximumSize */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplUIKit::setTitle(const String& /* title */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplUIKit::setIcon(Vector2u /* size */, const std::uint8_t* /* pixels */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplUIKit::setVisible(bool /* visible */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplUIKit::setMouseCursorVisible(bool /* visible */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplUIKit::setMouseCursorGrabbed(bool /* grabbed */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplUIKit::setMouseCursor(const CursorImpl& /* cursor */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplUIKit::setKeyRepeatEnabled(bool /* enabled */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplUIKit::requestFocus()
{
    // To implement
}


////////////////////////////////////////////////////////////
bool WindowImplUIKit::hasFocus() const
{
    return m_hasFocus;
}


////////////////////////////////////////////////////////////
void WindowImplUIKit::setImePreEditPosition(const Vector2i& /* position */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplUIKit::forwardEvent(Event event)
{
    if (event.is<Event::FocusGained>())
        m_hasFocus = true;
    else if (event.is<Event::FocusLost>())
        m_hasFocus = false;

    pushEvent(event);
}


////////////////////////////////////////////////////////////
SFView* WindowImplUIKit::getGlView() const
{
    return m_view;
}


////////////////////////////////////////////////////////////
void WindowImplUIKit::setVirtualKeyboardVisible(bool visible)
{
    if (visible)
        [m_view becomeFirstResponder];
    else
        [m_view resignFirstResponder];
}

} // namespace sf::priv
