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
#include <SFML/Config.hpp>

#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/CursorImpl.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Sensor.hpp>
#include <SFML/Window/SensorImpl.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/Vulkan.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <SFML/Window/WindowHandle.hpp>

#include <SFML/System/EnumArray.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/System/Vector3.hpp>

#include <array>
#include <memory>
#include <optional>
#include <queue>

#include <cstdint>


namespace sf
{
class String;
class Time;

namespace priv
{
////////////////////////////////////////////////////////////
/// \brief Abstract base class for OS-specific window implementation
///
////////////////////////////////////////////////////////////
class WindowImpl
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Create a new window depending on the current OS
    ///
    /// \param mode  Video mode to use
    /// \param title Title of the window
    /// \param style Window style
    /// \param state Window state
    /// \param settings Additional settings for the underlying OpenGL context
    ///
    /// \return Pointer to the created window
    ///
    ////////////////////////////////////////////////////////////
    static std::unique_ptr<WindowImpl> create(VideoMode              mode,
                                              const String&          title,
                                              std::uint32_t          style,
                                              State                  state,
                                              const ContextSettings& settings);

    ////////////////////////////////////////////////////////////
    /// \brief Create a new window depending on to the current OS
    ///
    /// \param handle Platform-specific handle of the control
    ///
    /// \return Pointer to the created window
    ///
    ////////////////////////////////////////////////////////////
    static std::unique_ptr<WindowImpl> create(WindowHandle handle);

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    virtual ~WindowImpl();

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    WindowImpl(const WindowImpl&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    WindowImpl& operator=(const WindowImpl&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Change the joystick threshold, i.e. the value below which
    ///        no move event will be generated
    ///
    /// \param threshold New threshold, in range [0, 100]
    ///
    ////////////////////////////////////////////////////////////
    void setJoystickThreshold(float threshold);

    ////////////////////////////////////////////////////////////
    /// \brief Wait for and return the next available window event
    ///
    /// If there's no event available, this function calls the
    /// window's internal event processing function.
    ///
    /// \param timeout Maximum time to wait (`Time::Zero` for infinite)
    ///
    /// \return The event on success, `std::nullopt` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] std::optional<Event> waitEvent(Time timeout);

    ////////////////////////////////////////////////////////////
    /// \brief Return the next window event, if available
    ///
    /// If there's no event available, this function calls the
    /// window's internal event processing function.
    ///
    /// \return The event if available, `std::nullopt` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] std::optional<Event> pollEvent();

    ////////////////////////////////////////////////////////////
    /// \brief Get the OS-specific handle of the window
    ///
    /// \return Handle of the window
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] virtual WindowHandle getNativeHandle() const = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Get the position of the window
    ///
    /// \return Position of the window, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] virtual Vector2i getPosition() const = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Get the minimum window rendering region size
    ///
    /// \return Minimum size
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] std::optional<Vector2u> getMinimumSize() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the maximum window rendering region size
    ///
    /// \return Maximum size
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] std::optional<Vector2u> getMaximumSize() const;

    ////////////////////////////////////////////////////////////
    /// \brief Change the position of the window on screen
    ///
    /// \param position New position of the window, in pixels
    ///
    ////////////////////////////////////////////////////////////
    virtual void setPosition(Vector2i position) = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Get the client size of the window
    ///
    /// \return Size of the window, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] virtual Vector2u getSize() const = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Change the size of the rendering region of the window
    ///
    /// \param size New size, in pixels
    ///
    ////////////////////////////////////////////////////////////
    virtual void setSize(Vector2u size) = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Set the minimum window rendering region size
    ///
    /// Pass `std::nullopt` to unset the minimum size
    ///
    /// \param minimumSize New minimum size, in pixels
    ///
    ////////////////////////////////////////////////////////////
    virtual void setMinimumSize(const std::optional<Vector2u>& minimumSize);

    ////////////////////////////////////////////////////////////
    /// \brief Set the maximum window rendering region size
    ///
    /// Pass `std::nullopt` to unset the maximum size
    ///
    /// \param maximumSize New maximum size, in pixels
    ///
    ////////////////////////////////////////////////////////////
    virtual void setMaximumSize(const std::optional<Vector2u>& maximumSize);

