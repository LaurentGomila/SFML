// Header for SFML unit tests.
//
// For a new graphics module test case, include this header.
// This ensures that string conversions are visible and can be used by doctest for debug output.

#ifndef SFML_TESTUTILITIES_GRAPHICS_HPP
#define SFML_TESTUTILITIES_GRAPHICS_HPP

#include "WindowUtil.hpp"

#include <SFML/Graphics/Rect.hpp>

namespace sf
{
    struct BlendMode;
    class Color;
    class Transform;

    std::ostream& operator <<(std::ostream& os, const BlendMode& blendMode);
    std::ostream& operator <<(std::ostream& os, const Color& color);
    std::ostream& operator <<(std::ostream& os, const Transform& transform);

    template <typename T>
    std::ostream& operator <<(std::ostream& os, const sf::Rect<T>& rect)
    {
        os << "(left=" << rect.left << ", top=" << rect.top << ", width=" << rect.width << ", height=" << rect.height << ")";
        return os;
    }
}

#endif // SFML_TESTUTILITIES_GRAPHICS_HPP
