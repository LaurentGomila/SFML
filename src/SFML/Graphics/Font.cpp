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
#include <SFML/Graphics/GLCheck.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>
#ifdef SFML_SYSTEM_ANDROID
#include <SFML/System/Android/ResourceStream.hpp>
#endif
#include <SFML/System/Err.hpp>
#include <SFML/System/InputStream.hpp>
#include <SFML/System/Utils.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_BITMAP_H
#include FT_STROKER_H
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <type_traits>


namespace
{
// FreeType callbacks that operate on a sf::InputStream
unsigned long read(FT_Stream rec, unsigned long offset, unsigned char* buffer, unsigned long count)
{
    auto  convertedOffset = static_cast<std::int64_t>(offset);
    auto* stream          = static_cast<sf::InputStream*>(rec->descriptor.pointer);
    if (stream->seek(convertedOffset) == convertedOffset)
    {
        if (count > 0)
            return static_cast<unsigned long>(
                stream->read(reinterpret_cast<char*>(buffer), static_cast<std::int64_t>(count)));
        else
            return 0;
    }
    else
        return count > 0 ? 0 : 1; // error code is 0 if we're reading, or nonzero if we're seeking
}
void close(FT_Stream)
{
}

// Helper to intepret memory as a specific type
template <typename T, typename U>
inline T reinterpret(const U& input)
{
    T output;
    std::memcpy(&output, &input, sizeof(U));
    return output;
}

// Combine outline thickness, boldness and font glyph index into a single 64-bit key
std::uint64_t combine(float outlineThickness, bool bold, std::uint32_t index)
{
    return (static_cast<std::uint64_t>(reinterpret<std::uint32_t>(outlineThickness)) << 32) |
           (static_cast<std::uint64_t>(bold) << 31) | index;
}
} // namespace


namespace sf
{
////////////////////////////////////////////////////////////
class Font::FontHandles
{
private:
    // Default constructible deleter functor
    struct Deleter
    {
        void operator()(FT_Library theLibrary)
        {
            FT_Done_FreeType(theLibrary);
        }
        void operator()(FT_Face theFace)
        {
            FT_Done_Face(theFace);
        }
        void operator()(FT_Stroker theStroker)
        {
            FT_Stroker_Done(theStroker);
        }
    };

public:
    std::unique_ptr<std::remove_pointer_t<FT_Library>, Deleter> library;   //< Pointer to the internal library interface
    std::unique_ptr<FT_StreamRec>                               streamRec; //< Pointer to the stream rec instance
    std::unique_ptr<std::remove_pointer_t<FT_Face>, Deleter>    face;      //< Pointer to the internal font face
    std::unique_ptr<std::remove_pointer_t<FT_Stroker>, Deleter> stroker;   //< Pointer to the stroker
};


////////////////////////////////////////////////////////////
Font::Font() : m_fontHandles(), m_isSmooth(true), m_info()
{
}


////////////////////////////////////////////////////////////
Font::Font(const Font& copy) :
m_fontHandles(copy.m_fontHandles),
m_isSmooth(copy.m_isSmooth),
m_info(copy.m_info),
m_pageLists(copy.m_pageLists),
m_pixelBuffer(copy.m_pixelBuffer)
{
}


////////////////////////////////////////////////////////////
Font::~Font() = default;


////////////////////////////////////////////////////////////
Font::Font(Font&&) noexcept = default;


////////////////////////////////////////////////////////////
Font& Font::operator=(Font&&) noexcept = default;


////////////////////////////////////////////////////////////
bool Font::loadFromFile(const std::filesystem::path& filename)
{
#ifndef SFML_SYSTEM_ANDROID

    // Cleanup the previous resources
    cleanup();

    auto fontHandles = std::make_unique<FontHandles>();

    // Initialize FreeType
    // Note: we initialize FreeType for every font instance in order to avoid having a single
    // global manager that would create a lot of issues regarding creation and destruction order.
    FT_Library library;
    if (FT_Init_FreeType(&library) != 0)
    {
        err() << "Failed to load font (failed to initialize FreeType)\n" << formatDebugPathInfo(filename) << std::endl;
        return false;
    }
    fontHandles->library.reset(library);

    // Load the new font face from the specified file
    FT_Face face;
    if (FT_New_Face(library, filename.string().c_str(), 0, &face) != 0)
    {
        err() << "Failed to load font (failed to create the font face)\n" << formatDebugPathInfo(filename) << std::endl;
        return false;
    }
    fontHandles->face.reset(face);

    // Load the stroker that will be used to outline the font
    FT_Stroker stroker;
    if (FT_Stroker_New(library, &stroker) != 0)
    {
        err() << "Failed to load font (failed to create the stroker)\n" << formatDebugPathInfo(filename) << std::endl;
        return false;
    }
    fontHandles->stroker.reset(stroker);

    // Select the unicode character map
    if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) != 0)
    {
        err() << "Failed to load font (failed to set the Unicode character set)\n"
              << formatDebugPathInfo(filename) << std::endl;
        return false;
    }

    // Store the loaded font handles
    m_fontHandles = std::move(fontHandles);

    // Store the font information
    m_info.family = face->family_name ? face->family_name : std::string();

    return true;

