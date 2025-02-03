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
#include <SFML/Window/InputImpl.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/Window.hpp>
#include <SFML/Window/macOS/AutoreleasePoolWrapper.hpp>
#include <SFML/Window/macOS/HIDInputManager.hpp>
#import <SFML/Window/macOS/SFOpenGLView.h>

#include <SFML/System/Err.hpp>

#import <AppKit/AppKit.h>
#include <ostream>

////////////////////////////////////////////////////////////
/// In order to keep track of the keyboard's state and mouse buttons' state
/// we use the HID manager. Mouse position is handled differently.
///
////////////////////////////////////////////////////////////

namespace
{
////////////////////////////////////////////////////////////
/// \brief Extract the dedicated SFOpenGLView from the SFML window
///
/// \param window a SFML window
/// \return nil if something went wrong or a SFOpenGLView*.
///
////////////////////////////////////////////////////////////
SFOpenGLView* getSFOpenGLViewFromSFMLWindow(const sf::WindowBase& window)
{
    const id nsHandle = static_cast<id>(window.getNativeHandle());

    // Get our SFOpenGLView from ...
    SFOpenGLView* view = nil;

    if ([nsHandle isKindOfClass:[NSWindow class]])
    {
        // If system handle is a window then from its content view.
        view = [nsHandle contentView];

        // Subview doesn't match ?
        if (![view isKindOfClass:[SFOpenGLView class]])
        {
            if ([view isKindOfClass:[NSView class]])
            {
                NSArray* const subviews = [view subviews];
                for (NSView* subview in subviews) // NOLINT(cppcoreguidelines-init-variables)
                {
                    if ([subview isKindOfClass:[SFOpenGLView class]])
                    {
                        view = static_cast<SFOpenGLView*>(subview);
                        break;
                    }
                }
            }
            else
            {
                sf::err() << "The content view is not a valid SFOpenGLView" << std::endl;

                view = nil;
            }
        }
    }
    else if ([nsHandle isKindOfClass:[NSView class]])
    {
        // If system handle is a view then from a subview of kind SFOpenGLView.
        NSArray* const subviews = [nsHandle subviews];
        for (NSView* subview in subviews) // NOLINT(cppcoreguidelines-init-variables)
        {
            if ([subview isKindOfClass:[SFOpenGLView class]])
            {
                view = static_cast<SFOpenGLView*>(subview);
                break;
            }
        }

        // No matching subview ?
        if (view == nil)
            sf::err() << "Cannot find a valid SFOpenGLView subview." << std::endl;
    }
    else
    {
        if (nsHandle != nil)
            sf::err() << "The system handle is neither a <NSWindow*> nor <NSView*>"
                      << "object. This shouldn't happen." << std::endl;
        // Else: this probably means the SFML window was previously closed.
    }

    return view;
}
} // namespace

namespace sf::priv::InputImpl
{
////////////////////////////////////////////////////////////
bool isKeyPressed(Keyboard::Key key)
{
    const AutoreleasePool pool;
    return HIDInputManager::getInstance().isKeyPressed(key);
}


////////////////////////////////////////////////////////////
bool isKeyPressed(Keyboard::Scancode code)
{
    return HIDInputManager::getInstance().isKeyPressed(code);
}


////////////////////////////////////////////////////////////
Keyboard::Key localize(Keyboard::Scancode code)
{
    return HIDInputManager::getInstance().localize(code);
}


////////////////////////////////////////////////////////////
Keyboard::Scancode delocalize(Keyboard::Key key)
{
    return HIDInputManager::getInstance().delocalize(key);
}


////////////////////////////////////////////////////////////
String getDescription(Keyboard::Scancode code)
{
    return HIDInputManager::getInstance().getDescription(code);
}


////////////////////////////////////////////////////////////
void setVirtualKeyboardVisible(bool /*visible*/)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
bool isMouseButtonPressed(Mouse::Button button)
{
    const AutoreleasePool pool;
    const NSUInteger      state = [NSEvent pressedMouseButtons];
    const NSUInteger      flag  = 1 << static_cast<int>(button);
    return (state & flag) != 0;
}


////////////////////////////////////////////////////////////
Vector2i getMousePosition()
{
    const AutoreleasePool pool;
    // Reverse Y axis to match SFML coord.
    NSPoint pos = [NSEvent mouseLocation];
    pos.y       = sf::VideoMode::getDesktopMode().size.y - pos.y;

    const int scale = static_cast<int>([[NSScreen mainScreen] backingScaleFactor]);
    return Vector2i(static_cast<int>(pos.x), static_cast<int>(pos.y)) * scale;
}


////////////////////////////////////////////////////////////
Vector2i getMousePosition(const sf::WindowBase& relativeTo)
{
    const AutoreleasePool pool;
    SFOpenGLView* const   view = getSFOpenGLViewFromSFMLWindow(relativeTo);

    // No view ?
    if (view == nil)
        return {};

    // Use -cursorPositionFromEvent: with nil.
    const NSPoint pos = [view cursorPositionFromEvent:nil];

    const int scale = static_cast<int>([view displayScaleFactor]);
    return Vector2i(static_cast<int>(pos.x), static_cast<int>(pos.y)) * scale;
}


////////////////////////////////////////////////////////////
void setMousePosition(Vector2i position)
{
    const AutoreleasePool pool;
    // Here we don't need to reverse the coordinates.
    const int     scale = static_cast<int>([[NSScreen mainScreen] backingScaleFactor]);
    const CGPoint pos   = CGPointMake(position.x / scale, position.y / scale);

    // Place the cursor.
    const auto event = CFPtr<CGEventRef>(
        CGEventCreateMouseEvent(nullptr,
                                kCGEventMouseMoved,
                                pos,
                                /* we don't care about this: */ kCGMouseButtonLeft));
    CGEventPost(kCGHIDEventTap, event.get());
    // This is a workaround to deprecated CGSetLocalEventsSuppressionInterval.
}


////////////////////////////////////////////////////////////
void setMousePosition(Vector2i position, const WindowBase& relativeTo)
{
    const AutoreleasePool pool;
    SFOpenGLView* const   view = getSFOpenGLViewFromSFMLWindow(relativeTo);

    // No view ?
    if (view == nil)
        return;

    // Let SFOpenGLView compute the position in global coordinate
    const int scale = static_cast<int>([view displayScaleFactor]);
    NSPoint   p     = NSMakePoint(position.x / scale, position.y / scale);
    p               = [view computeGlobalPositionOfRelativePoint:p];
    setMousePosition(sf::Vector2i(static_cast<int>(p.x), static_cast<int>(p.y)) * scale);
}


////////////////////////////////////////////////////////////
bool isTouchDown(unsigned int /*finger*/)
{
    // Not applicable
    return false;
}


////////////////////////////////////////////////////////////
Vector2i getTouchPosition(unsigned int /*finger*/)
{
    // Not applicable
    return {};
}


////////////////////////////////////////////////////////////
Vector2i getTouchPosition(unsigned int /*finger*/, const WindowBase& /*relativeTo*/)
{
    // Not applicable
    return {};
}

} // namespace sf::priv::InputImpl
