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
#include <SFML/Window/MonitorImpl.hpp>


namespace sf
{
class VideoMode;

namespace priv
{
////////////////////////////////////////////////////////////
/// \brief iOS (UIKit) implementation of MonitorImpl
///
/// \note Placeholder class. Actual storage of the monitor handle not implemented yet.
///
////////////////////////////////////////////////////////////
class MonitorImplUIKit : public MonitorImpl
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Construct the monitor implementation
    ///
    ////////////////////////////////////////////////////////////
    MonitorImplUIKit();

    ////////////////////////////////////////////////////////////
    /// \brief Create primary monitor implementation
    ///
    /// \return Pointer to the created primary monitor implementation
    ///
    ////////////////////////////////////////////////////////////
    static std::unique_ptr<MonitorImpl> createPrimaryMonitor();

    ////////////////////////////////////////////////////////////
    /// \brief Get the list of all the supported fullscreen video modes of this monitor
    ///
    /// \return Array filled with the fullscreen video modes of this monitor
    ///
    ////////////////////////////////////////////////////////////
    std::vector<VideoMode> getFullscreenModes();

    ////////////////////////////////////////////////////////////
    /// \brief Get the current desktop video mode of this monitor
    ///
    /// \return Current desktop video mode of this monitor
    ///
    ////////////////////////////////////////////////////////////
    VideoMode getDesktopMode();
};

} // namespace priv

} // namespace sf