#else

    m_stream = std::make_unique<priv::ResourceStream>(filename);
    return loadFromStream(*m_stream);

#endif
}


////////////////////////////////////////////////////////////
bool Font::loadFromMemory(const void* data, std::size_t sizeInBytes)
{
    // Cleanup the previous resources
    cleanup();

    auto fontHandles = std::make_unique<FontHandles>();

    // Initialize FreeType
    // Note: we initialize FreeType for every font instance in order to avoid having a single
    // global manager that would create a lot of issues regarding creation and destruction order.
    FT_Library library;
    if (FT_Init_FreeType(&library) != 0)
    {
        err() << "Failed to load font from memory (failed to initialize FreeType)" << std::endl;
        return false;
    }
    fontHandles->library.reset(library);

    // Load the new font face from the specified file
    FT_Face face;
    if (FT_New_Memory_Face(library, reinterpret_cast<const FT_Byte*>(data), static_cast<FT_Long>(sizeInBytes), 0, &face) != 0)
    {
        err() << "Failed to load font from memory (failed to create the font face)" << std::endl;
        return false;
    }
    fontHandles->face.reset(face);

    // Load the stroker that will be used to outline the font
    FT_Stroker stroker;
    if (FT_Stroker_New(library, &stroker) != 0)
    {
        err() << "Failed to load font from memory (failed to create the stroker)" << std::endl;
        return false;
    }
    fontHandles->stroker.reset(stroker);

    // Select the Unicode character map
    if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) != 0)
    {
        err() << "Failed to load font from memory (failed to set the Unicode character set)" << std::endl;
        return false;
    }

    // Store the loaded font handles
    m_fontHandles = std::move(fontHandles);

    // Store the font information
    m_info.family = face->family_name ? face->family_name : std::string();

    return true;
}


