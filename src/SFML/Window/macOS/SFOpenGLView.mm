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
#import <SFML/Window/macOS/SFOpenGLView+mouse_priv.h>
#import <SFML/Window/macOS/SFOpenGLView.h>
#import <SFML/Window/macOS/SFSilentResponder.h>
#include <SFML/Window/macOS/WindowImplCocoa.hpp>

#include <SFML/System/Err.hpp>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"


////////////////////////////////////////////////////////////
/// SFOpenGLView class: Privates Methods Declaration
///
////////////////////////////////////////////////////////////
@interface SFOpenGLView ()

////////////////////////////////////////////////////////////
/// \brief Handle screen changed event
///
////////////////////////////////////////////////////////////
- (void)updateScaleFactor;

////////////////////////////////////////////////////////////
/// \brief Handle view resized event
///
////////////////////////////////////////////////////////////
- (void)viewDidEndLiveResize;

////////////////////////////////////////////////////////////
/// \brief Callback for focus event
///
////////////////////////////////////////////////////////////
- (void)windowDidBecomeKey:(NSNotification*)notification;

////////////////////////////////////////////////////////////
/// \brief Callback for unfocus event
///
////////////////////////////////////////////////////////////
- (void)windowDidResignKey:(NSNotification*)notification;

////////////////////////////////////////////////////////////
/// \brief Handle going in fullscreen mode
///
////////////////////////////////////////////////////////////
- (void)enterFullscreen;

////////////////////////////////////////////////////////////
/// \brief Handle exiting fullscreen mode
///
////////////////////////////////////////////////////////////
- (void)exitFullscreen;

@end

@implementation SFOpenGLView

#pragma mark
#pragma mark SFOpenGLView's methods

////////////////////////////////////////////////////////
- (id)initWithFrame:(NSRect)frameRect
{
    return [self initWithFrame:frameRect fullscreen:NO highDpi:NO];
}

////////////////////////////////////////////////////////
- (id)initWithFrame:(NSRect)frameRect fullscreen:(BOOL)isFullscreen
{
    return [self initWithFrame:frameRect fullscreen:isFullscreen highDpi:NO];
}

////////////////////////////////////////////////////////
- (id)initWithFrame:(NSRect)frameRect fullscreen:(BOOL)isFullscreen highDpi:(BOOL)isHighDpi
{
    if ((self = [super initWithFrame:frameRect]))
    {
        [self setRequesterTo:nullptr];
        [self enableKeyRepeat];

        // Register for mouse move event
        m_mouseIsIn     = [self isMouseInside];
        NSUInteger opts = (NSTrackingActiveAlways | NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
                           NSTrackingEnabledDuringMouseDrag);
        m_trackingArea  = [[NSTrackingArea alloc] initWithRect:[self bounds] options:opts owner:self userInfo:nil];
        [self addTrackingArea:m_trackingArea];

        m_fullscreen    = isFullscreen;
        m_scaleFactor   = 1.0; // Default value; it will be updated in finishInit
        m_cursorGrabbed = NO;
        m_deltaXBuffer  = 0;
        m_deltaYBuffer  = 0;
        m_cursor        = [NSCursor arrowCursor];

        // Create a hidden text view for parsing key down event properly
        m_silentResponder = [[SFSilentResponder alloc] init];
        m_hiddenTextView  = [[NSTextView alloc] initWithFrame:NSZeroRect];
        [m_hiddenTextView setNextResponder:m_silentResponder];

        // If high DPI is requested, then use high resolution for the OpenGL view.
        // Currently, isHighDpi is always expected to be NO, and so the OpenGL view will render scaled.
        // Note: setWantsBestResolutionOpenGLSurface requires YES to work properly, rather than simply non-zero value.
        // isHighDpi is an Objective-C BOOL, which can have non-zero values other than YES.
        m_highDpi = isHighDpi ? YES : NO;
        [self setWantsBestResolutionOpenGLSurface:m_highDpi];

        // At that point, the view isn't attached to a window. We defer the rest of
        // the initialization process to later.
    }

    return self;
}


////////////////////////////////////////////////////////
- (void)update
{
    // In order to prevent an infinite recursion when the window/view is
    // resized to zero-height/width, we ignore update event when resizing.
    if (![self inLiveResize])
    {
        [super update];
    }
}


////////////////////////////////////////////////////////
- (void)finishInit
{
    // Register for window focus events
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(windowDidBecomeKey:)
               name:NSWindowDidBecomeKeyNotification
             object:[self window]];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(windowDidResignKey:)
               name:NSWindowDidResignKeyNotification
             object:[self window]];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(windowDidResignKey:)
               name:NSWindowWillCloseNotification
             object:[self window]];

    // Register for changed screen and changed screen's profile events
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(updateScaleFactor)
               name:NSWindowDidChangeScreenNotification
             object:[self window]];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(updateScaleFactor)
               name:NSWindowDidChangeScreenProfileNotification
             object:[self window]];

    // Now that we have a window, set up correctly the scale factor and cursor grabbing
    [self updateScaleFactor];
    [self updateCursorGrabbed]; // update for fullscreen

    // Register the drag types that this view can accept
    //[self registerForDraggedTypes:[NSArray arrayWithObjects:NSFilenamesPboardType, nil]];
}


////////////////////////////////////////////////////////
- (void)setRequesterTo:(sf::priv::WindowImplCocoa*)requester
{
    m_requester = requester;
}


////////////////////////////////////////////////////////
- (NSPoint)convertPointToScreen:(NSPoint)point
{
    NSRect rect = NSZeroRect;
    rect.origin = point;
    rect        = [[self window] convertRectToScreen:rect];
    return rect.origin;
}


