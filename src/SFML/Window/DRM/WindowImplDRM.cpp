////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2024-2025 Andrew Mickelson
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
#include <SFML/Window/DRM/DRMContext.hpp>
#include <SFML/Window/DRM/WindowImplDRM.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/InputImpl.hpp>
#include <SFML/Window/WindowEnums.hpp>

#include <SFML/System/Err.hpp>


namespace sf::priv
{

// Defined in DRM/InputImpl.cpp because they require access to that file's global state
namespace InputImpl
{
////////////////////////////////////////////////////////////
/// \brief Fetch input event from event queue
///
/// \return False if event queue is empty
///
////////////////////////////////////////////////////////////
std::optional<Event> checkEvent();

////////////////////////////////////////////////////////////
/// \brief Backup terminal configuration and disable console feedback
///
////////////////////////////////////////////////////////////
void setTerminalConfig();

////////////////////////////////////////////////////////////
/// \brief Restore terminal configuration from backup
///
////////////////////////////////////////////////////////////
void restoreTerminalConfig();
} // namespace InputImpl


////////////////////////////////////////////////////////////
WindowImplDRM::WindowImplDRM(WindowHandle /*handle*/)
{
    InputImpl::setTerminalConfig();
}


////////////////////////////////////////////////////////////
WindowImplDRM::WindowImplDRM(VideoMode mode,
                             const String& /*title*/,
                             std::uint32_t /*style*/,
                             State /*state*/,
                             const ContextSettings& /*settings*/) :
m_size(mode.size)
{
    InputImpl::setTerminalConfig();
}


////////////////////////////////////////////////////////////
WindowImplDRM::~WindowImplDRM()
{
    InputImpl::restoreTerminalConfig();
}


////////////////////////////////////////////////////////////
WindowHandle WindowImplDRM::getNativeHandle() const
{
    const Drm& drm = DRMContext::getDRM();
    return static_cast<WindowHandle>(drm.fileDescriptor);
}

////////////////////////////////////////////////////////////
Vector2i WindowImplDRM::getPosition() const
{
    return {0, 0};
}


////////////////////////////////////////////////////////////
void WindowImplDRM::setPosition(Vector2i /*position*/)
{
}


////////////////////////////////////////////////////////////
Vector2u WindowImplDRM::getSize() const
{
    return m_size;
}


////////////////////////////////////////////////////////////
void WindowImplDRM::setSize(Vector2u /*size*/)
{
}


////////////////////////////////////////////////////////////
void WindowImplDRM::setMinimumSize(const std::optional<Vector2u>& /* minimumSize */)
{
}


////////////////////////////////////////////////////////////
void WindowImplDRM::setMaximumSize(const std::optional<Vector2u>& /* maximumSize */)
{
}


////////////////////////////////////////////////////////////
void WindowImplDRM::setTitle(const String& /*title*/)
{
}


////////////////////////////////////////////////////////////
void WindowImplDRM::setIcon(Vector2u /*size*/, const std::uint8_t* /*pixels*/)
{
}


////////////////////////////////////////////////////////////
void WindowImplDRM::setVisible(bool /*visible*/)
{
}

////////////////////////////////////////////////////////////
void WindowImplDRM::setMouseCursorVisible(bool /*visible*/)
{
    // TODO: not implemented
}

////////////////////////////////////////////////////////////
void WindowImplDRM::setMouseCursorGrabbed(bool /*grabbed*/)
{
    // TODO: not implemented
}

////////////////////////////////////////////////////////////
void WindowImplDRM::setMouseCursor(const CursorImpl& /*cursor*/)
{
    // TODO: not implemented
}

////////////////////////////////////////////////////////////
void WindowImplDRM::setKeyRepeatEnabled(bool /*enabled*/)
{
    // TODO: not implemented
}


////////////////////////////////////////////////////////////
void WindowImplDRM::requestFocus()
{
    // Not applicable
}


////////////////////////////////////////////////////////////
bool WindowImplDRM::hasFocus() const
{
    return true;
}


////////////////////////////////////////////////////////////
void WindowImplDRM::setImePreEditPosition(const Vector2i& /* position */)
{
    // Not applicable
}


void WindowImplDRM::processEvents()
{
    while (const std::optional event = InputImpl::checkEvent())
        pushEvent(*event);
}

} // namespace sf::priv