////////////////////////////////////////////////////////////
bool Font::loadFromStream(InputStream& stream)
{
    // Cleanup the previous resources
    cleanup();

    auto fontHandles = std::make_unique<FontHandles>();

    // Initialize FreeType
    // Note: we initialize FreeType for every font instance in order to avoid having a single
    // global manager that would create a lot of issues regarding creation and destruction order.
    FT_Library library;
    if (FT_Init_FreeType(&library) != 0)
    {
        err() << "Failed to load font from stream (failed to initialize FreeType)" << std::endl;
        return false;
    }
    fontHandles->library.reset(library);

    // Make sure that the stream's reading position is at the beginning
    if (stream.seek(0) == -1)
    {
        err() << "Failed to seek font stream" << std::endl;
        return false;
    }

    // Prepare a wrapper for our stream, that we'll pass to FreeType callbacks
    fontHandles->streamRec                     = std::make_unique<FT_StreamRec>();
    fontHandles->streamRec->base               = nullptr;
    fontHandles->streamRec->size               = static_cast<unsigned long>(stream.getSize());
    fontHandles->streamRec->pos                = 0;
    fontHandles->streamRec->descriptor.pointer = &stream;
    fontHandles->streamRec->read               = &read;
    fontHandles->streamRec->close              = &close;

    // Setup the FreeType callbacks that will read our stream
    FT_Open_Args args;
    args.flags  = FT_OPEN_STREAM;
    args.stream = fontHandles->streamRec.get();
    args.driver = nullptr;

    // Load the new font face from the specified stream
    FT_Face face;
    if (FT_Open_Face(library, &args, 0, &face) != 0)
    {
        err() << "Failed to load font from stream (failed to create the font face)" << std::endl;
        return false;
    }
    fontHandles->face.reset(face);

    // Load the stroker that will be used to outline the font
    FT_Stroker stroker;
    if (FT_Stroker_New(library, &stroker) != 0)
    {
        err() << "Failed to load font from stream (failed to create the stroker)" << std::endl;
        return false;
    }
    fontHandles->stroker.reset(stroker);

    // Select the Unicode character map
    if (FT_Select_Charmap(face, FT_ENCODING_UNICODE) != 0)
    {
        err() << "Failed to load font from stream (failed to set the Unicode character set)" << std::endl;
        return false;
    }

    // Store the loaded font handles
    m_fontHandles = std::move(fontHandles);

    // Store the font information
    m_info.family = face->family_name ? face->family_name : std::string();

    return true;
}


////////////////////////////////////////////////////////////
const Font::Info& Font::getInfo() const
{
    return m_info;
}


////////////////////////////////////////////////////////////
const Glyph& Font::getGlyph(std::uint32_t codePoint, unsigned int characterSize, bool bold, float outlineThickness) const
{
    // Get the pages list corresponding to the character size
    PageList& pages = m_pageLists[characterSize];

    // Build the key by combining the glyph index (based on code point), bold flag, and outline thickness
    std::uint64_t key = combine(outlineThickness,
                                bold,
                                FT_Get_Char_Index(m_fontHandles ? m_fontHandles->face.get() : nullptr, codePoint));

    // Search the glyph in the pages list
    for (const auto& page : pages)
    {
        // Search the glyph into the cache
        const auto& glyphs = page.glyphs;
        const auto  found  = glyphs.find(key);
        if (found != glyphs.end())
        {
            return found->second;
        }
    }

    // Not found: we have to load it
    Glyph glyph = loadGlyph(codePoint, characterSize, bold, outlineThickness);

    // Store the glyph in the page that contains it
    for (auto& page : pages)
        if (glyph.texture == &page.texture)
            return page.glyphs.emplace(key, glyph).first->second;

    // No page found: simply store the glyph in the first page
    assert(glyph.texture == nullptr);
    if (pages.empty())
        pages.emplace_back(m_isSmooth);
    return pages.front().glyphs.emplace(key, glyph).first->second;
}


////////////////////////////////////////////////////////////
bool Font::hasGlyph(std::uint32_t codePoint) const
{
    return FT_Get_Char_Index(m_fontHandles ? m_fontHandles->face.get() : nullptr, codePoint) != 0;
}