    ////////////////////////////////////////////////////////////
    /// \brief Change the title of the window
    ///
    /// \param title New title
    ///
    ////////////////////////////////////////////////////////////
    virtual void setTitle(const String& title) = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Change the window's icon
    ///
    /// \param size   Icon's width and height, in pixels
    /// \param pixels Pointer to the pixels in memory, format must be RGBA 32 bits
    ///
    ////////////////////////////////////////////////////////////
    virtual void setIcon(Vector2u size, const std::uint8_t* pixels) = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Show or hide the window
    ///
    /// \param visible `true` to show, `false` to hide
    ///
    ////////////////////////////////////////////////////////////
    virtual void setVisible(bool visible) = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Show or hide the mouse cursor
    ///
    /// \param visible `true` to show, `false` to hide
    ///
    ////////////////////////////////////////////////////////////
    virtual void setMouseCursorVisible(bool visible) = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Grab or release the mouse cursor and keeps it from leaving
    ///
    /// \param grabbed `true` to enable, `false` to disable
    ///
    ////////////////////////////////////////////////////////////
    virtual void setMouseCursorGrabbed(bool grabbed) = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Set the displayed cursor to a native system cursor
    ///
    /// \param cursor Native system cursor type to display
    ///
    ////////////////////////////////////////////////////////////
    virtual void setMouseCursor(const CursorImpl& cursor) = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Enable or disable automatic key-repeat
    ///
    /// \param enabled `true` to enable, `false` to disable
    ///
    ////////////////////////////////////////////////////////////
    virtual void setKeyRepeatEnabled(bool enabled) = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Request the current window to be made the active
    ///        foreground window
    ///
    ////////////////////////////////////////////////////////////
    virtual void requestFocus() = 0;

    ////////////////////////////////////////////////////////////
    /// \brief Check whether the window has the input focus
    ///
    /// \return `true` if window has focus, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] virtual bool hasFocus() const = 0;

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
    bool createVulkanSurface(const VkInstance& instance, VkSurfaceKHR& surface, const VkAllocationCallbacks* allocator) const;

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
    virtual void setFileDroppingEnabled(bool enabled = true);

protected:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor
    ///
    ////////////////////////////////////////////////////////////
    WindowImpl();

    ////////////////////////////////////////////////////////////
    /// \brief Push a new event into the event queue
    ///
    /// This function is to be used by derived classes, to
    /// notify the SFML window that a new event was triggered
    /// by the system.
    ///
    /// \param event Event to push
    ///
    ////////////////////////////////////////////////////////////
    void pushEvent(const Event& event);

    ////////////////////////////////////////////////////////////
    /// \brief Process incoming events from the operating system
    ///
    ////////////////////////////////////////////////////////////
    virtual void processEvents() = 0;

private:
    struct JoystickStatesImpl;

    ////////////////////////////////////////////////////////////
    /// \return First event of the queue if available, `std::nullopt` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] std::optional<Event> popEvent();

    ////////////////////////////////////////////////////////////
    /// \brief Read the joysticks state and generate the appropriate events
    ///
    ////////////////////////////////////////////////////////////
    void processJoystickEvents();

    ////////////////////////////////////////////////////////////
    /// \brief Read the sensors state and generate the appropriate events
    ///
    ////////////////////////////////////////////////////////////
    void processSensorEvents();

    ////////////////////////////////////////////////////////////
    /// \brief Read joystick, sensors, and OS state and populate event queue
    ///
    ////////////////////////////////////////////////////////////
    void populateEventQueue();

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    std::queue<Event>                                m_events;             //!< Queue of available events
    std::unique_ptr<JoystickStatesImpl>              m_joystickStatesImpl; //!< Previous state of the joysticks (PImpl)
    EnumArray<Sensor::Type, Vector3f, Sensor::Count> m_sensorValue;        //!< Previous value of the sensors
    float m_joystickThreshold{0.1f}; //!< Joystick threshold (minimum motion for "move" event to be generated)
    std::array<EnumArray<Joystick::Axis, float, Joystick::AxisCount>, Joystick::Count>
        m_previousAxes{}; //!< Position of each axis last time a move event triggered, in range [-100, 100]
    std::optional<Vector2u> m_minimumSize; //!< Minimum window size
    std::optional<Vector2u> m_maximumSize; //!< Maximum window size
};

} // namespace priv

} // namespace sf
