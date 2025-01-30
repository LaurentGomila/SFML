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
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/Cursor.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/Vulkan.hpp>
#include <SFML/Window/WindowBase.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <SFML/Window/WindowHandle.hpp>
#include <SFML/Window/WindowImpl.hpp>

#include <algorithm>
#include <limits>

#include <cassert>
#include <cstdlib>


namespace sf
{
////////////////////////////////////////////////////////////
WindowBase::WindowBase() = default;


////////////////////////////////////////////////////////////
WindowBase::WindowBase(VideoMode mode, const String& title, std::uint32_t style, State state)
{
    WindowBase::create(mode, title, style, state);
}


////////////////////////////////////////////////////////////
WindowBase::WindowBase(VideoMode mode, const String& title, State state)
{
    WindowBase::create(mode, title, Style::Default, state);
}


////////////////////////////////////////////////////////////
WindowBase::WindowBase(WindowHandle handle)
{
    WindowBase::create(handle);
}


////////////////////////////////////////////////////////////
WindowBase::~WindowBase() = default;


////////////////////////////////////////////////////////////
WindowBase::WindowBase(WindowBase&&) noexcept = default;


////////////////////////////////////////////////////////////
WindowBase& WindowBase::operator=(WindowBase&&) noexcept = default;


////////////////////////////////////////////////////////////
void WindowBase::create(VideoMode mode, const String& title, std::uint32_t style, State state)
{
    // Recreate the window implementation
    m_impl = priv::WindowImpl::create(mode,
                                      title,
                                      style,
                                      state,
                                      ContextSettings{/* depthBits */ 0,
                                                      /* stencilBits */ 0,
                                                      /* antiAliasingLevel */ 0,
                                                      /* majorVersion */ 0,
                                                      /* minorVersion */ 0,
                                                      /* attributeFlags */ 0xFFFFFFFF,
                                                      /* sRgbCapable */ false});

    // Perform common initializations
    initialize();
}


////////////////////////////////////////////////////////////
void WindowBase::create(VideoMode mode, const String& title, State state)
{
    create(mode, title, Style::Default, state);
}


////////////////////////////////////////////////////////////
void WindowBase::create(WindowHandle handle)
{
    // Ensure the open window is closed first
    close();

    // Recreate the window implementation
    m_impl = priv::WindowImpl::create(handle);

    // Perform common initializations
    initialize();
}


////////////////////////////////////////////////////////////
void WindowBase::close()
{
    // Delete the window implementation
    m_impl.reset();
}


////////////////////////////////////////////////////////////
bool WindowBase::isOpen() const
{
    return m_impl != nullptr;
}


////////////////////////////////////////////////////////////
std::optional<Event> WindowBase::pollEvent()
{
    std::optional<sf::Event> event; // Use a single local variable for NRVO

    if (m_impl == nullptr)
        return event; // Empty optional

    event = m_impl->pollEvent();

    if (event.has_value())
        filterEvent(*event);

    return event;
}


////////////////////////////////////////////////////////////
std::optional<Event> WindowBase::waitEvent(Time timeout)
{
    std::optional<sf::Event> event; // Use a single local variable for NRVO

    if (m_impl == nullptr)
        return event; // Empty optional

    event = m_impl->waitEvent(timeout);

    if (event.has_value())
        filterEvent(*event);

    return event;
}


////////////////////////////////////////////////////////////
Vector2i WindowBase::getPosition() const
{
    return m_impl ? m_impl->getPosition() : Vector2i();
}


////////////////////////////////////////////////////////////
void WindowBase::setPosition(Vector2i position)
{
    if (m_impl)
        m_impl->setPosition(position);
}


////////////////////////////////////////////////////////////
Vector2u WindowBase::getSize() const
{
    return m_size;
}


////////////////////////////////////////////////////////////
void WindowBase::setSize(Vector2u size)
{
    if (m_impl)
    {
        // Constrain requested size within minimum and maximum bounds
        const auto minimumSize = m_impl->getMinimumSize().value_or(Vector2u());
        const auto maximumSize = m_impl->getMaximumSize().value_or(
            Vector2u(std::numeric_limits<unsigned int>::max(), std::numeric_limits<unsigned int>::max()));
        const auto width  = std::clamp(size.x, minimumSize.x, maximumSize.x);
        const auto height = std::clamp(size.y, minimumSize.y, maximumSize.y);

        // Do nothing if requested size matches current size
        const Vector2u clampedSize(width, height);
        if (clampedSize == m_size)
            return;

        m_impl->setSize(clampedSize);

        // Cache the new size
        m_size = clampedSize;

        // Notify the derived class
        onResize();
    }
}


////////////////////////////////////////////////////////////
void WindowBase::setMinimumSize(const std::optional<Vector2u>& minimumSize)
{
    if (m_impl)
    {
        [[maybe_unused]] const auto validateMinimumSize = [this, minimumSize]
        {
            if (!minimumSize.has_value() || !m_impl->getMaximumSize().has_value())
                return true;
            return minimumSize->x <= m_impl->getMaximumSize()->x && minimumSize->y <= m_impl->getMaximumSize()->y;
        };
        assert(validateMinimumSize() && "Minimum size cannot be bigger than the maximum size along either axis");

        m_impl->setMinimumSize(minimumSize);
        setSize(getSize());
    }
}


////////////////////////////////////////////////////////////
void WindowBase::setMaximumSize(const std::optional<Vector2u>& maximumSize)
{
    if (m_impl)
    {
        [[maybe_unused]] const auto validateMaximumSize = [this, maximumSize]
        {
            if (!maximumSize.has_value() || !m_impl->getMinimumSize().has_value())
                return true;
            return maximumSize->x >= m_impl->getMinimumSize()->x && maximumSize->y >= m_impl->getMinimumSize()->y;
        };
        assert(validateMaximumSize() && "Maximum size cannot be smaller than the minimum size along either axis");

        m_impl->setMaximumSize(maximumSize);
        setSize(getSize());
    }
}


////////////////////////////////////////////////////////////
void WindowBase::setTitle(const String& title)
{
    if (m_impl)
        m_impl->setTitle(title);
}


////////////////////////////////////////////////////////////
void WindowBase::setIcon(Vector2u size, const std::uint8_t* pixels)
{
    if (m_impl)
        m_impl->setIcon(size, pixels);
}


////////////////////////////////////////////////////////////
void WindowBase::setVisible(bool visible)
{
    if (m_impl)
        m_impl->setVisible(visible);
}


////////////////////////////////////////////////////////////
void WindowBase::setMouseCursorVisible(bool visible)
{
    if (m_impl)
        m_impl->setMouseCursorVisible(visible);
}


////////////////////////////////////////////////////////////
void WindowBase::setMouseCursorGrabbed(bool grabbed)
{
    if (m_impl)
        m_impl->setMouseCursorGrabbed(grabbed);
}


////////////////////////////////////////////////////////////
void WindowBase::setMouseCursor(const Cursor& cursor)
{
    if (m_impl)
        m_impl->setMouseCursor(cursor.getImpl());
}


////////////////////////////////////////////////////////////
void WindowBase::setKeyRepeatEnabled(bool enabled)
{
    if (m_impl)
        m_impl->setKeyRepeatEnabled(enabled);
}


////////////////////////////////////////////////////////////
void WindowBase::setJoystickThreshold(float threshold)
{
    if (m_impl)
        m_impl->setJoystickThreshold(threshold);
}


////////////////////////////////////////////////////////////
void WindowBase::requestFocus()
{
    if (m_impl)
        m_impl->requestFocus();
}


////////////////////////////////////////////////////////////
bool WindowBase::hasFocus() const
{
    return m_impl && m_impl->hasFocus();
}


////////////////////////////////////////////////////////////
void WindowBase::setImePreEditPosition(const Vector2i& position)
{
    if (m_impl)
        m_impl->setImePreEditPosition(position);
}


////////////////////////////////////////////////////////////
WindowHandle WindowBase::getNativeHandle() const
{
    return m_impl ? m_impl->getNativeHandle() : WindowHandle{};
}


////////////////////////////////////////////////////////////
bool WindowBase::createVulkanSurface(const VkInstance& instance, VkSurfaceKHR& surface, const VkAllocationCallbacks* allocator)
{
    return m_impl ? m_impl->createVulkanSurface(instance, surface, allocator) : false;
}


////////////////////////////////////////////////////////////
void WindowBase::onCreate()
{
    // Nothing by default
}


////////////////////////////////////////////////////////////
void WindowBase::onResize()
{
    // Nothing by default
}


////////////////////////////////////////////////////////////
void WindowBase::filterEvent(const Event& event)
{
    // Notify resize events to the derived class
    if (const auto* resized = event.getIf<Event::Resized>())
    {
        // Cache the new size
        m_size = resized->size;

        // Notify the derived class
        onResize();
    }
}


////////////////////////////////////////////////////////////
void WindowBase::initialize()
{
    // Setup default behaviors (to get a consistent behavior across different implementations)
    setVisible(true);
    setMouseCursorVisible(true);
    setKeyRepeatEnabled(true);

    // Get and cache the initial size of the window
    m_size = m_impl->getSize();

    // Notify the derived class
    onCreate();
}

} // namespace sf