////////////////////////////////////////////////////////////
float Font::getKerning(std::uint32_t first, std::uint32_t second, unsigned int characterSize, bool bold) const
{
    // Special case where first or second is 0 (null character)
    if (first == 0 || second == 0)
        return 0.f;

    auto face = m_fontHandles ? m_fontHandles->face.get() : nullptr;

    if (face && setCurrentSize(characterSize))
    {
        // Convert the characters to indices
        FT_UInt index1 = FT_Get_Char_Index(face, first);
        FT_UInt index2 = FT_Get_Char_Index(face, second);

        // Retrieve position compensation deltas generated by FT_LOAD_FORCE_AUTOHINT flag
        auto firstRsbDelta  = static_cast<float>(getGlyph(first, characterSize, bold).rsbDelta);
        auto secondLsbDelta = static_cast<float>(getGlyph(second, characterSize, bold).lsbDelta);

        // Get the kerning vector if present
        FT_Vector kerning;
        kerning.x = kerning.y = 0;
        if (FT_HAS_KERNING(face))
            FT_Get_Kerning(face, index1, index2, FT_KERNING_UNFITTED, &kerning);

        // X advance is already in pixels for bitmap fonts
        if (!FT_IS_SCALABLE(face))
            return static_cast<float>(kerning.x);

        // Combine kerning with compensation deltas and return the X advance
        // Flooring is required as we use FT_KERNING_UNFITTED flag which is not quantized in 64 based grid
        return std::floor(
            (secondLsbDelta - firstRsbDelta + static_cast<float>(kerning.x) + 32) / static_cast<float>(1 << 6));
    }
    else
    {
        // Invalid font
        return 0.f;
    }
}


////////////////////////////////////////////////////////////
float Font::getLineSpacing(unsigned int characterSize) const
{
    auto face = m_fontHandles ? m_fontHandles->face.get() : nullptr;

    if (face && setCurrentSize(characterSize))
    {
        return static_cast<float>(face->size->metrics.height) / static_cast<float>(1 << 6);
    }
    else
    {
        return 0.f;
    }
}


////////////////////////////////////////////////////////////
float Font::getUnderlinePosition(unsigned int characterSize) const
{
    auto face = m_fontHandles ? m_fontHandles->face.get() : nullptr;

    if (face && setCurrentSize(characterSize))
    {
        // Return a fixed position if font is a bitmap font
        if (!FT_IS_SCALABLE(face))
            return static_cast<float>(characterSize) / 10.f;

        return -static_cast<float>(FT_MulFix(face->underline_position, face->size->metrics.y_scale)) /
               static_cast<float>(1 << 6);
    }
    else
    {
        return 0.f;
    }
}


////////////////////////////////////////////////////////////
float Font::getUnderlineThickness(unsigned int characterSize) const
{
    auto face = m_fontHandles ? m_fontHandles->face.get() : nullptr;

    if (face && setCurrentSize(characterSize))
    {
        // Return a fixed thickness if font is a bitmap font
        if (!FT_IS_SCALABLE(face))
            return static_cast<float>(characterSize) / 14.f;

        return static_cast<float>(FT_MulFix(face->underline_thickness, face->size->metrics.y_scale)) /
               static_cast<float>(1 << 6);
    }
    else
    {
        return 0.f;
    }
}

////////////////////////////////////////////////////////////
std::unordered_set<std::uint64_t> Font::getTextureIds(unsigned int characterSize) const
{
    PageList& pages = m_pageLists[characterSize];
    if (pages.empty())
        pages.emplace_back(m_isSmooth);

    std::unordered_set<std::uint64_t> textureIds;
    for (const auto& page : pages)
    {
        textureIds.emplace(page.texture.m_cacheId);
    }
    return textureIds;
}

////////////////////////////////////////////////////////////
void Font::setSmooth(bool smooth)
{
    if (smooth != m_isSmooth)
    {
        m_isSmooth = smooth;

        for (auto& [key, pageList] : m_pageLists)
        {
            for (auto& page : pageList)
            {
                page.texture.setSmooth(m_isSmooth);
            }
        }
    }
}

////////////////////////////////////////////////////////////
bool Font::isSmooth() const
{
    return m_isSmooth;
}

////////////////////////////////////////////////////////////
Font& Font::operator=(const Font& right)
{
    Font temp(right);

    std::swap(m_fontHandles, temp.m_fontHandles);
    std::swap(m_isSmooth, temp.m_isSmooth);
    std::swap(m_info, temp.m_info);
    std::swap(m_pageLists, temp.m_pageLists);
    std::swap(m_pixelBuffer, temp.m_pixelBuffer);

#ifdef SFML_SYSTEM_ANDROID
    std::swap(m_stream, temp.m_stream);
#endif

    return *this;
}


