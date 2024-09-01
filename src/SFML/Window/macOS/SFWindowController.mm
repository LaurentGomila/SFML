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
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <SFML/Window/WindowHandle.hpp>
#import <SFML/Window/macOS/NSImage+raw.h>
#import <SFML/Window/macOS/SFApplication.h>
#import <SFML/Window/macOS/SFOpenGLView.h>
#import <SFML/Window/macOS/SFWindow.h>
#import <SFML/Window/macOS/SFWindowController.h>
#import <SFML/Window/macOS/Scaling.h>
#include <SFML/Window/macOS/WindowImplCocoa.hpp>

#include <SFML/System/Err.hpp>

#include <ApplicationServices/ApplicationServices.h>
#import <OpenGL/OpenGL.h>
#include <algorithm>
#include <ostream>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

////////////////////////////////////////////////////////////
/// SFBlackView is a simple view filled with black, nothing more
///
////////////////////////////////////////////////////////////
@interface SFBlackView : NSView
@end

@implementation SFBlackView

////////////////////////////////////////////////////////////
- (void)drawRect:(NSRect)dirtyRect
{
    [[NSColor blackColor] setFill];
    NSRectFill(dirtyRect);
}

@end

////////////////////////////////////////////////////////////
/// SFWindowController class: private interface
///
////////////////////////////////////////////////////////////
@interface SFWindowController ()

////////////////////////////////////////////////////////////
/// \brief Retrieves the screen height
///
/// \return screen height
///
////////////////////////////////////////////////////////////
- (float)screenHeight;

////////////////////////////////////////////////////////////
/// \brief Retrieves the title bar height
///
/// \return title bar height
///
////////////////////////////////////////////////////////////
- (float)titlebarHeight;

@end

@implementation SFWindowController

#pragma mark
#pragma mark SFWindowController's methods

////////////////////////////////////////////////////////
- (id)initWithWindow:(NSWindow*)window
{
    if ((self = [super init]))
    {
        m_window        = nil;
        m_oglView       = nil;
        m_requester     = nil;
        m_fullscreen    = NO; // assuming this is the case... too hard to handle anyway.
        m_restoreResize = NO;
        m_highDpi       = NO;

        // Retain the window for our own use.
        m_window = [window retain];

        if (m_window == nil)
        {
            sf::err() << "No window was given to -[SFWindowController initWithWindow:]." << std::endl;
            return self;
        }

        // Create the view.
        m_oglView = [[SFOpenGLView alloc] initWithFrame:[[m_window contentView] frame] fullscreen:NO highDpi:NO];

        if (m_oglView == nil)
        {
            sf::err() << "Could not create an instance of NSOpenGLView "
                      << "in -[SFWindowController initWithWindow:]." << std::endl;
            return self;
        }

        // Set the view to the window as its content view.
        [m_window setContentView:m_oglView];

        [m_oglView finishInit];
    }

    return self;
}


////////////////////////////////////////////////////////
- (id)initWithMode:(const sf::VideoMode&)mode andStyle:(std::uint32_t)style andState:(sf::State)state
{
    // If we are not on the main thread we stop here and advice the user.
    if ([NSThread currentThread] != [NSThread mainThread])
    {
        /*
         * See https://lists.apple.com/archives/cocoa-dev/2011/Feb/msg00460.html
         * for more information.
         */
        sf::err() << "Cannot create a window from a worker thread. (OS X limitation)" << std::endl;

        return nil;
    }

    if ((self = [super init]))
    {
        m_window        = nil;
        m_oglView       = nil;
        m_requester     = nil;
        m_fullscreen    = (state == sf::State::Fullscreen) ? YES : NO;
        m_restoreResize = NO;
        m_highDpi       = NO;

        if (m_fullscreen)
            [self setupFullscreenViewWithMode:mode];
        else
            [self setupWindowWithMode:mode andStyle:style];

        [m_oglView finishInit];
    }
    return self;
}


