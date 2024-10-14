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

#pragma once

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Window/Event.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <SFML/Window/WindowImpl.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

////////////////////////////////////////////////////////////
/// Predefine OBJ-C classes
////////////////////////////////////////////////////////////
#ifdef __OBJC__

#import <SFML/Window/macOS/WindowImplDelegateProtocol.h>
using WindowImplDelegateRef = id<WindowImplDelegateProtocol, NSObject>;

@class NSOpenGLContext;
using NSOpenGLContextRef = NSOpenGLContext*;

#else // If C++

using unichar = unsigned short; // See NSString.h

using WindowImplDelegateRef = void*;
using NSOpenGLContextRef    = void*;

#endif

namespace sf
{
class String;

namespace priv
{
////////////////////////////////////////////////////////////
/// \brief macOS (Cocoa) implementation of WindowImpl
///
////////////////////////////////////////////////////////////
class WindowImplCocoa : public WindowImpl
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Construct the window implementation from an existing control
    ///
    /// \param handle Platform-specific handle of the control
    ///
    ////////////////////////////////////////////////////////////
    WindowImplCocoa(WindowHandle handle);

    ////////////////////////////////////////////////////////////
    /// \brief Create the window implementation
    ///
    /// \param mode  Video mode to use
    /// \param title Title of the window
    /// \param style Window style
    /// \param state Window state
    /// \param settings Additional settings for the underlying OpenGL context
    ///
    ////////////////////////////////////////////////////////////
    WindowImplCocoa(VideoMode mode, const String& title, std::uint32_t style, State state, const ContextSettings& settings);

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~WindowImplCocoa() override;

    ////////////////////////////////////////////////////////////
    /// \brief Window Closed Event - called by the cocoa window object
    ///
    /// Send the event to SFML WindowImpl class.
    ///
    ////////////////////////////////////////////////////////////
    void windowClosed();

    ////////////////////////////////////////////////////////////
    /// \brief Window Resized Event - called by the cocoa window object
    ///
    /// Send the event to SFML WindowImpl class.
    ///
    /// \param size new width and height
    ///
    ////////////////////////////////////////////////////////////
    void windowResized(Vector2u size);

    ////////////////////////////////////////////////////////////
    /// \brief Window Lost Focus Event - called by the cocoa window object
    ///
    /// Send the event to SFML WindowImpl class.
    ///
    ////////////////////////////////////////////////////////////
    void windowFocusLost();

    ////////////////////////////////////////////////////////////
    /// \brief Window Get Focus Event - called by the cocoa window object
    ///
    /// Send the event to SFML WindowImpl class.
    ///
    ////////////////////////////////////////////////////////////
    void windowFocusGained();

    ////////////////////////////////////////////////////////////
    /// \brief Mouse Down Event - called by the cocoa view object
    ///
    /// Send the event to SFML WindowImpl class.
    ///
    /// \param button   active button
    /// \param position mouse x and y position
    ///
    ////////////////////////////////////////////////////////////
    void mouseDownAt(Mouse::Button button, Vector2i position);

    ////////////////////////////////////////////////////////////
    /// \brief Mouse Up Event - called by the cocoa view object
    ///
    /// Send the event to SFML WindowImpl class.
    ///
    /// \param button   active button
    /// \param position mouse x and y position
    ///
    ////////////////////////////////////////////////////////////
    void mouseUpAt(Mouse::Button button, Vector2i position);

    ////////////////////////////////////////////////////////////
    /// \brief Mouse Moved Event - called by the cocoa view object
    ///
    /// Send the event to SFML WindowImpl class.
    ///
    /// \param position mouse x and y position
    ///
    ////////////////////////////////////////////////////////////
    void mouseMovedAt(Vector2i position);

    ////////////////////////////////////////////////////////////
    /// \brief Mouse Wheel Scrolled Event - called by the cocoa view object
    ///
    /// Send the event to SFML WindowImpl class.
    ///
    /// \param deltaX   horizontal scrolling delta
    /// \param deltaY   vertical scrolling delta
    /// \param position mouse x and y position
    ///
    ////////////////////////////////////////////////////////////
    void mouseWheelScrolledAt(float deltaX, float deltaY, Vector2i position);

    ////////////////////////////////////////////////////////////
    /// \brief Mouse In Event - called by the cocoa view object
    ///
    /// Send the event to SFML WindowImpl class.
    ///
    ////////////////////////////////////////////////////////////
    void mouseMovedIn();

    ////////////////////////////////////////////////////////////
    /// \brief Mouse Out Event - called by the cocoa view object
    ///
    /// Send the event to SFML WindowImpl class.
    ///
    ////////////////////////////////////////////////////////////
    void mouseMovedOut();

    ////////////////////////////////////////////////////////////
    /// \brief Key Down Event - called by the cocoa view object
    ///
    /// Send the event to SFML WindowImpl class.
    ///
    /// \param event active key
    ///
    ////////////////////////////////////////////////////////////
    void keyDown(Event::KeyPressed event);

    ////////////////////////////////////////////////////////////
    /// \brief Key Up Event - called by the cocoa view object
    ///
    /// Send the event to SFML WindowImpl class.
    ///
    /// \param event active key
    ///
    ////////////////////////////////////////////////////////////
    void keyUp(Event::KeyReleased event);

    ////////////////////////////////////////////////////////////
    /// \brief Text Entred Event - called by the cocoa view object
    ///
    /// Send the event to SFML WindowImpl class.
    ///
    /// \param charcode Unicode input
    ///
    ////////////////////////////////////////////////////////////
    void textEntered(unichar charcode);

