////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2024 Laurent Gomila (laurent@sfml-dev.org)
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
#include <SFML/Window/WindowEnums.hpp> // Prevent conflict with macro None from Xlib
#include <SFML/Window/WindowImpl.hpp>

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

#include <deque>
#include <memory>


namespace sf::priv
{
////////////////////////////////////////////////////////////
/// \brief Linux (X11) implementation of WindowImpl
///
////////////////////////////////////////////////////////////
class WindowImplX11 : public WindowImpl
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Construct the window implementation from an existing control
    ///
    /// \param handle Platform-specific handle of the control
    ///
    ////////////////////////////////////////////////////////////
    WindowImplX11(WindowHandle handle);

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
    WindowImplX11(VideoMode mode, const String& title, std::uint32_t style, State state, const ContextSettings& settings);

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~WindowImplX11() override;

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
    /// \param visible True to show, false to hide
    ///
    ////////////////////////////////////////////////////////////
    void setVisible(bool visible) override;

    ////////////////////////////////////////////////////////////
    /// \brief Show or hide the mouse cursor
    ///
    /// \param visible True to show, false to hide
    ///
    ////////////////////////////////////////////////////////////
    void setMouseCursorVisible(bool visible) override;

    ////////////////////////////////////////////////////////////
    /// \brief Grab or release the mouse cursor
    ///
    /// \param grabbed True to enable, false to disable
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
    /// \param enabled True to enable, false to disable
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
    /// \brief Enable or disable file dropping.
    ///
    /// \param enabled True to enable, false to disable
    ///
    ////////////////////////////////////////////////////////////
    void setFileDroppingEnabled(bool enabled = true) override;

    ////////////////////////////////////////////////////////////
    /// \brief Check whether the window has the input focus
    ///
    /// \return True if window has focus, false otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool hasFocus() const override;

protected:
    ////////////////////////////////////////////////////////////
    /// \brief Process incoming events from the operating system
    ///
    ////////////////////////////////////////////////////////////
    void processEvents() override;

    ////////////////////////////////////////////////////////////
    /// \brief Check if the given file type can be accepted
    ///
    /// \param fileType The file type to check
    ///
    /// \return If the file type is acceptable
    ///
    ////////////////////////////////////////////////////////////
    bool canAcceptFileType(const Atom& fileType);

private:
    ////////////////////////////////////////////////////////////
    /// \brief Request the WM to make the current window active
    ///
    ////////////////////////////////////////////////////////////
    void grabFocus();

    ////////////////////////////////////////////////////////////
    /// \brief Set fullscreen video mode
    ///
    /// \param Mode video mode to switch to
    ///
    ////////////////////////////////////////////////////////////
    void setVideoMode(const VideoMode& mode);

    ////////////////////////////////////////////////////////////
    /// \brief Reset to desktop video mode
    ///
    ////////////////////////////////////////////////////////////
    void resetVideoMode();

    ////////////////////////////////////////////////////////////
    /// \brief Switch to fullscreen mode
    ///
    ////////////////////////////////////////////////////////////
    void switchToFullscreen();

    ////////////////////////////////////////////////////////////
    /// \brief Set the WM protocols we support
    ///
    ////////////////////////////////////////////////////////////
    void setProtocols();

    ////////////////////////////////////////////////////////////
    /// \brief Update the last time we received user input
    ///
    /// \param time Last time we received user input
    ///
    ////////////////////////////////////////////////////////////
    void updateLastInputTime(::Time time);

    ////////////////////////////////////////////////////////////
    /// \brief Do some common initializations after the window has been created
    ///
    ////////////////////////////////////////////////////////////
    void initialize();

    ////////////////////////////////////////////////////////////
    /// \brief Create a transparent mouse cursor
    ///
    ////////////////////////////////////////////////////////////
    void createHiddenCursor();

    ////////////////////////////////////////////////////////////
    /// \brief Cleanup graphical resources attached to the window
    ///
    ////////////////////////////////////////////////////////////
    void cleanup();

    ////////////////////////////////////////////////////////////
    /// \brief Process an incoming event from the window
    ///
    /// \param windowEvent Event which has been received
    ///
    /// \return True if the event was processed, false if it was discarded
    ///
    ////////////////////////////////////////////////////////////
    bool processEvent(XEvent& windowEvent);

    ////////////////////////////////////////////////////////////
    /// \brief Check if a valid version of XRandR extension is present
    ///
    /// \return True if a valid XRandR version found, false otherwise
    ///
    ////////////////////////////////////////////////////////////
    bool checkXRandR();

    ////////////////////////////////////////////////////////////
    /// \brief Get the RROutput of the primary monitor
    ///
    /// \param rootWindow the root window
    /// \param res screen resources
    ///
    /// \return RROutput of the primary monitor
    ///
    ////////////////////////////////////////////////////////////
    RROutput getOutputPrimary(::Window& rootWindow, XRRScreenResources* res);

    ////////////////////////////////////////////////////////////
    /// \brief Get coordinates of the primary monitor
    ///
    /// \return Position of the primary monitor
    ///
    ////////////////////////////////////////////////////////////
    Vector2i getPrimaryMonitorPosition();

    ////////////////////////////////////////////////////////////
    /// \brief Set min/max window size
    ///
    ////////////////////////////////////////////////////////////
    void setWindowSizeConstraints() const;

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    ::Window                 m_window{};       ///< X identifier defining our window
    std::shared_ptr<Display> m_display;        ///< Pointer to the display
    int                      m_screen;         ///< Screen identifier
    std::shared_ptr<_XIM>    m_inputMethod;    ///< Input method linked to the X display
    XIC                      m_inputContext{}; ///< Input context used to get unicode input in our window
    bool                     m_isExternal{};   ///< Tell whether the window has been created externally or by SFML
    RRMode                   m_oldVideoMode{}; ///< Video mode in use before we switch to fullscreen
    RRCrtc                   m_oldRRCrtc{};    ///< RRCrtc in use before we switch to fullscreen
    ::Cursor m_hiddenCursor{}; ///< As X11 doesn't provide cursor hiding, we must create a transparent one
    ::Cursor m_lastCursor{None}; ///< Last cursor used -- this data is not owned by the window and is required to be always valid
    bool     m_keyRepeat{true}; ///< Is the KeyRepeat feature enabled?
    Vector2i m_previousSize{-1, -1}; ///< Previous size of the window, to find if a ConfigureNotify event is a resize event (could be a move event only)
    bool     m_useSizeHints{};   ///< Is the size of the window fixed with size hints?
    bool     m_fullscreen{};     ///< Is the window in fullscreen?
    bool     m_cursorGrabbed{};  ///< Is the mouse cursor trapped?
    bool     m_windowMapped{};   ///< Has the window been mapped by the window manager?
    Pixmap   m_iconPixmap{};     ///< The current icon pixmap if in use
    Pixmap   m_iconMaskPixmap{}; ///< The current icon mask pixmap if in use
    ::Time   m_lastInputTime{};  ///< Last time we received user input
    ::Window m_dropSource{0};    ///< The window which is giving the dropped item
    Atom m_acceptedFileType{0}; ///< The MIME type that the other window supports that we also support for file dropping
};

} // namespace sf::priv