////////////////////////////////////////////////////////
- (void)setupFullscreenViewWithMode:(const sf::VideoMode&)mode
{
    // Create a screen-sized window on the main display
    const sf::VideoMode desktop    = sf::VideoMode::getDesktopMode();
    NSRect              windowRect = NSMakeRect(0, 0, desktop.size.x, desktop.size.y);
    m_window                       = [[SFWindow alloc]
        initWithContentRect:windowRect
                  styleMask:NSBorderlessWindowMask
                    backing:NSBackingStoreBuffered
                      defer:NO];

    if (m_window == nil)
    {
        sf::err() << "Could not create an instance of NSWindow "
                  << "in -[SFWindowController setupFullscreenViewWithMode:]." << std::endl;
        return;
    }

    // Set the window level to be above the menu bar
    [m_window setLevel:NSMainMenuWindowLevel + 1];

    // More window configuration...
    [m_window setOpaque:YES];
    [m_window setHidesOnDeactivate:YES];
    [m_window setAutodisplay:YES];
    [m_window setReleasedWhenClosed:NO]; // We own the class, not AppKit

    // Register for event
    [m_window setDelegate:self];
    [m_window setAcceptsMouseMovedEvents:YES];
    [m_window setIgnoresMouseEvents:NO];

    // Create a master view containing our OpenGL view
    NSView* masterView = [[[SFBlackView alloc] initWithFrame:windowRect] autorelease];

    if (masterView == nil)
    {
        sf::err() << "Could not create an instance of SFBlackView "
                  << "in -[SFWindowController setupFullscreenViewWithMode:]." << std::endl;
        return;
    }

    // Create our OpenGL view size and the view
    CGFloat width   = std::min(mode.size.x, desktop.size.x);
    CGFloat height  = std::min(mode.size.y, desktop.size.y);
    CGFloat x       = (desktop.size.x - width) / 2.0;
    CGFloat y       = (desktop.size.y - height) / 2.0;
    NSRect  oglRect = NSMakeRect(x, y, width, height);

    m_oglView = [[SFOpenGLView alloc] initWithFrame:oglRect fullscreen:YES highDpi:m_highDpi];

    if (m_oglView == nil)
    {
        sf::err() << "Could not create an instance of NSOpenGLView "
                  << "in -[SFWindowController setupFullscreenViewWithMode:]." << std::endl;
        return;
    }

    // Populate the window and views
    [masterView addSubview:m_oglView];
    [m_window setContentView:masterView];
}


////////////////////////////////////////////////////////
- (void)setupWindowWithMode:(const sf::VideoMode&)mode andStyle:(std::uint32_t)style
{
    // We know that sf::State is not Fullscreen

    // Create our window size.
    NSRect rect = NSMakeRect(0, 0, mode.size.x, mode.size.y);

    // Convert the SFML window style to Cocoa window style.
    unsigned int nsStyle = NSBorderlessWindowMask;
    if (style & sf::Style::Titlebar)
        nsStyle |= NSTitledWindowMask | NSMiniaturizableWindowMask;
    if (style & sf::Style::Resize)
        nsStyle |= NSResizableWindowMask;
    if (style & sf::Style::Close)
        nsStyle |= NSClosableWindowMask;

    // Create the window.
    m_window = [[SFWindow alloc]
        initWithContentRect:rect
                  styleMask:nsStyle
                    backing:NSBackingStoreBuffered
                      defer:NO]; // Don't defer it!
    /*
     "YES" produces some "invalid drawable".
     See http://www.cocoabuilder.com/archive/cocoa/152482-nsviews-and-nsopenglcontext-invalid-drawable-error.html

     [...]
     As best as I can figure, this is happening because the NSWindow (and
     hence my view) are not visible on screen yet, and the system doesn't like that.
     [...]
     */

    if (m_window == nil)
    {
        sf::err() << "Could not create an instance of NSWindow "
                  << "in -[SFWindowController setupWindowWithMode:andStyle:]." << std::endl;

        return;
    }

    // Create the view.
    m_oglView = [[SFOpenGLView alloc] initWithFrame:[[m_window contentView] frame] fullscreen:NO highDpi:m_highDpi];

    if (m_oglView == nil)
    {
        sf::err() << "Could not create an instance of NSOpenGLView "
                  << "in -[SFWindowController setupWindowWithMode:andStyle:]." << std::endl;

        return;
    }

    // Set the view to the window as its content view.
    [m_window setContentView:m_oglView];

    // Register for event.
    [m_window setDelegate:self];
    [m_window setAcceptsMouseMovedEvents:YES];
    [m_window setIgnoresMouseEvents:NO];

    // And some other things...
    [m_window center];
    [m_window setAutodisplay:YES];
    [m_window setReleasedWhenClosed:NO]; // We own the class, not AppKit
}