////////////////////////////////////////////////////////////
void Font::cleanup()
{
    // Drop ownership of shared FreeType pointers
    m_fontHandles.reset();

    // Reset members
    m_pageLists.clear();
    std::vector<std::uint8_t>().swap(m_pixelBuffer);
}


////////////////////////////////////////////////////////////
Glyph Font::loadGlyph(std::uint32_t codePoint, unsigned int characterSize, bool bold, float outlineThickness) const
{
    // The glyph to return
    Glyph glyph;

    // Stop if no font is loaded
    if (!m_fontHandles)
        return glyph;

    // Get our FT_Face
    auto face = m_fontHandles->face.get();
    if (!face)
        return glyph;

    // Set the character size
    if (!setCurrentSize(characterSize))
        return glyph;

    // Load the glyph corresponding to the code point
    FT_Int32 flags = FT_LOAD_TARGET_NORMAL | FT_LOAD_FORCE_AUTOHINT;
    if (outlineThickness != 0)
        flags |= FT_LOAD_NO_BITMAP;
    if (FT_Load_Char(face, codePoint, flags) != 0)
        return glyph;

    // Retrieve the glyph
    FT_Glyph glyphDesc;
    if (FT_Get_Glyph(face->glyph, &glyphDesc) != 0)
        return glyph;

    // Apply bold and outline (there is no fallback for outline) if necessary -- first technique using outline (highest quality)
    FT_Pos weight  = 1 << 6;
    bool   outline = (glyphDesc->format == FT_GLYPH_FORMAT_OUTLINE);
    if (outline)
    {
        if (bold)
        {
            auto outlineGlyph = reinterpret_cast<FT_OutlineGlyph>(glyphDesc);
            FT_Outline_Embolden(&outlineGlyph->outline, weight);
        }

        if (outlineThickness != 0)
        {
            auto stroker = m_fontHandles->stroker.get();

            FT_Stroker_Set(stroker,
                           static_cast<FT_Fixed>(outlineThickness * static_cast<float>(1 << 6)),
                           FT_STROKER_LINECAP_ROUND,
                           FT_STROKER_LINEJOIN_ROUND,
                           0);
            FT_Glyph_Stroke(&glyphDesc, stroker, true);
        }
    }

    // Convert the glyph to a bitmap (i.e. rasterize it)
    // Warning! After this line, do not read any data from glyphDesc directly, use
    // bitmapGlyph.root to access the FT_Glyph data.
    FT_Glyph_To_Bitmap(&glyphDesc, FT_RENDER_MODE_NORMAL, nullptr, 1);
    auto       bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyphDesc);
    FT_Bitmap& bitmap      = bitmapGlyph->bitmap;

    // Apply bold if necessary -- fallback technique using bitmap (lower quality)
    if (!outline)
    {
        if (bold)
            FT_Bitmap_Embolden(m_fontHandles->library.get(), &bitmap, weight, weight);

        if (outlineThickness != 0)
            err() << "Failed to outline glyph (no fallback available)" << std::endl;
    }

    // Compute the glyph's advance offset
    glyph.advance = static_cast<float>(bitmapGlyph->root.advance.x >> 16);
    if (bold)
        glyph.advance += static_cast<float>(weight) / static_cast<float>(1 << 6);

    glyph.lsbDelta = static_cast<int>(face->glyph->lsb_delta);
    glyph.rsbDelta = static_cast<int>(face->glyph->rsb_delta);

    unsigned int width  = bitmap.width;
    unsigned int height = bitmap.rows;

    if ((width > 0) && (height > 0))
    {
        // Leave a small padding around characters, so that filtering doesn't
        // pollute them with pixels from neighbors
        const unsigned int padding = 2;

        width += 2 * padding;
        height += 2 * padding;

        // Get the pages list corresponding to the character size
        PageList& pages = m_pageLists[characterSize];

        // Find a page that can fit well the glyph
        Page* foundPage = nullptr;
        for (auto& page : pages)
        {
            // Try to find a good position for the new glyph into the texture
            glyph.textureRect = findGlyphRect(page, {width, height});

            if (glyph.textureRect.width > 0)
            {
                foundPage = &page;
                break;
            }
        }

        // If we didn't find a matching page, create a new one
        if (!foundPage)
        {
            foundPage = &pages.emplace_back(m_isSmooth);

            // Try to find a good position for the new glyph into the texture
            glyph.textureRect = findGlyphRect(*foundPage, {width, height});

            if (glyph.textureRect.width == 0)
            {
                // Oops, we've reached the maximum texture size even for a single glyph...
                err() << "Failed to add a new character to the font: the maximum texture size has been reached"
                      << std::endl;
                glyph.textureRect = IntRect({0, 0}, {2, 2});
            }
        }

        glyph.texture = &foundPage->texture;

        // Make sure the texture data is positioned in the center
        // of the allocated texture rectangle
        glyph.textureRect.left += static_cast<int>(padding);
        glyph.textureRect.top += static_cast<int>(padding);
        glyph.textureRect.width -= static_cast<int>(2 * padding);
        glyph.textureRect.height -= static_cast<int>(2 * padding);

        // Compute the glyph's bounding box
        glyph.bounds.left   = static_cast<float>(bitmapGlyph->left);
        glyph.bounds.top    = static_cast<float>(-bitmapGlyph->top);
        glyph.bounds.width  = static_cast<float>(bitmap.width);
        glyph.bounds.height = static_cast<float>(bitmap.rows);

        // Resize the pixel buffer to the new size and fill it with transparent white pixels
        m_pixelBuffer.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);

        std::uint8_t* current = m_pixelBuffer.data();
        std::uint8_t* end     = current + width * height * 4;

        while (current != end)
        {
            (*current++) = 255;
            (*current++) = 255;
            (*current++) = 255;
            (*current++) = 0;
        }

        // Extract the glyph's pixels from the bitmap
        const std::uint8_t* pixels = bitmap.buffer;
        if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
        {
            // Pixels are 1 bit monochrome values
            for (unsigned int y = padding; y < height - padding; ++y)
            {
                for (unsigned int x = padding; x < width - padding; ++x)
                {
                    // The color channels remain white, just fill the alpha channel
                    std::size_t index            = x + y * width;
                    m_pixelBuffer[index * 4 + 3] = ((pixels[(x - padding) / 8]) & (1 << (7 - ((x - padding) % 8)))) ? 255 : 0;
                }
                pixels += bitmap.pitch;
            }
        }
        else
        {
            // Pixels are 8 bits gray levels
            for (unsigned int y = padding; y < height - padding; ++y)
            {
                for (unsigned int x = padding; x < width - padding; ++x)
                {
                    // The color channels remain white, just fill the alpha channel
                    std::size_t index            = x + y * width;
                    m_pixelBuffer[index * 4 + 3] = pixels[x - padding];
                }
                pixels += bitmap.pitch;
            }
        }

        // Write the pixels to the texture
        unsigned int x = static_cast<unsigned int>(glyph.textureRect.left) - padding;
        unsigned int y = static_cast<unsigned int>(glyph.textureRect.top) - padding;
        unsigned int w = static_cast<unsigned int>(glyph.textureRect.width) + 2 * padding;
        unsigned int h = static_cast<unsigned int>(glyph.textureRect.height) + 2 * padding;
        foundPage->texture.update(m_pixelBuffer.data(), {w, h}, {x, y});
    }

    // Delete the FT glyph
    FT_Done_Glyph(glyphDesc);

    // Done :)
    return glyph;
}