////////////////////////////////////////////////////////
- (NSPoint)computeGlobalPositionOfRelativePoint:(NSPoint)point
{
    // Flip SFML coordinates to match window coordinates
    point.y = [self frame].size.height - point.y;

    // Get the position of (x, y) in the coordinate system of the window.
    point = [self convertPoint:point toView:self];
    point = [self convertPoint:point toView:nil]; // nil means window

    // Convert it to screen coordinates
    point = [self convertPointToScreen:point];

    // Flip screen coordinates to match CGDisplayMoveCursorToPoint referential.
    const double screenHeight = [[[self window] screen] frame].size.height;
    point.y                   = screenHeight - point.y;

    return point;
}


////////////////////////////////////////////////////////
- (CGFloat)displayScaleFactor
{
    return m_scaleFactor;
}


////////////////////////////////////////////////////////
- (void)updateScaleFactor
{
    NSWindow* window         = [self window];
    NSScreen* screen         = window ? [window screen] : [NSScreen mainScreen];
    CGFloat   oldScaleFactor = m_scaleFactor;
    m_scaleFactor            = m_highDpi ? [screen backingScaleFactor] : 1.0;

    // Send a resize event if the scaling factor changed
    if ((m_scaleFactor != oldScaleFactor) && (m_requester != nullptr))
    {
        NSSize newSize = [self frame].size;
        m_requester->windowResized({static_cast<unsigned int>(newSize.width), static_cast<unsigned int>(newSize.height)});
    }
}


////////////////////////////////////////////////////////
- (void)viewDidEndLiveResize
{
    // We use viewDidEndLiveResize to notify the user ONCE
    // only, when the resizing is finished.
    // In a perfect world we would like to notify the user
    // in live that the window is being resized. However,
    // it seems impossible to forward to the user
    // NSViewFrameDidChangeNotification before the resizing
    // is done. Several notifications are emitted but they
    // are all delivered after when the work is done.

    [super viewDidEndLiveResize];

    // Update mouse internal state.
    [self updateMouseState];
    [self updateCursorGrabbed];

    // Update the OGL view to fit the new size.
    [self update];

    // Send an event
    if (m_requester == nullptr)
        return;

    // The new size
    NSSize newSize = [self frame].size;
    m_requester->windowResized({static_cast<unsigned int>(newSize.width), static_cast<unsigned int>(newSize.height)});
}

////////////////////////////////////////////////////////
- (void)windowDidBecomeKey:(NSNotification*)notification
{
    (void)notification;

    [self updateCursorGrabbed];

    if (m_requester)
        m_requester->windowFocusGained();

    if (m_fullscreen)
        [self enterFullscreen];
}


////////////////////////////////////////////////////////
- (void)windowDidResignKey:(NSNotification*)notification
{
    (void)notification;

    [self updateCursorGrabbed];

    if (m_requester)
        m_requester->windowFocusLost();

    if (m_fullscreen)
        [self exitFullscreen];
}


////////////////////////////////////////////////////////
- (void)enterFullscreen
{
    // Remove the tracking area first,
    // just to be sure we don't add it twice!
    [self removeTrackingArea:m_trackingArea];
    [self addTrackingArea:m_trackingArea];

    // Fire an mouse entered event if needed
    if (!m_mouseIsIn && (m_requester != nullptr))
        m_requester->mouseMovedIn();

    // Update status
    m_mouseIsIn = YES;
}


////////////////////////////////////////////////////////
- (void)exitFullscreen
{
    [self removeTrackingArea:m_trackingArea];

    // Fire an mouse left event if needed
    if (m_mouseIsIn && (m_requester != nullptr))
        m_requester->mouseMovedOut();

    // Update status
    m_mouseIsIn = NO;
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
    // Check if what is being dragged is a file
    if ([sender.draggingPasteboard.types containsObject:NSFilenamesPboardType])
    {
        // Make sure that we can get the filename (not just the contents)
        if (sender.draggingSourceOperationMask & NSDragOperationLink)
        {
            // Return that we can accept a filename
            return NSDragOperationLink;
        }
    }

    // Return that we can't accept this file
    return NSDragOperationNone;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender
{
    // Make sure that the dragged item is a file
    if ([sender.draggingPasteboard.types containsObject:NSFilenamesPboardType])
    {
        // Get the filenames from the sender
        NSArray* files = [sender.draggingPasteboard propertyListForType:NSFilenamesPboardType];

        // Convert to std::vector<std::string>

        std::vector<sf::String> filenames;

        for (NSUInteger i = 0; i < [files count]; i++)
        {
            NSObject* item = files[i];

            // Make sure that we can cast the NSObject*
            if ([item isKindOfClass:[NSMutableString class]])
            {
                auto* filename = reinterpret_cast<NSMutableString*>(item);

                filenames.emplace_back([filename UTF8String]);
            }
        }

        NSPoint mousePosition = [self cursorPositionFromEvent:nil];

        if (m_requester != nullptr)
            m_requester->handleFileDroppingEvent(filenames, sf::Vector2i{int(mousePosition.x), int(mousePosition.y)});
    }
    else
    {
        // If somehow we get a drag operation for an item we cant accept, return no
        return NO;
    }

    // Everything went well, return yes
    return YES;
}

#pragma mark
#pragma mark Subclassing methods


////////////////////////////////////////////////////////
- (void)dealloc
{
    // Unregister for window focus events
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    // Unregister
    [self removeTrackingArea:m_trackingArea];

    // Release attributes
    [m_hiddenTextView release];
    [m_silentResponder release];
    [m_trackingArea release];

    [self setRequesterTo:nullptr];

    [super dealloc];
}


@end