////////////////////////////////////////////////////////
- (void)dealloc
{
    [self closeWindow];
    [NSMenu setMenuBarVisible:YES];

    [m_window release];
    [m_oglView release];

    [super dealloc];
}


#pragma mark
#pragma mark WindowImplDelegateProtocol's methods


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
    return m_window;
}


////////////////////////////////////////////////////////
- (BOOL)isMouseInside
{
    return [m_oglView isMouseInside];
}


////////////////////////////////////////////////////////
- (void)setCursorGrabbed:(BOOL)grabbed
{
    // Remove or restore resizeable style if needed
    BOOL resizeable = (([m_window styleMask] & NSResizableWindowMask) != 0) ? YES : NO;
    if (grabbed && resizeable)
    {
        m_restoreResize     = YES;
        NSUInteger newStyle = [m_window styleMask] & ~NSResizableWindowMask;
        [m_window setStyleMask:newStyle];
    }
    else if (!grabbed && m_restoreResize)
    {
        m_restoreResize     = NO;
        NSUInteger newStyle = [m_window styleMask] | NSResizableWindowMask;
        [m_window setStyleMask:newStyle];
    }

    // Forward to our view
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
    // Note: since 10.7 the conversion API works with NSRect
    // instead of NSPoint. Therefore we use a NSRect but ignore
    // its width and height.

    // Position of the bottom-left corner in the different coordinate systems:
    NSRect corner = [m_oglView frame]; // bottom left; size is ignored
    NSRect view   = [m_oglView convertRectToBacking:corner];
    NSRect window = [m_oglView convertRect:view toView:nil];
    NSRect screen = [[m_oglView window] convertRectToScreen:window];

    // Get the top-left corner in screen coordinates
    CGFloat x = screen.origin.x;
    CGFloat y = screen.origin.y + [m_oglView frame].size.height;

    // Flip y-axis (titlebar was already taken into account above)
    y = static_cast<double>([self screenHeight]) - y;

    return NSMakePoint(x, y);
}


////////////////////////////////////////////////////////
- (void)setWindowPositionTo:(sf::Vector2i)position
{
    NSPoint point = NSMakePoint(position.x, position.y);

    // Flip for SFML window coordinate system and take titlebar into account
    point.y = static_cast<double>([self screenHeight]) - point.y + static_cast<double>([self titlebarHeight]);

    // Place the window.
    [m_window setFrameTopLeftPoint:point];

    // In case the cursor was grabbed we need to update its position
    [m_oglView updateCursorGrabbed];
}


////////////////////////////////////////////////////////
- (NSSize)size
{
    return [m_oglView frame].size;
}


