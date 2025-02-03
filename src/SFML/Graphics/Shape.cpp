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
#include <cmath>
#include <cstddef>

namespace
{
// Compute the direction of a segment
sf::Vector2f computeDirection(sf::Vector2f p1, sf::Vector2f p2)
{
    sf::Vector2f direction = p2 - p1;
    const float  length    = direction.length();
    if (length != 0.f)
        direction /= length;
    return direction;
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
    updateOutline();
}


////////////////////////////////////////////////////////////
float Shape::getOutlineThickness() const
{
    return m_outlineThickness;
}


////////////////////////////////////////////////////////////
void Shape::setMiterLimit(float miterLimit)
{
    assert(miterLimit >= 1.f && "Shape::setMiterLimit(float) cannot set miter limit to a value lower than 1");
    m_miterLimit = miterLimit;
    updateOutline();
}


////////////////////////////////////////////////////////////
float Shape::getMiterLimit() const
{
    return m_miterLimit;
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
    for (std::size_t i = 0; i < m_vertices.getVertexCount(); ++i)
        m_vertices[i].color = m_fillColor;
}


////////////////////////////////////////////////////////////
void Shape::updateTexCoords()
{
    const FloatRect convertedTextureRect(m_textureRect);

    // Make sure not to divide by zero when the points are aligned on a vertical or horizontal line
    const Vector2f safeInsideSize(m_insideBounds.size.x > 0 ? m_insideBounds.size.x : 1.f,
                                  m_insideBounds.size.y > 0 ? m_insideBounds.size.y : 1.f);

    for (std::size_t i = 0; i < m_vertices.getVertexCount(); ++i)
    {
        const Vector2f ratio    = (m_vertices[i].position - m_insideBounds.position).componentWiseDiv(safeInsideSize);
        m_vertices[i].texCoords = convertedTextureRect.position + convertedTextureRect.size.componentWiseMul(ratio);
    }
}


////////////////////////////////////////////////////////////
void Shape::updateOutline()
{
    // Return if there is no outline or no vertices
    if (m_outlineThickness == 0.f || m_vertices.getVertexCount() < 2)
    {
        m_outlineVertices.clear();
        m_bounds = m_insideBounds;
        return;
    }

    const std::size_t count = m_vertices.getVertexCount() - 2;
    m_outlineVertices.resize((count + 1) * 2); // We need at least that many vertices.
                                               // We will add two more vertices each time we need a bevel.

    // Determine if points are defined clockwise or counterclockwise. This will impact normals computation.
    const bool flipNormals = [this, count]()
    {
        float twiceArea = 0.f;
        for (std::size_t i = 0; i < count; ++i)
            twiceArea += m_vertices[i + 1].position.cross(m_vertices[i + 2].position);
        return twiceArea >= 0.f;
    }();

    std::size_t outlineIndex = 0;
    for (std::size_t i = 0; i < count; ++i)
    {
        const std::size_t index = i + 1;

        // Get the two segments shared by the current point
        const Vector2f p0 = (i == 0) ? m_vertices[count].position : m_vertices[index - 1].position;
        const Vector2f p1 = m_vertices[index].position;
        const Vector2f p2 = m_vertices[index + 1].position;

        // Compute their direction
        const Vector2f d1 = computeDirection(p0, p1);
        const Vector2f d2 = computeDirection(p1, p2);

        // Compute their normal pointing towards the outside of the shape
        const Vector2f n1 = flipNormals ? -d1.perpendicular() : d1.perpendicular();
        const Vector2f n2 = flipNormals ? -d2.perpendicular() : d2.perpendicular();

        // Decide whether to add a bevel or not
        const float twoCos2            = 1.f + n1.dot(n2);
        const float squaredLengthRatio = m_miterLimit * m_miterLimit * twoCos2 / 2.f;
        const bool  isConvexCorner     = d1.dot(n2) * m_outlineThickness >= 0.f;
        const bool  needsBevel         = twoCos2 == 0.f || (squaredLengthRatio < 1.f && isConvexCorner);

        if (needsBevel)
        {
            // Make room for two more vertices
            m_outlineVertices.resize(m_outlineVertices.getVertexCount() + 2);

            // Combine normals to get bevel edge's direction and normal vector pointing towards the outside of the shape
            const float    twoSin2   = 1.f - n1.dot(n2);
            const Vector2f direction = (n2 - n1) / twoSin2; // Length is 1 / sin
            const Vector2f extrusion = (flipNormals != (d1.dot(n2) >= 0.f) ? direction : -direction).perpendicular();

            // Compute bevel corner position in (direction, extrusion) coordinates
            const float sin = std::sqrt(twoSin2 / 2.f);
            const float u   = m_miterLimit * sin;
            const float v   = 1.f - std::sqrt(squaredLengthRatio);

            // Update the outline points
            m_outlineVertices[outlineIndex++].position = p1;
            m_outlineVertices[outlineIndex++].position = p1 + (u * extrusion - v * direction) * m_outlineThickness;
            m_outlineVertices[outlineIndex++].position = p1;
            m_outlineVertices[outlineIndex++].position = p1 + (u * extrusion + v * direction) * m_outlineThickness;
        }
        else
        {
            // Combine normals to get the extrusion direction
            const Vector2f extrusion = (n1 + n2) / twoCos2;

            // Update the outline points
            m_outlineVertices[outlineIndex++].position = p1;
            m_outlineVertices[outlineIndex++].position = p1 + extrusion * m_outlineThickness;
        }
    }

    // Duplicate the first point at the end, to close the outline
    m_outlineVertices[outlineIndex++].position = m_outlineVertices[0].position;
    m_outlineVertices[outlineIndex++].position = m_outlineVertices[1].position;

    // Update outline colors
    updateOutlineColors();

    // Update the shape's bounds
    m_bounds = m_outlineVertices.getBounds();
}


////////////////////////////////////////////////////////////
void Shape::updateOutlineColors()
{
    for (std::size_t i = 0; i < m_outlineVertices.getVertexCount(); ++i)
        m_outlineVertices[i].color = m_outlineColor;
}

} // namespace sf
