////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2022 Laurent Gomila (laurent@sfml-dev.org)
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
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <algorithm>
#include <cmath>


namespace
{
// Add an underline or strikethrough line to the vertex array
void addLine(sf::VertexArray& vertices,
             float            lineLength,
             float            lineTop,
             const sf::Color& color,
             float            offset,
             float            thickness,
             float            outlineThickness = 0)
{
    float top    = std::floor(lineTop + offset - (thickness / 2) + 0.5f);
    float bottom = top + std::floor(thickness + 0.5f);

    vertices.append(sf::Vertex(sf::Vector2f(-outlineThickness, top - outlineThickness), color, sf::Vector2f(1, 1)));
    vertices.append(
        sf::Vertex(sf::Vector2f(lineLength + outlineThickness, top - outlineThickness), color, sf::Vector2f(1, 1)));
    vertices.append(sf::Vertex(sf::Vector2f(-outlineThickness, bottom + outlineThickness), color, sf::Vector2f(1, 1)));
    vertices.append(sf::Vertex(sf::Vector2f(-outlineThickness, bottom + outlineThickness), color, sf::Vector2f(1, 1)));
    vertices.append(
        sf::Vertex(sf::Vector2f(lineLength + outlineThickness, top - outlineThickness), color, sf::Vector2f(1, 1)));
    vertices.append(
        sf::Vertex(sf::Vector2f(lineLength + outlineThickness, bottom + outlineThickness), color, sf::Vector2f(1, 1)));
}

// Add a glyph quad to the vertex array
void addGlyphQuad(sf::VertexArray& vertices, sf::Vector2f position, const sf::Color& color, const sf::Glyph& glyph, float italicShear)
{
    float padding = 1.0;

    float left   = glyph.bounds.left - padding;
    float top    = glyph.bounds.top - padding;
    float right  = glyph.bounds.left + glyph.bounds.width + padding;
    float bottom = glyph.bounds.top + glyph.bounds.height + padding;

    float u1 = static_cast<float>(glyph.textureRect.left) - padding;
    float v1 = static_cast<float>(glyph.textureRect.top) - padding;
    float u2 = static_cast<float>(glyph.textureRect.left + glyph.textureRect.width) + padding;
    float v2 = static_cast<float>(glyph.textureRect.top + glyph.textureRect.height) + padding;

    vertices.append(
        sf::Vertex(sf::Vector2f(position.x + left - italicShear * top, position.y + top), color, sf::Vector2f(u1, v1)));
    vertices.append(
        sf::Vertex(sf::Vector2f(position.x + right - italicShear * top, position.y + top), color, sf::Vector2f(u2, v1)));
    vertices.append(
        sf::Vertex(sf::Vector2f(position.x + left - italicShear * bottom, position.y + bottom), color, sf::Vector2f(u1, v2)));
    vertices.append(
        sf::Vertex(sf::Vector2f(position.x + left - italicShear * bottom, position.y + bottom), color, sf::Vector2f(u1, v2)));
    vertices.append(
        sf::Vertex(sf::Vector2f(position.x + right - italicShear * top, position.y + top), color, sf::Vector2f(u2, v1)));
    vertices.append(sf::Vertex(sf::Vector2f(position.x + right - italicShear * bottom, position.y + bottom),
                               color,
                               sf::Vector2f(u2, v2)));
}
} // namespace