////////////////////////////////////////////////////////
- (void)resizeTo:(sf::Vector2u)size
{
    if (m_fullscreen)
    {
        // Special case when fullscreen: only resize the opengl view
        // and make sure the requested size is not bigger than the window.
        const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

        size.x = std::min(size.x, desktop.size.x);
        size.y = std::min(size.y, desktop.size.y);

        const auto origin  = sf::Vector2<CGFloat>(desktop.size - size) / CGFloat{2};
        NSRect     oglRect = NSMakeRect(origin.x, origin.y, size.x, size.y);

        [m_oglView setFrame:oglRect];
        [m_oglView setNeedsDisplay:YES];
    }
    else
    {
        // Before resizing, remove resizable mask to be able to resize
        // beyond the desktop boundaries.
        NSUInteger styleMask = [m_window styleMask];

        [m_window setStyleMask:styleMask ^ NSResizableWindowMask];

        // Add titlebar height.
        size.y += static_cast<unsigned int>([self titlebarHeight]);

        // Corner case: don't set the window height bigger than the screen height
        // or the view will be resized _later_ without generating a resize event.
        NSRect  screenFrame      = [[NSScreen mainScreen] visibleFrame];
        CGFloat maxVisibleHeight = screenFrame.size.height;
        if (size.y > maxVisibleHeight)
            size.y = static_cast<unsigned int>(maxVisibleHeight);

        if (m_requester != nil)
            m_requester->windowResized({size.x, size.y - static_cast<unsigned int>([self titlebarHeight])});

        NSRect frame = NSMakeRect([m_window frame].origin.x, [m_window frame].origin.y, size.x, size.y);

        [m_window setFrame:frame display:YES];

        // And restore the mask
        [m_window setStyleMask:styleMask];
    }
}


////////////////////////////////////////////////////////
- (void)setMinimumSize:(NSSize)size
{
    [m_window setContentMinSize:size];
}


////////////////////////////////////////////////////////
- (void)setMaximumSize:(NSSize)size
{
    [m_window setContentMaxSize:size];
}


////////////////////////////////////////////////////////
- (void)changeTitle:(NSString*)title
{
    [m_window setTitle:title];
}


////////////////////////////////////////////////////////
- (void)hideWindow
{
    [m_window orderOut:nil];
}


////////////////////////////////////////////////////////
- (void)showWindow
{
    [m_window makeKeyAndOrderFront:nil];
}


////////////////////////////////////////////////////////
- (void)closeWindow
{
    [self applyContext:nil];
    [m_window close];
    [m_window setDelegate:nil];
    [self setRequesterTo:nil];
    [SFApplication processEvent];
}


////////////////////////////////////////////////////////
- (void)requestFocus
{
    [m_window makeKeyAndOrderFront:nil];

    // In case the app is not active, make its dock icon bounce for one sec
    [NSApp requestUserAttention:NSInformationalRequest];
}


////////////////////////////////////////////////////////////
- (BOOL)hasFocus
{
    return [NSApp keyWindow] == m_window;
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
    // Load image and set app icon.
    NSImage* icon = [NSImage imageWithRawData:pixels andSize:NSMakeSize(size.x, size.y)];

    [[SFApplication sharedApplication] setApplicationIconImage:icon];

    [icon release];
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
    if (m_requester != nil)
        [SFApplication processEvent];
}


////////////////////////////////////////////////////////
- (void)applyContext:(NSOpenGLContext*)context
{
    [m_oglView setOpenGLContext:context];
    [context setView:m_oglView];
}


#pragma mark
#pragma mark NSWindowDelegate's methods


////////////////////////////////////////////////////////
- (BOOL)windowShouldClose:(id)sender
{
    (void)sender;

    if (m_requester == nil)
        return YES;

    m_requester->windowClosed();
    return NO;
}


#pragma mark
#pragma mark Other methods

////////////////////////////////////////////////////////
- (float)screenHeight
{
    NSDictionary* deviceDescription = [[m_window screen] deviceDescription];
    NSNumber*     screenNumber      = [deviceDescription valueForKey:@"NSScreenNumber"];
    auto          screenID          = static_cast<CGDirectDisplayID>([screenNumber intValue]);
    return static_cast<float>(CGDisplayPixelsHigh(screenID));
}


////////////////////////////////////////////////////////
- (float)titlebarHeight
{
    return static_cast<float>(NSHeight([m_window frame]) - NSHeight([[m_window contentView] frame]));
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