////////////////////////////////////////////////////////////
IntRect Font::findGlyphRect(Page& page, const Vector2u& size) const
{
    // Find the line that fits well the glyph
    Row*  row       = nullptr;
    float bestRatio = 0;
    for (auto it = page.rows.begin(); it != page.rows.end() && !row; ++it)
    {
        float ratio = static_cast<float>(size.y) / static_cast<float>(it->height);

        // Ignore rows that are either too small or too high
        if ((ratio < 0.7f) || (ratio > 1.f))
            continue;

        // Check if there's enough horizontal space left in the row
        if (size.x > page.texture.getSize().x - it->width)
            continue;

        // Make sure that this new row is the best found so far
        if (ratio < bestRatio)
            continue;

        // The current row passed all the tests: we can select it
        row       = &*it;
        bestRatio = ratio;
    }

    // If we didn't find a matching row, create a new one (10% taller than the glyph)
    if (!row)
    {
        unsigned int rowHeight = size.y + size.y / 10;
        while ((page.nextRow + rowHeight >= page.texture.getSize().y) || (size.x >= page.texture.getSize().x))
        {
            // Not enough space: resize the texture if possible
            Vector2u textureSize = page.texture.getSize();
            if ((textureSize.x * 2 <= Texture::getMaximumSize()) && (textureSize.y * 2 <= Texture::getMaximumSize()))
            {
                // Make the texture 2 times bigger
                Texture newTexture;
                if (!newTexture.create(textureSize * 2u))
                {
                    err() << "Failed to create new page texture" << std::endl;
                    return IntRect({0, 0}, {2, 2});
                }

                newTexture.setSmooth(m_isSmooth);
                newTexture.update(page.texture);
                page.texture.swap(newTexture);
            }
            else
            {
                // We've reached the maximum texture size, so return an empty rect
                return {};
            }
        }

        // We can now create the new row
        page.rows.emplace_back(page.nextRow, rowHeight);
        page.nextRow += rowHeight;
        row = &page.rows.back();
    }

    // Find the glyph's rectangle on the selected row
    IntRect rect(Rect<unsigned int>({row->width, row->top}, size));

    // Update the row informations
    row->width += size.x;

    return rect;
}


