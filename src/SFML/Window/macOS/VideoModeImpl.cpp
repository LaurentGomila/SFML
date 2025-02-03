////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2025 Marco Antognini (antognini.marco@gmail.com),
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
#include <SFML/Window/VideoModeImpl.hpp>
#include <SFML/Window/macOS/Utils.hpp>
#include <SFML/Window/macOS/cg_sf_conversion.hpp>

#include <SFML/System/Err.hpp>

#include <algorithm>
#include <ostream>

namespace sf::priv
{

////////////////////////////////////////////////////////////
std::vector<VideoMode> VideoModeImpl::getFullscreenModes()
{
    // Retrieve all modes available for main screen only.
    const auto cgmodes = CFPtr<CFArrayRef>(CGDisplayCopyAllDisplayModes(CGMainDisplayID(), nullptr));

    if (cgmodes == nullptr)
    {
        sf::err() << "Couldn't get VideoMode for main display." << std::endl;
        return {};
    }

    const VideoMode        desktop = getDesktopMode();
    std::vector<VideoMode> modes   = {desktop};

    // Loop on each mode and convert it into a sf::VideoMode object.
    const CFIndex modesCount = CFArrayGetCount(cgmodes.get());
    for (CFIndex i = 0; i < modesCount; ++i)
    {
        auto* cgmode = static_cast<CGDisplayModeRef>(const_cast<void*>(CFArrayGetValueAtIndex(cgmodes.get(), i)));

        const VideoMode mode = convertCGModeToSFMode(cgmode);

        // Skip if bigger than desktop as we currently don't perform hard resolution switch
        if ((mode.size.x > desktop.size.x) || (mode.size.y > desktop.size.y))
            continue;

        // If not yet listed we add it to our modes array.
        if (std::find(modes.begin(), modes.end(), mode) == modes.end())
            modes.push_back(mode);
    }

    return modes;
}


////////////////////////////////////////////////////////////
VideoMode VideoModeImpl::getDesktopMode()
{
    VideoMode mode; // RVO

    // Rely exclusively on mode and convertCGModeToSFMode
    // instead of display id and CGDisplayPixelsHigh/Wide.

    const CGDirectDisplayID display = CGMainDisplayID();
    CGDisplayModeRef        cgmode  = CGDisplayCopyDisplayMode(display);

    mode = convertCGModeToSFMode(cgmode);

    CGDisplayModeRelease(cgmode);

    return mode;
}

} // namespace sf::priv
