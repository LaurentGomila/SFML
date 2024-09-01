////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2024 Marco Antognini (antognini.marco@gmail.com),
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
#import <SFML/Window/macOS/SFApplication.h>
#import <SFML/Window/macOS/SFOpenGLView.h>
#import <SFML/Window/macOS/SFViewController.h>
#include <SFML/Window/macOS/WindowImplCocoa.hpp>

#include <SFML/System/Err.hpp>

#include <ostream>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

@implementation SFViewController


////////////////////////////////////////////////////////
- (id)initWithView:(NSView*)view
{
    if ((self = [super init]))
    {
        m_requester = nullptr;

        // Retain the view for our own use.
        m_view = [view retain];

        if (m_view == nil)
        {
            sf::err() << "No view was given to initWithWindow:." << std::endl;
            return self;
        }

        // Create the view.
        NSRect frame   = [m_view frame];
        frame.origin.x = 0;
        frame.origin.y = 0;
        m_oglView      = [[SFOpenGLView alloc] initWithFrame:frame];

        if (m_oglView == nil)
        {
            sf::err() << "Could not create an instance of NSOpenGLView "
                      << "in (SFViewController -initWithView:)." << std::endl;

            return self;
        }

        // Set the (OGL) view to the view as its "content" view.
        [m_view addSubview:m_oglView];

        [m_oglView setAutoresizingMask:[m_view autoresizingMask]];

        [m_oglView finishInit];
    }

    return self;
}


////////////////////////////////////////////////////////
- (void)dealloc
{
    [self closeWindow];

    [m_view release];
    [m_oglView release];

    [super dealloc];
}


////////////////////////////////////////////////////////
- (CGFloat)displayScaleFactor
{
    return [m_oglView displayScaleFactor];
}


////////////////////////////////////////////////////////
- (void)setRequesterTo:(sf::priv::WindowImplCocoa*)requester
{
    // Forward to the view.
    [m_oglView setRequesterTo:requester];
    m_requester = requester;
}


////////////////////////////////////////////////////////
- (sf::WindowHandle)getNativeHandle
{
    return m_view;
}


////////////////////////////////////////////////////////
- (BOOL)isMouseInside
{
    return [m_oglView isMouseInside];
}


////////////////////////////////////////////////////////
- (void)setCursorGrabbed:(BOOL)grabbed
{
    [m_oglView setCursorGrabbed:grabbed];
}


////////////////////////////////////////////////////////
- (void)setCursor:(NSCursor*)cursor
{
    [m_oglView setCursor:cursor];
}


////////////////////////////////////////////////////////////
- (NSPoint)position
{
    // Origin is bottom-left corner of the window
    return [m_view convertPoint:NSMakePoint(0, 0) toView:nil]; // nil means window
}


////////////////////////////////////////////////////////.
- (void)setWindowPositionTo:(sf::Vector2i)position
{
    (void)position;
    sf::err() << "Cannot move SFML area when SFML is integrated in a NSView. Use the view handler directly instead."
              << std::endl;
}


////////////////////////////////////////////////////////////
- (NSSize)size
{
    return [m_oglView frame].size;
}


////////////////////////////////////////////////////////
- (void)resizeTo:(sf::Vector2u)size
{
    NSRect frame = NSMakeRect([m_view frame].origin.x, [m_view frame].origin.y, size.x, size.y);

    [m_view setFrame:frame];
    [m_oglView setFrame:frame];
}


////////////////////////////////////////////////////////
- (void)setMinimumSize:(NSSize)size
{
}


////////////////////////////////////////////////////////
- (void)setMaximumSize:(NSSize)size
{
}


////////////////////////////////////////////////////////
- (void)changeTitle:(NSString*)title
{
    (void)title;
    sf::err() << "Cannot change the title of the SFML area when SFML is integrated in a NSView." << std::endl;
}


////////////////////////////////////////////////////////
- (void)hideWindow
{
    [m_view setHidden:YES];
}


////////////////////////////////////////////////////////
- (void)showWindow
{
    [m_view setHidden:NO];
}


////////////////////////////////////////////////////////
- (void)closeWindow
{
    // If the "window" is really a view, this is a no-op.
}


////////////////////////////////////////////////////////
- (void)requestFocus
{
    // Note: this doesn't imply that the view will get any event.
    // The user has to make sure events are forwarded to the view
    // with the usual responder chain.
    [[m_view window] makeKeyAndOrderFront:nil];

    // In case the app is not active, make its dock icon bounce for one sec
    [NSApp requestUserAttention:NSInformationalRequest];
}


////////////////////////////////////////////////////////////
- (BOOL)hasFocus
{
    return [NSApp keyWindow] == [m_view window];
}


////////////////////////////////////////////////////////
- (void)enableKeyRepeat
{
    [m_oglView enableKeyRepeat];
}


////////////////////////////////////////////////////////
- (void)disableKeyRepeat
{
    [m_oglView disableKeyRepeat];
}


////////////////////////////////////////////////////////
- (void)setIconTo:(sf::Vector2u)size with:(const std::uint8_t*)pixels
{
    (void)size;
    (void)pixels;
    sf::err() << "Cannot set an icon when SFML is integrated in a NSView." << std::endl;
}


////////////////////////////////////////////////////////
- (void)processEvent
{
    // If we are not on the main thread we stop here and advice the user.
    if ([NSThread currentThread] != [NSThread mainThread])
    {
        /*
         * See https://lists.apple.com/archives/cocoa-dev/2011/Feb/msg00460.html
         * for more information.
         */
        sf::err() << "Cannot fetch event from a worker thread. (OS X restriction)" << std::endl;

        return;
    }

    // If we don't have a requester we don't fetch event.
    if (m_requester != nullptr)
        [SFApplication processEvent];
}


////////////////////////////////////////////////////////
- (void)applyContext:(NSOpenGLContext*)context
{
    [m_oglView setOpenGLContext:context];
    [context setView:m_oglView];
}

////////////////////////////////////////////////////////
- (void)setFileDroppingEnabled:(bool)enabled
{
    if (enabled)
    {
        [m_oglView registerForDraggedTypes:[NSArray arrayWithObjects:NSFilenamesPboardType, nil]];
    }
    else
    {
        [m_oglView unregisterDraggedTypes];
    }
}

@end