////////////////////////////////////////////////////////////
bool Font::setCurrentSize(unsigned int characterSize) const
{
    // FT_Set_Pixel_Sizes is an expensive function, so we must call it
    // only when necessary to avoid killing performances

    // m_fontHandles and m_fontHandles->face are checked to be non-null before calling this method
    auto      face        = m_fontHandles->face.get();
    FT_UShort currentSize = face->size->metrics.x_ppem;

    if (currentSize != characterSize)
    {
        FT_Error result = FT_Set_Pixel_Sizes(face, 0, characterSize);

        if (result == FT_Err_Invalid_Pixel_Size)
        {
            // In the case of bitmap fonts, resizing can
            // fail if the requested size is not available
            if (!FT_IS_SCALABLE(face))
            {
                err() << "Failed to set bitmap font size to " << characterSize << std::endl;
                err() << "Available sizes are: ";
                for (int i = 0; i < face->num_fixed_sizes; ++i)
                {
                    const long size = (face->available_sizes[i].y_ppem + 32) >> 6;
                    err() << size << " ";
                }
                err() << std::endl;
            }
            else
            {
                err() << "Failed to set font size to " << characterSize << std::endl;
            }
        }

        return result == FT_Err_Ok;
    }

    return true;
}


////////////////////////////////////////////////////////////
Font::Page::Page(bool smooth) : nextRow(3)
{
    // Make sure that the texture is initialized by default
    sf::Image image;
    image.create({128, 128}, Color(255, 255, 255, 0));

    // Reserve a 2x2 white square for texturing underlines
    for (unsigned int x = 0; x < 2; ++x)
        for (unsigned int y = 0; y < 2; ++y)
            image.setPixel({x, y}, Color(255, 255, 255, 255));

    // Create the texture
    if (!texture.loadFromImage(image))
    {
        err() << "Failed to load font page texture" << std::endl;
    }

    texture.setSmooth(smooth);
}

} // namespace sf
