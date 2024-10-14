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
#include <SFML/Window/Export.hpp>

#include <SFML/Window/Vulkan.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <SFML/Window/WindowHandle.hpp>

#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

#include <memory>
#include <optional>

#include <cstdint>


namespace sf
{
class Cursor;
class String;
class VideoMode;

namespace priv
{
class WindowImpl;
}

class Event;

////////////////////////////////////////////////////////////
/// \brief Window that serves as a base for other windows
///
////////////////////////////////////////////////////////////
class SFML_WINDOW_API WindowBase
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor
    ///
    /// This constructor doesn't actually create the window,
    /// use the other constructors or call `create()` to do so.
    ///
    ////////////////////////////////////////////////////////////
    WindowBase();

    ////////////////////////////////////////////////////////////
    /// \brief Construct a new window
    ///
    /// This constructor creates the window with the size and pixel
    /// depth defined in \a `mode`. An optional style can be passed to
    /// customize the look and behavior of the window (borders,
    /// title bar, resizable, closable, ...). An optional state can
    /// be provided. If \a `state` is `State::Fullscreen`, then \a `mode`
    /// must be a valid video mode.
    ///
    /// \param mode  Video mode to use (defines the width, height and depth of the rendering area of the window)
    /// \param title Title of the window
    /// \param style %Window style, a bitwise OR combination of `sf::Style` enumerators
    /// \param state %Window state
    ///
    ////////////////////////////////////////////////////////////
    WindowBase(VideoMode mode, const String& title, std::uint32_t style = Style::Default, State state = State::Windowed);

    ////////////////////////////////////////////////////////////
    /// \brief Construct a new window
    ///
    /// This constructor creates the window with the size and pixel
    /// depth defined in \a `mode`.
    ///
    /// \param mode  Video mode to use (defines the width, height and depth of the rendering area of the window)
    /// \param title Title of the window
    /// \param state %Window state
    ///
    ////////////////////////////////////////////////////////////
    WindowBase(VideoMode mode, const String& title, State state);