namespace sf
{
////////////////////////////////////////////////////////////
Text::Text() :
m_string(),
m_font(nullptr),
m_characterSize(30),
m_letterSpacingFactor(1.f),
m_lineSpacingFactor(1.f),
m_style(Regular),
m_fillColor(255, 255, 255),
m_outlineColor(0, 0, 0),
m_outlineThickness(0),
m_bounds(),
m_geometryNeedUpdate(false)
{
}


////////////////////////////////////////////////////////////
Text::Text(const String& string, const Font& font, unsigned int characterSize) :
m_string(string),
m_font(&font),
m_characterSize(characterSize),
m_letterSpacingFactor(1.f),
m_lineSpacingFactor(1.f),
m_style(Regular),
m_fillColor(255, 255, 255),
m_outlineColor(0, 0, 0),
m_outlineThickness(0),
m_bounds(),
m_geometryNeedUpdate(true)
{
}


////////////////////////////////////////////////////////////
Text::Text(const Text&) = default;


////////////////////////////////////////////////////////////
Text& Text::operator=(const Text&) = default;


////////////////////////////////////////////////////////////
Text::Text(Text&&) noexcept = default;


////////////////////////////////////////////////////////////
Text& Text::operator=(Text&&) noexcept = default;


////////////////////////////////////////////////////////////
void Text::setString(const String& string)
{
    if (m_string != string)
    {
        m_string             = string;
        m_geometryNeedUpdate = true;
    }
}


////////////////////////////////////////////////////////////
void Text::setFont(const Font& font)
{
    if (m_font != &font)
    {
        m_font               = &font;
        m_geometryNeedUpdate = true;
    }
}


////////////////////////////////////////////////////////////
void Text::setCharacterSize(unsigned int size)
{
    if (m_characterSize != size)
    {
        m_characterSize      = size;
        m_geometryNeedUpdate = true;

        // Glyph textures will change, so delete all VertexArray instances
        m_fillVerticesMap.clear();
        m_outlineVerticesMap.clear();
    }
}


////////////////////////////////////////////////////////////
void Text::setLetterSpacing(float spacingFactor)
{
    if (m_letterSpacingFactor != spacingFactor)
    {
        m_letterSpacingFactor = spacingFactor;
        m_geometryNeedUpdate  = true;
    }
}


////////////////////////////////////////////////////////////
void Text::setLineSpacing(float spacingFactor)
{
    if (m_lineSpacingFactor != spacingFactor)
    {
        m_lineSpacingFactor  = spacingFactor;
        m_geometryNeedUpdate = true;
    }
}


////////////////////////////////////////////////////////////
void Text::setStyle(std::uint32_t style)
{
    if (m_style != style)
    {
        m_style              = style;
        m_geometryNeedUpdate = true;
    }
}


////////////////////////////////////////////////////////////
void Text::setFillColor(const Color& color)
{
    if (color != m_fillColor)
    {
        m_fillColor = color;

        // Change vertex colors directly, no need to update whole geometry
        // (if geometry is updated anyway, we can skip this step)
        if (!m_geometryNeedUpdate)
        {
            for (auto& vertMap : m_fillVerticesMap)
            {
                VertexArray& vertices = vertMap.second;
                for (std::size_t i = 0; i < vertices.getVertexCount(); ++i)
                    vertices[i].color = m_fillColor;
            }
        }
    }
}


////////////////////////////////////////////////////////////
void Text::setOutlineColor(const Color& color)
{
    if (color != m_outlineColor)
    {
        m_outlineColor = color;

        // Change vertex colors directly, no need to update whole geometry
        // (if geometry is updated anyway, we can skip this step)
        if (!m_geometryNeedUpdate)
        {
            for (auto& vertMap : m_outlineVerticesMap)
            {
                VertexArray& vertices = vertMap.second;
                for (std::size_t i = 0; i < vertices.getVertexCount(); ++i)
                    vertices[i].color = m_outlineColor;
            }
        }
    }
}


////////////////////////////////////////////////////////////
void Text::setOutlineThickness(float thickness)
{
    if (thickness != m_outlineThickness)
    {
        m_outlineThickness   = thickness;
        m_geometryNeedUpdate = true;
    }
}


////////////////////////////////////////////////////////////
const String& Text::getString() const
{
    return m_string;
}


////////////////////////////////////////////////////////////
const Font* Text::getFont() const
{
    return m_font;
}


////////////////////////////////////////////////////////////
unsigned int Text::getCharacterSize() const
{
    return m_characterSize;
}


////////////////////////////////////////////////////////////
float Text::getLetterSpacing() const
{
    return m_letterSpacingFactor;
}


////////////////////////////////////////////////////////////
float Text::getLineSpacing() const
{
    return m_lineSpacingFactor;
}


////////////////////////////////////////////////////////////
std::uint32_t Text::getStyle() const
{
    return m_style;
}


////////////////////////////////////////////////////////////
const Color& Text::getFillColor() const
{
    return m_fillColor;
}


////////////////////////////////////////////////////////////
const Color& Text::getOutlineColor() const
{
    return m_outlineColor;
}


////////////////////////////////////////////////////////////
float Text::getOutlineThickness() const
{
    return m_outlineThickness;
}


////////////////////////////////////////////////////////////
Vector2f Text::findCharacterPos(std::size_t index) const
{
    // Make sure that we have a valid font
    if (!m_font)
        return Vector2f();

    // Adjust the index if it's out of range
    if (index > m_string.getSize())
        index = m_string.getSize();

    // Precompute the variables needed by the algorithm
    bool  isBold          = m_style & Bold;
    float whitespaceWidth = m_font->getGlyph(U' ', m_characterSize, isBold).advance;
    float letterSpacing   = (whitespaceWidth / 3.f) * (m_letterSpacingFactor - 1.f);
    whitespaceWidth += letterSpacing;
    float lineSpacing = m_font->getLineSpacing(m_characterSize) * m_lineSpacingFactor;

    // Compute the position
    Vector2f      position;
    std::uint32_t prevChar = 0;
    for (std::size_t i = 0; i < index; ++i)
    {
        std::uint32_t curChar = m_string[i];

        // Apply the kerning offset
        position.x += m_font->getKerning(prevChar, curChar, m_characterSize, isBold);
        prevChar = curChar;

        // Handle special characters
        switch (curChar)
        {
            case U' ':
                position.x += whitespaceWidth;
                continue;
            case U'\t':
                position.x += whitespaceWidth * 4;
                continue;
            case U'\n':
                position.y += lineSpacing;
                position.x = 0;
                continue;
        }

        // For regular characters, add the advance offset of the glyph
        position.x += m_font->getGlyph(curChar, m_characterSize, isBold).advance + letterSpacing;
    }

    // Transform the position to global coordinates
    position = getTransform().transformPoint(position);

    return position;
}


////////////////////////////////////////////////////////////
FloatRect Text::getLocalBounds() const
{
    ensureGeometryUpdate();

    return m_bounds;
}


////////////////////////////////////////////////////////////
FloatRect Text::getGlobalBounds() const
{
    return getTransform().transformRect(getLocalBounds());
}


////////////////////////////////////////////////////////////
void Text::draw(RenderTarget& target, const RenderStates& states) const
{
    if (m_font)
    {
        ensureGeometryUpdate();

        RenderStates statesCopy(states);
        statesCopy.transform *= getTransform();

        // Only draw the outline if there is something to draw
        if (m_outlineThickness != 0)
        {
            for (auto& [texture, verts] : m_outlineVerticesMap)
            {
                if (verts.getVertexCount() > 0)
                {
                    statesCopy.texture = texture;
                    target.draw(verts, statesCopy);
                }
            }
        }

        for (auto& [texture, verts] : m_fillVerticesMap)
        {
            if (verts.getVertexCount() > 0)
            {
                statesCopy.texture = texture;
                target.draw(verts, statesCopy);
            }
        }
    }
}


////////////////////////////////////////////////////////////
void Text::ensureGeometryUpdate() const
{
    if (!m_font)
        return;

    // Do nothing, if geometry has not changed and the font textures have not changed
    const auto textureIds         = m_font->getTextureIds(m_characterSize);
    const auto textureNeedsUpdate = textureIds != m_fontTextureIds;
    if (!m_geometryNeedUpdate && !textureNeedsUpdate)
        return;

    // Save the current fonts texture ids
    m_fontTextureIds = textureIds;

    // Mark geometry as updated
    m_geometryNeedUpdate = false;

    if (textureNeedsUpdate)
    {
        m_fillVerticesMap.clear();
        m_outlineVerticesMap.clear();
    }
    else
    {
        // Clear the previous geometry but keep all VertexArray instances so they can reuse their allocated memory
        for (auto& verts : m_fillVerticesMap)
            verts.second.clear();
        for (auto& verts : m_outlineVerticesMap)
            verts.second.clear();
    }

    m_bounds = FloatRect();

    // No text: nothing to draw
    if (m_string.isEmpty())
        return;

    // Compute values related to the text style
    bool  isBold             = m_style & Bold;
    bool  isUnderlined       = m_style & Underlined;
    bool  isStrikeThrough    = m_style & StrikeThrough;
    float italicShear        = (m_style & Italic) ? sf::degrees(12).asRadians() : 0.f;
    float underlineOffset    = m_font->getUnderlinePosition(m_characterSize);
    float underlineThickness = m_font->getUnderlineThickness(m_characterSize);

    // Compute the location of the strike through dynamically
    // We use the center point of the lowercase 'x' glyph as the reference
    // We reuse the underline thickness as the thickness of the strike through as well
    FloatRect xBounds             = m_font->getGlyph(U'x', m_characterSize, isBold).bounds;
    float     strikeThroughOffset = xBounds.top + xBounds.height / 2.f;

    // Precompute the variables needed by the algorithm
    float whitespaceWidth = m_font->getGlyph(U' ', m_characterSize, isBold).advance;
    float letterSpacing   = (whitespaceWidth / 3.f) * (m_letterSpacingFactor - 1.f);
    whitespaceWidth += letterSpacing;
    float lineSpacing = m_font->getLineSpacing(m_characterSize) * m_lineSpacingFactor;
    float x           = 0.f;
    auto  y           = static_cast<float>(m_characterSize);

    // Create one quad for each character
    auto          minX     = static_cast<float>(m_characterSize);
    auto          minY     = static_cast<float>(m_characterSize);
    float         maxX     = 0.f;
    float         maxY     = 0.f;
    std::uint32_t prevChar = 0;
    for (std::size_t i = 0; i < m_string.getSize(); ++i)
    {
        std::uint32_t curChar = m_string[i];

        // Skip the \r char to avoid weird graphical issues
        if (curChar == U'\r')
            continue;

        // Apply the kerning offset
        x += m_font->getKerning(prevChar, curChar, m_characterSize, isBold);

        // If we're using the underlined style and there's a new line, draw a line
        if (isUnderlined && (curChar == U'\n' && prevChar != U'\n'))
        {
            addLine(m_fillVerticesMap[nullptr], x, y, m_fillColor, underlineOffset, underlineThickness);

            if (m_outlineThickness != 0)
                addLine(m_outlineVerticesMap[nullptr], x, y, m_outlineColor, underlineOffset, underlineThickness, m_outlineThickness);
        }

        // If we're using the strike through style and there's a new line, draw a line across all characters
        if (isStrikeThrough && (curChar == U'\n' && prevChar != U'\n'))
        {
            addLine(m_fillVerticesMap[nullptr], x, y, m_fillColor, strikeThroughOffset, underlineThickness);

            if (m_outlineThickness != 0)
                addLine(m_outlineVerticesMap[nullptr], x, y, m_outlineColor, strikeThroughOffset, underlineThickness, m_outlineThickness);
        }

        prevChar = curChar;

        // Handle special characters
        if ((curChar == U' ') || (curChar == U'\n') || (curChar == U'\t'))
        {
            // Update the current bounds (min coordinates)
            minX = std::min(minX, x);
            minY = std::min(minY, y);

            switch (curChar)
            {
                case U' ':
                    x += whitespaceWidth;
                    break;
                case U'\t':
                    x += whitespaceWidth * 4;
                    break;
                case U'\n':
                    y += lineSpacing;
                    x = 0;
                    break;
            }

            // Update the current bounds (max coordinates)
            maxX = std::max(maxX, x);
            maxY = std::max(maxY, y);

            // Next glyph, no need to create a quad for whitespace
            continue;
        }

        // Apply the outline
        if (m_outlineThickness != 0)
        {
            const Glyph& glyph = m_font->getGlyph(curChar, m_characterSize, isBold, m_outlineThickness);

            // Add the outline glyph to the vertices
            addGlyphQuad(m_outlineVerticesMap[glyph.texture], Vector2f(x, y), m_outlineColor, glyph, italicShear);
        }

        // Extract the current glyph's description
        const Glyph& glyph = m_font->getGlyph(curChar, m_characterSize, isBold);

        // Add the glyph to the vertices
        addGlyphQuad(m_fillVerticesMap[glyph.texture], Vector2f(x, y), m_fillColor, glyph, italicShear);

        // Update the current bounds
        float left   = glyph.bounds.left;
        float top    = glyph.bounds.top;
        float right  = glyph.bounds.left + glyph.bounds.width;
        float bottom = glyph.bounds.top + glyph.bounds.height;

        minX = std::min(minX, x + left - italicShear * bottom);
        maxX = std::max(maxX, x + right - italicShear * top);
        minY = std::min(minY, y + top);
        maxY = std::max(maxY, y + bottom);

        // Advance to the next character
        x += glyph.advance + letterSpacing;
    }

    // If we're using outline, update the current bounds
    if (m_outlineThickness != 0)
    {
        float outline = std::abs(std::ceil(m_outlineThickness));
        minX -= outline;
        maxX += outline;
        minY -= outline;
        maxY += outline;
    }

    // If we're using the underlined style, add the last line
    if (isUnderlined && (x > 0))
    {
        addLine(m_fillVerticesMap[nullptr], x, y, m_fillColor, underlineOffset, underlineThickness);

        if (m_outlineThickness != 0)
            addLine(m_outlineVerticesMap[nullptr], x, y, m_outlineColor, underlineOffset, underlineThickness, m_outlineThickness);
    }

    // If we're using the strike through style, add the last line across all characters
    if (isStrikeThrough && (x > 0))
    {
        addLine(m_fillVerticesMap[nullptr], x, y, m_fillColor, strikeThroughOffset, underlineThickness);

        if (m_outlineThickness != 0)
            addLine(m_outlineVerticesMap[nullptr], x, y, m_outlineColor, strikeThroughOffset, underlineThickness, m_outlineThickness);
    }

    // Update the bounding rectangle
    m_bounds.left   = minX;
    m_bounds.top    = minY;
    m_bounds.width  = maxX - minX;
    m_bounds.height = maxY - minY;

    // Finally, set type of primitives to triangles
    for (auto& vertMap : m_fillVerticesMap)
        vertMap.second.setPrimitiveType(PrimitiveType::Triangles);
    for (auto& vertMap : m_outlineVerticesMap)
        vertMap.second.setPrimitiveType(PrimitiveType::Triangles);
}

} // namespace sf