    ////////////////////////////////////////////////////////////
    /// \brief Apply the context to the view
    ///
    /// Called by the SFML context object to finalize its creation.
    ///
    /// \param context The context to bind to the window
    ///
    ////////////////////////////////////////////////////////////
    void applyContext(NSOpenGLContextRef context) const;

    ////////////////////////////////////////////////////////////
    /// \brief Handle the file dropping event
    ///
    /// Called by SFOpenGLView to handle the file dropping event
    ///
    /// \param files The files that were dropped
    ///
    ////////////////////////////////////////////////////////////
    void handleFileDroppingEvent(std::vector<sf::String> files, sf::Vector2i position);

    ////////////////////////////////////////////////////////////
    /// \brief Change the type of the current process
    ///
    /// The type of the process is changed to become a full GUI app.
    /// Also ensure NSApp is constructed.
    ///
    ////////////////////////////////////////////////////////////
    static void setUpProcess();

    ////////////////////////////////////////////////////////////
    /// \brief Get the OS-specific handle of the window
    ///
    /// \return Handle of the window
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] WindowHandle getNativeHandle() const override;

    ////////////////////////////////////////////////////////////
    /// \brief Get the position of the window
    ///
    /// \return Position of the window, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vector2i getPosition() const override;

    ////////////////////////////////////////////////////////////
    /// \brief Change the position of the window on screen
    ///
    /// \param position New position of the window, in pixels
    ///
    ////////////////////////////////////////////////////////////
    void setPosition(Vector2i position) override;

    ////////////////////////////////////////////////////////////
    /// \brief Get the client size of the window
    ///
    /// \return Size of the window, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vector2u getSize() const override;

    ////////////////////////////////////////////////////////////
    /// \brief Change the size of the rendering region of the window
    ///
    /// \param size New size, in pixels
    ///
    ////////////////////////////////////////////////////////////
    void setSize(Vector2u size) override;

    ////////////////////////////////////////////////////////////
    /// \brief Set the minimum window rendering region size
    ///
    /// Pass `std::nullopt` to unset the minimum size
    ///
    /// \param minimumSize New minimum size, in pixels
    ///
    ////////////////////////////////////////////////////////////
    void setMinimumSize(const std::optional<Vector2u>& minimumSize) override;

    ////////////////////////////////////////////////////////////
    /// \brief Set the maximum window rendering region size
    ///
    /// Pass `std::nullopt` to unset the maximum size
    ///
    /// \param maximumSize New maximum size, in pixels
    ///
    ////////////////////////////////////////////////////////////
    void setMaximumSize(const std::optional<Vector2u>& maximumSize) override;

    ////////////////////////////////////////////////////////////
    /// \brief Change the title of the window
    ///
    /// \param title New title
    ///
    ////////////////////////////////////////////////////////////
    void setTitle(const String& title) override;

    ////////////////////////////////////////////////////////////
    /// \brief Change the window's icon
    ///
    /// \param size   Icon's width and height, in pixels
    /// \param pixels Pointer to the pixels in memory, format must be RGBA 32 bits
    ///
    ////////////////////////////////////////////////////////////
    void setIcon(Vector2u size, const std::uint8_t* pixels) override;

    ////////////////////////////////////////////////////////////
    /// \brief Show or hide the window
    ///
    /// \param visible `true` to show, `false` to hide
    ///
    ////////////////////////////////////////////////////////////
    void setVisible(bool visible) override;

    ////////////////////////////////////////////////////////////
    /// \brief Show or hide the mouse cursor
    ///
    /// \param visible `true` to show, `false` to hide
    ///
    ////////////////////////////////////////////////////////////
    void setMouseCursorVisible(bool visible) override;

    ////////////////////////////////////////////////////////////
    /// \brief Grab or release the mouse cursor
    ///
    /// \param grabbed `true` to grab, `false` to release
    ///
    ////////////////////////////////////////////////////////////
    void setMouseCursorGrabbed(bool grabbed) override;

    ////////////////////////////////////////////////////////////
    /// \brief Set the displayed cursor to a native system cursor
    ///
    /// \param cursor Native system cursor type to display
    ///
    ////////////////////////////////////////////////////////////
    void setMouseCursor(const CursorImpl& cursor) override;

    ////////////////////////////////////////////////////////////
    /// \brief Enable or disable automatic key-repeat
    ///
    /// \param enabled `true` to enable, `false` to disable
    ///
    ////////////////////////////////////////////////////////////
    void setKeyRepeatEnabled(bool enabled) override;

    ////////////////////////////////////////////////////////////
    /// \brief Request the current window to be made the active
    ///        foreground window
    ///
    ////////////////////////////////////////////////////////////
    void requestFocus() override;

    ////////////////////////////////////////////////////////////
    /// \brief Check whether the window has the input focus
    ///
    /// \return `true` if window has focus, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool hasFocus() const override;

    ////////////////////////////////////////////////////////////
    /// \brief Enable or disable file dropping.
    ///
    /// If this is disabled, then when a user drags a file on to the window
    /// the file will be automatically denied. When this is enabled, the file
    /// will be accepted, no matter the type
    ///
    /// \param enabled True to enable, false to disable
    ///
    ////////////////////////////////////////////////////////////
    void setFileDroppingEnabled(bool enabled = true) override;

protected:
    ////////////////////////////////////////////////////////////
    /// \brief Process incoming events from the operating system
    ///
    ////////////////////////////////////////////////////////////
    void processEvents() override;

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    WindowImplDelegateRef m_delegate;         ///< Implementation in Obj-C.
    bool                  m_showCursor{true}; ///< Is the cursor displayed or hidden?
};

} // namespace priv

} // namespace sf

#pragma GCC diagnostic pop