    ////////////////////////////////////////////////////////////
    /// \brief Construct the window from an existing control
    ///
    /// \param handle Platform-specific handle of the control
    ///
    ////////////////////////////////////////////////////////////
    explicit WindowBase(WindowHandle handle);

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    /// Closes the window and frees all the resources attached to it.
    ///
    ////////////////////////////////////////////////////////////
    virtual ~WindowBase();

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    WindowBase(const WindowBase&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    WindowBase& operator=(const WindowBase&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    WindowBase(WindowBase&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment
    ///
    ////////////////////////////////////////////////////////////
    WindowBase& operator=(WindowBase&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Create (or recreate) the window
    ///
    /// If the window was already created, it closes it first.
    /// If \a `state` is `State::Fullscreen`, then \a `mode` must be
    /// a valid video mode.
    ///
    /// \param mode  Video mode to use (defines the width, height and depth of the rendering area of the window)
    /// \param title Title of the window
    /// \param style %Window style, a bitwise OR combination of `sf::Style` enumerators
    /// \param state %Window state
    ///
    ////////////////////////////////////////////////////////////
    virtual void create(VideoMode mode, const String& title, std::uint32_t style = Style::Default, State state = State::Windowed);

    ////////////////////////////////////////////////////////////
    /// \brief Create (or recreate) the window
    ///
    /// If the window was already created, it closes it first.
    /// If \a `state` is `State::Fullscreen`, then \a `mode` must be
    /// a valid video mode.
    ///
    /// \param mode  Video mode to use (defines the width, height and depth of the rendering area of the window)
    /// \param title Title of the window
    /// \param state %Window state
    ///
    ////////////////////////////////////////////////////////////
    virtual void create(VideoMode mode, const String& title, State state);

    ////////////////////////////////////////////////////////////
    /// \brief Create (or recreate) the window from an existing control
    ///
    /// \param handle Platform-specific handle of the control
    ///
    ////////////////////////////////////////////////////////////
    virtual void create(WindowHandle handle);

    ////////////////////////////////////////////////////////////
    /// \brief Close the window and destroy all the attached resources
    ///
    /// After calling this function, the `sf::Window` instance remains
    /// valid and you can call `create()` to recreate the window.
    /// All other functions such as `pollEvent()` or `display()` will
    /// still work (i.e. you don't have to test `isOpen()` every time),
    /// and will have no effect on closed windows.
    ///
    ////////////////////////////////////////////////////////////
    virtual void close();

    ////////////////////////////////////////////////////////////
    /// \brief Tell whether or not the window is open
    ///
    /// This function returns whether or not the window exists.
    /// Note that a hidden window (`setVisible(false)`) is open
    /// (therefore this function would return `true`).
    ///
    /// \return `true` if the window is open, `false` if it has been closed
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isOpen() const;

    ////////////////////////////////////////////////////////////
    /// \brief Pop the next event from the front of the FIFO event queue, if any, and return it
    ///
    /// This function is not blocking: if there's no pending event then
    /// it will return a `std::nullopt`. Note that more than one event
    /// may be present in the event queue, thus you should always call
    /// this function in a loop to make sure that you process every
    /// pending event.
    /// \code
    /// while (const std::optional event = window.pollEvent())
    /// {
    ///    // process event...
    /// }
    /// \endcode
    ///
    /// \return The event, otherwise `std::nullopt` if no events are pending
    ///
    /// \see `waitEvent`, `handleEvents`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] std::optional<Event> pollEvent();

    ////////////////////////////////////////////////////////////
    /// \brief Wait for an event and return it
    ///
    /// This function is blocking: if there's no pending event then
    /// it will wait until an event is received or until the provided
    /// timeout elapses. Only if an error or a timeout occurs the
    /// returned event will be `std::nullopt`.
    /// This function is typically used when you have a thread that is
    /// dedicated to events handling: you want to make this thread sleep
    /// as long as no new event is received.
    /// \code
    /// while (const std::optional event = window.waitEvent())
    /// {
    ///    // process event...
    /// }
    /// \endcode
    ///
    /// \param timeout Maximum time to wait (`Time::Zero` for infinite)
    ///
    /// \return The event, otherwise `std::nullopt` on timeout or if window was closed
    ///
    /// \see `pollEvent`, `handleEvents`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] std::optional<Event> waitEvent(Time timeout = Time::Zero);

    ////////////////////////////////////////////////////////////
    /// \brief Handle all pending events
    ///
    /// This function is not blocking: if there's no pending event then
    /// it will return without calling any of the handlers.
    ///
    /// This function can take a variadic list of event handlers that
    /// each take a concrete event type as a single parameter. The event
    /// handlers can be any kind of callable object that has an
    /// `operator()` defined for a specific event type. Additionally a
    /// generic callable can also be provided that will be invoked for
    /// every event type. If both types of callables are provided, the
    /// callables taking concrete event types will be preferred over the
    /// generic callable by overload resolution. Generic callables can
    /// be used to customize handler dispatching based on the deduced
    /// type of the event and other information available at compile
    /// time.
    ///
    /// Examples of callables:
    /// - Lambda expressions: `[&](const sf::Event::KeyPressed) { ... }`
    /// - Free functions: `void handler(const sf::Event::KeyPressed&) { ... }`
    ///
    /// \code
    /// // Only provide handlers for concrete event types
    /// window.handleEvents(
    ///     [&](const sf::Event::Closed&) { /* handle event */ },
    ///     [&](const sf::Event::KeyPressed& keyPress) { /* handle event */ }
    /// );
    /// \endcode
    /// \code
    /// // Provide a generic event handler
    /// window.handleEvents(
    ///     [&](const auto& event)
    ///     {
    ///         if constexpr (std::is_same_v<std::decay_t<decltype(event)>, sf::Event::Closed>)
    ///         {
    ///             // Handle Closed
    ///             handleClosed();
    ///         }
    ///         else if constexpr (std::is_same_v<std::decay_t<decltype(event)>, sf::Event::KeyPressed>)
    ///         {
    ///             // Handle KeyPressed
    ///             handleKeyPressed(event);
    ///         }
    ///         else
    ///         {
    ///             // Handle non-KeyPressed
    ///             handleOtherEvents(event);
    ///         }
    ///     }
    /// );
    /// \endcode
    /// \code
    /// // Provide handlers for concrete types and fall back to generic handler
    /// window.handleEvents(
    ///     [&](const sf::Event::Closed&) { /* handle event */ },
    ///     [&](const sf::Event::KeyPressed& keyPress) { /* handle event */ },
    ///     [&](const auto& event) { /* handle all other events */ }
    /// );
    /// \endcode
    ///
    /// Calling member functions is supported through lambda
    /// expressions.
    /// \code
    /// // Provide a generic event handler
    /// window.handleEvents(
    ///     [this](const auto& event) { handle(event); }
    /// );
    /// \endcode
    ///
    /// \param handlers A variadic list of callables that take a specific event as their only parameter
    ///
    /// \see `waitEvent`, `pollEvent`
    ///
    ////////////////////////////////////////////////////////////
    template <typename... Ts>
    void handleEvents(Ts&&... handlers);

    ////////////////////////////////////////////////////////////
    /// \brief Get the position of the window
    ///
    /// \return Position of the window, in pixels
    ///
    /// \see `setPosition`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vector2i getPosition() const;

    ////////////////////////////////////////////////////////////
    /// \brief Change the position of the window on screen
    ///
    /// This function only works for top-level windows
    /// (i.e. it will be ignored for windows created from
    /// the handle of a child window/control).
    ///
    /// \param position New position, in pixels
    ///
    /// \see `getPosition`
    ///
    ////////////////////////////////////////////////////////////
    void setPosition(Vector2i position);

    ////////////////////////////////////////////////////////////
    /// \brief Get the size of the rendering region of the window
    ///
    /// The size doesn't include the titlebar and borders
    /// of the window.
    ///
    /// \return Size in pixels
    ///
    /// \see `setSize`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vector2u getSize() const;

    ////////////////////////////////////////////////////////////
    /// \brief Change the size of the rendering region of the window
    ///
    /// \param size New size, in pixels
    ///
    /// \see `getSize`
    ///
    ////////////////////////////////////////////////////////////
    void setSize(Vector2u size);

    ////////////////////////////////////////////////////////////
    /// \brief Set the minimum window rendering region size
    ///
    /// Pass `std::nullopt` to unset the minimum size
    ///
    /// \param minimumSize New minimum size, in pixels
    ///
    ////////////////////////////////////////////////////////////
    void setMinimumSize(const std::optional<Vector2u>& minimumSize);

    ////////////////////////////////////////////////////////////
    /// \brief Set the maximum window rendering region size
    ///
    /// Pass `std::nullopt` to unset the maximum size
    ///
    /// \param maximumSize New maximum size, in pixels
    ///
    ////////////////////////////////////////////////////////////
    void setMaximumSize(const std::optional<Vector2u>& maximumSize);

    ////////////////////////////////////////////////////////////
    /// \brief Change the title of the window
    ///
    /// \param title New title
    ///
    /// \see `setIcon`
    ///
    ////////////////////////////////////////////////////////////
    void setTitle(const String& title);

    ////////////////////////////////////////////////////////////
    /// \brief Change the window's icon
    ///
    /// \a `pixels` must be an array of \a `size` pixels
    /// in 32-bits RGBA format.
    ///
    /// The OS default icon is used by default.
    ///
    /// \param size   Icon's width and height, in pixels
    /// \param pixels Pointer to the array of pixels in memory. The
    ///               pixels are copied, so you need not keep the
    ///               source alive after calling this function.
    ///
    /// \see `setTitle`
    ///
    ////////////////////////////////////////////////////////////
    void setIcon(Vector2u size, const std::uint8_t* pixels);

    ////////////////////////////////////////////////////////////
    /// \brief Show or hide the window
    ///
    /// The window is shown by default.
    ///
    /// \param visible `true` to show the window, `false` to hide it
    ///
    ////////////////////////////////////////////////////////////
    void setVisible(bool visible);

    ////////////////////////////////////////////////////////////
    /// \brief Show or hide the mouse cursor
    ///
    /// The mouse cursor is visible by default.
    ///
    /// \param visible `true` to show the mouse cursor, `false` to hide it
    ///
    ////////////////////////////////////////////////////////////
    void setMouseCursorVisible(bool visible);

    ////////////////////////////////////////////////////////////
    /// \brief Grab or release the mouse cursor
    ///
    /// If set, grabs the mouse cursor inside this window's client
    /// area so it may no longer be moved outside its bounds.
    /// Note that grabbing is only active while the window has
    /// focus.
    ///
    /// \param grabbed `true` to enable, `false` to disable
    ///
    ////////////////////////////////////////////////////////////
    void setMouseCursorGrabbed(bool grabbed);

    ////////////////////////////////////////////////////////////
    /// \brief Set the displayed cursor to a native system cursor
    ///
    /// Upon window creation, the arrow cursor is used by default.
    ///
    /// \warning The cursor must not be destroyed while in use by
    ///          the window.
    ///
    /// \warning Features related to Cursor are not supported on
    ///          iOS and Android.
    ///
    /// \param cursor Native system cursor type to display
    ///
    /// \see `sf::Cursor::createFromSystem`, `sf::Cursor::createFromPixels`
    ///
    ////////////////////////////////////////////////////////////
    void setMouseCursor(const Cursor& cursor);

    ////////////////////////////////////////////////////////////
    /// \brief Enable or disable automatic key-repeat
    ///
    /// If key repeat is enabled, you will receive repeated
    /// KeyPressed events while keeping a key pressed. If it is disabled,
    /// you will only get a single event when the key is pressed.
    ///
    /// Key repeat is enabled by default.
    ///
    /// \param enabled `true` to enable, `false` to disable
    ///
    ////////////////////////////////////////////////////////////
    void setKeyRepeatEnabled(bool enabled);

    ////////////////////////////////////////////////////////////
    /// \brief Change the joystick threshold
    ///
    /// The joystick threshold is the value below which
    /// no JoystickMoved event will be generated.
    ///
    /// The threshold value is 0.1 by default.
    ///
    /// \param threshold New threshold, in the range [0, 100]
    ///
    ////////////////////////////////////////////////////////////
    void setJoystickThreshold(float threshold);

    ////////////////////////////////////////////////////////////
    /// \brief Request the current window to be made the active
    ///        foreground window
    ///
    /// At any given time, only one window may have the input focus
    /// to receive input events such as keystrokes or mouse events.
    /// If a window requests focus, it only hints to the operating
    /// system, that it would like to be focused. The operating system
    /// is free to deny the request.
    /// This is not to be confused with `setActive()`.
    ///
    /// \see `hasFocus`
    ///
    ////////////////////////////////////////////////////////////
    void requestFocus();

    ////////////////////////////////////////////////////////////
    /// \brief Check whether the window has the input focus
    ///
    /// At any given time, only one window may have the input focus
    /// to receive input events such as keystrokes or most mouse
    /// events.
    ///
    /// \return `true` if window has focus, `false` otherwise
    /// \see `requestFocus`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool hasFocus() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the OS-specific handle of the window
    ///
    /// The type of the returned handle is `sf::WindowHandle`,
    /// which is a type alias to the handle type defined by the OS.
    /// You shouldn't need to use this function, unless you have
    /// very specific stuff to implement that SFML doesn't support,
    /// or implement a temporary workaround until a bug is fixed.
    ///
    /// \return System handle of the window
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] WindowHandle getNativeHandle() const;

    ////////////////////////////////////////////////////////////
    /// \brief Create a Vulkan rendering surface
    ///
    /// \param instance  Vulkan instance
    /// \param surface   Created surface
    /// \param allocator Allocator to use
    ///
    /// \return `true` if surface creation was successful, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool createVulkanSurface(const VkInstance&            instance,
                                           VkSurfaceKHR&                surface,
                                           const VkAllocationCallbacks* allocator = nullptr);

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
    void setFileDroppingEnabled(bool enabled = true);

protected:
    ////////////////////////////////////////////////////////////
    /// \brief Function called after the window has been created
    ///
    /// This function is called so that derived classes can
    /// perform their own specific initialization as soon as
    /// the window is created.
    ///
    ////////////////////////////////////////////////////////////
    virtual void onCreate();

    ////////////////////////////////////////////////////////////
    /// \brief Function called after the window has been resized
    ///
    /// This function is called so that derived classes can
    /// perform custom actions when the size of the window changes.
    ///
    ////////////////////////////////////////////////////////////
    virtual void onResize();

private:
    friend class Window;

    ////////////////////////////////////////////////////////////
    /// \brief Processes an event before it is sent to the user
    ///
    /// This function is called every time an event is received
    /// from the internal window (through `pollEvent` or `waitEvent`).
    /// It filters out unwanted events, and performs whatever internal
    /// stuff the window needs before the event is returned to the
    /// user.
    ///
    /// \param event Event to filter
    ///
    ////////////////////////////////////////////////////////////
    void filterEvent(const Event& event);

    ////////////////////////////////////////////////////////////
    /// \brief Perform some common internal initializations
    ///
    ////////////////////////////////////////////////////////////
    void initialize();

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    std::unique_ptr<priv::WindowImpl> m_impl; //!< Platform-specific implementation of the window
    Vector2u                          m_size; //!< Current size of the window
};

} // namespace sf

#include <SFML/Window/WindowBase.inl>

////////////////////////////////////////////////////////////
/// \class sf::WindowBase
/// \ingroup window
///
/// `sf::WindowBase` serves as the base class for all Windows.
///
/// A `sf::WindowBase` can create its own new window, or be embedded into
/// an already existing control using the `create(handle)` function.
///
/// The `sf::WindowBase` class provides a simple interface for manipulating
/// the window: move, resize, show/hide, control mouse cursor, etc.
/// It also provides event handling through its `pollEvent()` and `waitEvent()`
/// functions.
///
/// Usage example:
/// \code
/// // Declare and create a new window
/// sf::WindowBase window(sf::VideoMode({800, 600}), "SFML window");
///
/// // The main loop - ends as soon as the window is closed
/// while (window.isOpen())
/// {
///    // Event processing
///    while (const std::optional event = window.pollEvent())
///    {
///        // Request for closing the window
///        if (event->is<sf::Event::Closed>())
///            window.close();
///    }
///
///    // Do things with the window here...
/// }
/// \endcode
///
////////////////////////////////////////////////////////////
