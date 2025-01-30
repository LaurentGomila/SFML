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
#include <SFML/Window/macOS/AutoreleasePoolWrapper.hpp>
#import <SFML/Window/macOS/SFApplication.h>
#import <SFML/Window/macOS/SFApplicationDelegate.h>
#import <SFML/Window/macOS/SFKeyboardModifiersHelper.h>
#import <SFML/Window/macOS/SFViewController.h>
#import <SFML/Window/macOS/SFWindowController.h>
#import <SFML/Window/macOS/Scaling.h>
#include <SFML/Window/macOS/WindowImplCocoa.hpp>

#include <SFML/System/Err.hpp>
#include <SFML/System/String.hpp>

#include <limits>
#include <ostream>

namespace sf::priv
{

////////////////////////////////////////////////////////////
/// According to Apple's documentation, each invocation of
/// unhide must be balanced by an invocation of hide in
/// order for the cursor display to be correct.
/// So we keep track of those calls ourself.
////////////////////////////////////////////////////////////

namespace
{
bool isCursorHidden = false; // initially, the cursor is visible

NSString* sfStringToNSString(const sf::String& string)
{
    const auto  length = static_cast<std::uint32_t>(string.getSize() * sizeof(std::uint32_t));
    const void* data   = reinterpret_cast<const void*>(string.getData());

    NSStringEncoding encoding = 0;
    if (NSHostByteOrder() == NS_LittleEndian)
        encoding = NSUTF32LittleEndianStringEncoding;
    else
        encoding = NSUTF32BigEndianStringEncoding;

    NSString* const str = [[NSString alloc] initWithBytes:data length:length encoding:encoding];
    return [str autorelease];
}


////////////////////////////////////////////////////////
void hideMouseCursor()
{
    const AutoreleasePool pool;
    if (!isCursorHidden)
    {
        [NSCursor hide];
        isCursorHidden = true;
    }
}


////////////////////////////////////////////////////////
void showMouseCursor()
{
    const AutoreleasePool pool;
    if (isCursorHidden)
    {
        [NSCursor unhide];
        isCursorHidden = false;
    }
}
} // namespace

#pragma mark
#pragma mark WindowImplCocoa's ctor/dtor

////////////////////////////////////////////////////////////
WindowImplCocoa::WindowImplCocoa(WindowHandle handle)
{
    const AutoreleasePool pool;
    // Treat the handle as it real type
    const id nsHandle = static_cast<id>(handle);
    if ([nsHandle isKindOfClass:[NSWindow class]])
    {
        // We have a window.
        m_delegate = [[SFWindowController alloc] initWithWindow:nsHandle];
    }
    else if ([nsHandle isKindOfClass:[NSView class]])
    {
        // We have a view.
        m_delegate = [[SFViewController alloc] initWithView:nsHandle];
    }
    else
    {

        sf::err() << "Cannot import this Window Handle because it is neither "
                  << "a <NSWindow*> nor <NSView*> object "
                  << "(or any of their subclasses). You gave a <" << [[nsHandle className] UTF8String] << "> object."
                  << std::endl;
        return;
    }

    [m_delegate setRequesterTo:this];

    // Finally, set up keyboard helper
    initialiseKeyboardHelper();
}


////////////////////////////////////////////////////////////
WindowImplCocoa::WindowImplCocoa(VideoMode mode, const String& title, std::uint32_t style, State state, const ContextSettings& /*settings*/)
{
    const AutoreleasePool pool;
    // Transform the app process.
    setUpProcess();

    m_delegate = [[SFWindowController alloc] initWithMode:mode andStyle:style andState:state];
    [m_delegate changeTitle:sfStringToNSString(title)];
    [m_delegate setRequesterTo:this];

    // Finally, set up keyboard helper
    initialiseKeyboardHelper();
}


////////////////////////////////////////////////////////////
WindowImplCocoa::~WindowImplCocoa()
{
    const AutoreleasePool pool;
    [m_delegate closeWindow];
    // Tell the window/view controller (and the OpenGL view) that the delegate
    // (this object) no longer exists to prevent events being sent to the window
    // after it has been deleted.
    [m_delegate setRequesterTo:nil];
    [m_delegate release];

    // Put the next window in front, if any.
    NSArray* const windows = [NSApp orderedWindows];
    if ([windows count] > 0)
    {
        NSWindow* const nextWindow = [windows objectAtIndex:0];
        if ([nextWindow isVisible])
            [nextWindow makeKeyAndOrderFront:nil];
    }
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::applyContext(NSOpenGLContextRef context) const
{
    const AutoreleasePool pool;
    [m_delegate applyContext:context];
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::setUpProcess()
{
    const AutoreleasePool pool;
    static bool           isTheProcessSetAsApplication = false;

    if (!isTheProcessSetAsApplication)
    {
        // Do it only once !
        isTheProcessSetAsApplication = true;

        // Make sure NSApp is properly initialized
        [SFApplication sharedApplication];

        // Set the process as a normal application so it can get focus
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp activateIgnoringOtherApps:YES];

        // Register an application delegate if there is none
        if (![[SFApplication sharedApplication] delegate])
            [[NSApplication sharedApplication] setDelegate:[[SFApplicationDelegate alloc] init]];

        // Create menus for the application (before finishing launching!)
        [SFApplication setUpMenuBar];

        // Tell the application to stop bouncing in the Dock.
        [[SFApplication sharedApplication] finishLaunching];
        // NOTE: This last call won't harm anything even if SFML window was
        // created with an external handle.
    }
}


#pragma mark
#pragma mark WindowImplCocoa's window-event methods


////////////////////////////////////////////////////////////
void WindowImplCocoa::windowClosed()
{
    pushEvent(Event::Closed{});
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::windowResized(Vector2u size)
{
    scaleOutXY(size, m_delegate);
    pushEvent(Event::Resized{size});
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::windowFocusLost()
{
    if (!m_showCursor && [m_delegate isMouseInside])
        showMouseCursor(); // Make sure the cursor is visible
    pushEvent(Event::FocusLost{});
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::windowFocusGained()
{
    if (!m_showCursor && [m_delegate isMouseInside])
        hideMouseCursor(); // Restore user's setting
    pushEvent(Event::FocusGained{});
}

#pragma mark
#pragma mark WindowImplCocoa's mouse-event methods


////////////////////////////////////////////////////////////
void WindowImplCocoa::mouseDownAt(Mouse::Button button, Vector2i position)
{
    scaleOutXY(position, m_delegate);
    pushEvent(Event::MouseButtonPressed{button, position});
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::mouseUpAt(Mouse::Button button, Vector2i position)
{
    scaleOutXY(position, m_delegate);
    pushEvent(Event::MouseButtonReleased{button, position});
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::mouseMovedAt(Vector2i position)
{
    scaleOutXY(position, m_delegate);
    pushEvent(Event::MouseMoved{position});
}

////////////////////////////////////////////////////////////
void WindowImplCocoa::mouseWheelScrolledAt(float deltaX, float deltaY, Vector2i position)
{
    scaleOutXY(position, m_delegate);
    pushEvent(Event::MouseWheelScrolled{Mouse::Wheel::Vertical, deltaY, position});
    pushEvent(Event::MouseWheelScrolled{Mouse::Wheel::Horizontal, deltaX, position});
}

////////////////////////////////////////////////////////////
void WindowImplCocoa::mouseMovedIn()
{
    if (!m_showCursor)
        hideMouseCursor(); // Restore user's setting
    pushEvent(Event::MouseEntered{});
}

////////////////////////////////////////////////////////////
void WindowImplCocoa::mouseMovedOut()
{
    if (!m_showCursor)
        showMouseCursor(); // Make sure the cursor is visible
    pushEvent(Event::MouseLeft{});
}


#pragma mark
#pragma mark WindowImplCocoa's key-event methods


////////////////////////////////////////////////////////////
void WindowImplCocoa::keyDown(Event::KeyPressed event)
{
    pushEvent(event);
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::keyUp(Event::KeyReleased event)
{
    pushEvent(event);
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::textEntered(unichar charcode)
{
    pushEvent(Event::TextEntered{charcode});
}


#pragma mark
#pragma mark WindowImplCocoa's event-related methods

////////////////////////////////////////////////////////////
void WindowImplCocoa::processEvents()
{
    const AutoreleasePool pool;
    [m_delegate processEvent];
}

#pragma mark
#pragma mark WindowImplCocoa's private methods

////////////////////////////////////////////////////////////
WindowHandle WindowImplCocoa::getNativeHandle() const
{
    const AutoreleasePool pool;
    return [m_delegate getNativeHandle];
}


////////////////////////////////////////////////////////////
Vector2i WindowImplCocoa::getPosition() const
{
    const AutoreleasePool pool;
    const NSPoint         pos = [m_delegate position];
    sf::Vector2i          ret(static_cast<int>(pos.x), static_cast<int>(pos.y));
    scaleOutXY(ret, m_delegate);
    return ret;
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::setPosition(Vector2i position)
{
    const AutoreleasePool pool;
    sf::Vector2i          backingPosition = position;
    scaleInXY(backingPosition, m_delegate);
    [m_delegate setWindowPositionTo:backingPosition];
}


////////////////////////////////////////////////////////////
Vector2u WindowImplCocoa::getSize() const
{
    const AutoreleasePool pool;
    const NSSize          size = [m_delegate size];
    Vector2u              ret(static_cast<unsigned int>(size.width), static_cast<unsigned int>(size.height));
    scaleOutXY(ret, m_delegate);
    return ret;
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::setSize(Vector2u size)
{
    sf::Vector2u backingSize = size;
    scaleInXY(backingSize, m_delegate);
    [m_delegate resizeTo:backingSize];
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::setMinimumSize(const std::optional<Vector2u>& minimumSize)
{
    WindowImpl::setMinimumSize(minimumSize);
    const AutoreleasePool pool;
    const NSSize          size = minimumSize ? NSMakeSize(minimumSize->x, minimumSize->y) : NSMakeSize(0, 0);
    [m_delegate setMinimumSize:size];
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::setMaximumSize(const std::optional<Vector2u>& maximumSize)
{
    WindowImpl::setMaximumSize(maximumSize);
    const AutoreleasePool pool;
    const NSSize          size = maximumSize ? NSMakeSize(maximumSize->x, maximumSize->y)
                                             : NSMakeSize(std::numeric_limits<CGFloat>::max(), std::numeric_limits<CGFloat>::max());
    [m_delegate setMaximumSize:size];
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::setTitle(const String& title)
{
    const AutoreleasePool pool;
    [m_delegate changeTitle:sfStringToNSString(title)];
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::setIcon(Vector2u size, const std::uint8_t* pixels)
{
    const AutoreleasePool pool;
    [m_delegate setIconTo:size with:pixels];
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::setVisible(bool visible)
{
    const AutoreleasePool pool;
    if (visible)
        [m_delegate showWindow];
    else
        [m_delegate hideWindow];
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::setMouseCursorVisible(bool visible)
{
    const AutoreleasePool pool;
    m_showCursor = visible;

    // If the mouse is over the window, we apply the new setting
    if ([m_delegate isMouseInside])
    {
        if (m_showCursor)
            showMouseCursor();
        else
            hideMouseCursor();
    }
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::setMouseCursorGrabbed(bool grabbed)
{
    const AutoreleasePool pool;
    [m_delegate setCursorGrabbed:grabbed];
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::setMouseCursor(const CursorImpl& cursor)
{
    const AutoreleasePool pool;
    [m_delegate setCursor:cursor.m_cursor];
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::setKeyRepeatEnabled(bool enabled)
{
    const AutoreleasePool pool;
    if (enabled)
        [m_delegate enableKeyRepeat];
    else
        [m_delegate disableKeyRepeat];
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::requestFocus()
{
    const AutoreleasePool pool;
    [m_delegate requestFocus];
}


////////////////////////////////////////////////////////////
bool WindowImplCocoa::hasFocus() const
{
    const AutoreleasePool pool;
    return [m_delegate hasFocus];
}


////////////////////////////////////////////////////////////
void WindowImplCocoa::setImePreEditPosition(const Vector2i& /* position */)
{
    // TODO: To implement
}


} // namespace sf::priv
