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
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Shape.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <algorithm>

#include <cassert>
#include <cstddef>

namespace
{
// Compute the normal of a segment
sf::Vector2f computeNormal(sf::Vector2f p1, sf::Vector2f p2)
{
    sf::Vector2f normal = (p2 - p1).perpendicular();
    const float  length = normal.length();
    if (length != 0.f)
        normal /= length;
    return normal;
}
} // namespace


namespace sf
{
////////////////////////////////////////////////////////////
void Shape::setTexture(const Texture* texture, bool resetRect)
{
    if (texture)
    {
        // Recompute the texture area if requested, or if there was no texture & rect before
        if (resetRect || (!m_texture && (m_textureRect == IntRect())))
            setTextureRect(IntRect({0, 0}, Vector2i(texture->getSize())));
    }

    // Assign the new texture
    m_texture = texture;
}


////////////////////////////////////////////////////////////
const Texture* Shape::getTexture() const
{
    return m_texture;
}


////////////////////////////////////////////////////////////
void Shape::setTextureRect(const IntRect& rect)
{
    m_textureRect = rect;
    updateTexCoords();
}


////////////////////////////////////////////////////////////
const IntRect& Shape::getTextureRect() const
{
    return m_textureRect;
}


////////////////////////////////////////////////////////////
void Shape::setFillColor(Color color)
{
    m_fillColor = color;
    updateFillColors();
}


////////////////////////////////////////////////////////////
Color Shape::getFillColor() const
{
    return m_fillColor;
}


////////////////////////////////////////////////////////////
void Shape::setOutlineColor(Color color)
{
    m_outlineColor = color;
    updateOutlineColors();
}


////////////////////////////////////////////////////////////
Color Shape::getOutlineColor() const
{
    return m_outlineColor;
}


////////////////////////////////////////////////////////////
void Shape::setOutlineThickness(float thickness)
{
    m_outlineThickness = thickness;
    update(); // recompute everything because the whole shape must be offset
}


////////////////////////////////////////////////////////////
float Shape::getOutlineThickness() const
{
    return m_outlineThickness;
}


////////////////////////////////////////////////////////////
Vector2f Shape::getGeometricCenter() const
{
    const auto count = getPointCount();

    switch (count)
    {
        case 0:
            assert(false && "Cannot calculate geometric center of shape with no points");
            return Vector2f{};
        case 1:
            return getPoint(0);
        case 2:
            return (getPoint(0) + getPoint(1)) / 2.f;
        default: // more than two points
            Vector2f centroid;
            float    twiceArea = 0;

            auto previousPoint = getPoint(count - 1);
            for (std::size_t i = 0; i < count; ++i)
            {
                const auto  currentPoint = getPoint(i);
                const float product      = previousPoint.cross(currentPoint);
                twiceArea += product;
                centroid += (currentPoint + previousPoint) * product;

                previousPoint = currentPoint;
            }

            if (twiceArea != 0.f)
            {
                return centroid / 3.f / twiceArea;
            }

            // Fallback for no area - find the center of the bounding box
            auto minPoint = getPoint(0);
            auto maxPoint = minPoint;
            for (std::size_t i = 1; i < count; ++i)
            {
                const auto currentPoint = getPoint(i);
                minPoint.x              = std::min(minPoint.x, currentPoint.x);
                maxPoint.x              = std::max(maxPoint.x, currentPoint.x);
                minPoint.y              = std::min(minPoint.y, currentPoint.y);
                maxPoint.y              = std::max(maxPoint.y, currentPoint.y);
            }
            return (maxPoint + minPoint) / 2.f;
    }
}


////////////////////////////////////////////////////////////
FloatRect Shape::getLocalBounds() const
{
    return m_bounds;
}


////////////////////////////////////////////////////////////
FloatRect Shape::getGlobalBounds() const
{
    return getTransform().transformRect(getLocalBounds());
}


////////////////////////////////////////////////////////////
void Shape::update()
{
    // Get the total number of points of the shape
    const std::size_t count = getPointCount();
    if (count < 3)
    {
        m_vertices.clear();
        m_outlineVertices.clear();
        return;
    }

    m_vertices.resize(count + 2); // + 2 for center and repeated first point

    // Position
    for (std::size_t i = 0; i < count; ++i)
        m_vertices[i + 1].position = getPoint(i);
    m_vertices[count + 1].position = m_vertices[1].position;

    // Update the bounding rectangle
    m_vertices[0]  = m_vertices[1]; // so that the result of getBounds() is correct
    m_insideBounds = m_vertices.getBounds();

    // Compute the center and make it the first vertex
    m_vertices[0].position = m_insideBounds.getCenter();

    // Color
    updateFillColors();

    // Texture coordinates
    updateTexCoords();

    // Outline
    updateOutline();
}


////////////////////////////////////////////////////////////
void Shape::draw(RenderTarget& target, RenderStates states) const
{
    states.transform *= getTransform();
    states.coordinateType = CoordinateType::Pixels;

    // Render the inside
    states.texture = m_texture;
    target.draw(m_vertices, states);

    // Render the outline
    if (m_outlineThickness != 0)
    {
        states.texture = nullptr;
        target.draw(m_outlineVertices, states);
    }
}


////////////////////////////////////////////////////////////
void Shape::updateFillColors()
{
    for (auto& vertex : m_vertices)
        vertex.color = m_fillColor;
}


////////////////////////////////////////////////////////////
void Shape::updateTexCoords()
{
    const FloatRect convertedTextureRect(m_textureRect);

    // Make sure not to divide by zero when the points are aligned on a vertical or horizontal line
    const Vector2f safeInsideSize(m_insideBounds.size.x > 0 ? m_insideBounds.size.x : 1.f,
                                  m_insideBounds.size.y > 0 ? m_insideBounds.size.y : 1.f);

    for (auto& vertex : m_vertices)
    {
        const Vector2f ratio = (vertex.position - m_insideBounds.position).componentWiseDiv(safeInsideSize);
        vertex.texCoords     = convertedTextureRect.position + convertedTextureRect.size.componentWiseMul(ratio);
    }
}


////////////////////////////////////////////////////////////
void Shape::updateOutline()
{
    // Return if there is no outline
    if (m_outlineThickness == 0.f)
    {
        m_outlineVertices.clear();
        m_bounds = m_insideBounds;
        return;
    }

    const std::size_t count = m_vertices.getVertexCount() - 2;
    m_outlineVertices.resize((count + 1) * 2);

    for (std::size_t i = 0; i < count; ++i)
    {
        const std::size_t index = i + 1;

        // Get the two segments shared by the current point
        const Vector2f p0 = (i == 0) ? m_vertices[count].position : m_vertices[index - 1].position;
        const Vector2f p1 = m_vertices[index].position;
        const Vector2f p2 = m_vertices[index + 1].position;

        // Compute their normal
        Vector2f n1 = computeNormal(p0, p1);
        Vector2f n2 = computeNormal(p1, p2);

        // Make sure that the normals point towards the outside of the shape
        // (this depends on the order in which the points were defined)
        if (n1.dot(m_vertices[0].position - p1) > 0)
            n1 = -n1;
        if (n2.dot(m_vertices[0].position - p1) > 0)
            n2 = -n2;

        // Combine them to get the extrusion direction
        const float    factor = 1.f + (n1.x * n2.x + n1.y * n2.y);
        const Vector2f normal = (n1 + n2) / factor;

        // Update the outline points
        m_outlineVertices[i * 2 + 0].position = p1;
        m_outlineVertices[i * 2 + 1].position = p1 + normal * m_outlineThickness;
    }

    // Duplicate the first point at the end, to close the outline
    m_outlineVertices[count * 2 + 0].position = m_outlineVertices[0].position;
    m_outlineVertices[count * 2 + 1].position = m_outlineVertices[1].position;

    // Update outline colors
    updateOutlineColors();

    // Update the shape's bounds
    m_bounds = m_outlineVertices.getBounds();
}


////////////////////////////////////////////////////////////
void Shape::updateOutlineColors()
{
    for (auto& vertex : m_outlineVertices)
        vertex.color = m_outlineColor;
}

} // namespace sf
