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
#import <SFML/Window/macOS/Scaling.h>
#include <SFML/Window/macOS/Utils.hpp>
#include <SFML/Window/macOS/cg_sf_conversion.hpp>

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

namespace sf::priv
{

////////////////////////////////////////////////////////////
unsigned int modeBitsPerPixel(CGDisplayModeRef mode)
{
    // Compare encoding.
    const auto pixEnc = CFPtr<CFStringRef>(CGDisplayModeCopyPixelEncoding(mode));
    if (CFStringCompare(pixEnc.get(), CFSTR(IO32BitDirectPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        return 32;
    if (CFStringCompare(pixEnc.get(), CFSTR(IO16BitDirectPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        return 16;
    if (CFStringCompare(pixEnc.get(), CFSTR(IO8BitIndexedPixels), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        return 8;

    return 0;
}


////////////////////////////////////////////////////////////
VideoMode convertCGModeToSFMode(CGDisplayModeRef cgmode)
{
    // The main documentation says the sizes returned by
    // CGDisplayModeGetWidth and CGDisplayModeGetHeight
    // are expressed in pixels. However, some additional
    // documentation [1] states they actually return
    // values in points starting with 10.8.
    //
    // We therefore needs to use the scaling factor to
    // convert the dimensions properly.
    //
    // [1]: "APIs for Supporting High Resolution" > "Additions and Changes for OS X v10.8"
    // https://developer.apple.com/library/mac/documentation/GraphicsAnimation/Conceptual/HighResolutionOSX/APIs/APIs.html#//apple_ref/doc/uid/TP40012302-CH5-SW27
    auto size = Vector2u(Vector2(CGDisplayModeGetPixelWidth(cgmode), CGDisplayModeGetPixelHeight(cgmode)));
    scaleInXY(size, nil);
    return VideoMode(size, modeBitsPerPixel(cgmode));
}

} // namespace sf::priv
