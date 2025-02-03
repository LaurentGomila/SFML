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

#pragma once

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Graphics/Export.hpp>

#include <SFML/Graphics/CoordinateType.hpp>
#include <SFML/Graphics/Rect.hpp>

#include <SFML/Window/GlResource.hpp>

#include <SFML/System/Vector2.hpp>

#include <filesystem>

#include <cstddef>
#include <cstdint>


namespace sf
{
class InputStream;
class Window;
class Image;

////////////////////////////////////////////////////////////
/// \brief Image living on the graphics card that can be used for drawing
///
////////////////////////////////////////////////////////////
class SFML_GRAPHICS_API Texture : GlResource
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor
    ///
    /// Creates a texture with width 0 and height 0.
    ///
    /// \see `resize`
    ///
    ////////////////////////////////////////////////////////////
    Texture();

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~Texture();

    ////////////////////////////////////////////////////////////
    /// \brief Copy constructor
    ///
    /// \param copy instance to copy
    ///
    ////////////////////////////////////////////////////////////
    Texture(const Texture& copy);

    ////////////////////////////////////////////////////////////
    /// \brief Copy assignment operator
    ///
    ////////////////////////////////////////////////////////////
    Texture& operator=(const Texture&);

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    Texture(Texture&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment operator
    ///
    ////////////////////////////////////////////////////////////
    Texture& operator=(Texture&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Construct the texture from a file on disk
    ///
    /// The maximum size for a texture depends on the graphics
    /// driver and can be retrieved with the getMaximumSize function.
    ///
    /// \param filename Path of the image file to load
    /// \param sRgb     `true` to enable sRGB conversion, `false` to disable it
    ///
    /// \throws sf::Exception if loading was unsuccessful
    ///
    /// \see `loadFromFile`, `loadFromMemory`, `loadFromStream`, `loadFromImage`
    ///
    ////////////////////////////////////////////////////////////
    explicit Texture(const std::filesystem::path& filename, bool sRgb = false);

    ////////////////////////////////////////////////////////////
    /// \brief Construct the texture from a sub-rectangle of a file on disk
    ///
    /// The `area` argument can be used to load only a sub-rectangle
    /// of the whole image. If you want the entire image then leave
    /// the default value (which is an empty `IntRect`).
    /// If the `area` rectangle crosses the bounds of the image, it
    /// is adjusted to fit the image size.
    ///
    /// The maximum size for a texture depends on the graphics
    /// driver and can be retrieved with the `getMaximumSize` function.
    ///
    /// \param filename Path of the image file to load
    /// \param sRgb     `true` to enable sRGB conversion, `false` to disable it
    /// \param area     Area of the image to load
    ///
    /// \throws sf::Exception if loading was unsuccessful
    ///
    /// \see `loadFromFile`, `loadFromMemory`, `loadFromStream`, `loadFromImage`
    ///
    ////////////////////////////////////////////////////////////
    Texture(const std::filesystem::path& filename, bool sRgb, const IntRect& area);

    ////////////////////////////////////////////////////////////
    /// \brief Construct the texture from a file in memory
    ///
    /// The maximum size for a texture depends on the graphics
    /// driver and can be retrieved with the `getMaximumSize` function.
    ///
    /// \param data Pointer to the file data in memory
    /// \param size Size of the data to load, in bytes
    /// \param sRgb `true` to enable sRGB conversion, `false` to disable it
    ///
    /// \throws sf::Exception if loading was unsuccessful
    ///
    /// \see `loadFromFile`, `loadFromMemory`, `loadFromStream`, `loadFromImage`
    ///
    ////////////////////////////////////////////////////////////
    Texture(const void* data, std::size_t size, bool sRgb = false);

    ////////////////////////////////////////////////////////////
    /// \brief Construct the texture from a sub-rectangle of a file in memory
    ///
    /// The `area` argument can be used to load only a sub-rectangle
    /// of the whole image. If you want the entire image then leave
    /// the default value (which is an empty `IntRect`).
    /// If the `area` rectangle crosses the bounds of the image, it
    /// is adjusted to fit the image size.
    ///
    /// The maximum size for a texture depends on the graphics
    /// driver and can be retrieved with the `getMaximumSize` function.
    ///
    /// \param data Pointer to the file data in memory
    /// \param size Size of the data to load, in bytes
    /// \param sRgb `true` to enable sRGB conversion, `false` to disable it
    /// \param area Area of the image to load
    ///
    /// \throws sf::Exception if loading was unsuccessful
    ///
    /// \see `loadFromFile`, `loadFromMemory`, `loadFromStream`, `loadFromImage`
    ///
    ////////////////////////////////////////////////////////////
    Texture(const void* data, std::size_t size, bool sRgb, const IntRect& area);

    ////////////////////////////////////////////////////////////
    /// \brief Construct the texture from a custom stream
    ///
    /// The maximum size for a texture depends on the graphics
    /// driver and can be retrieved with the `getMaximumSize` function.
    ///
    /// \param stream Source stream to read from
    /// \param sRgb   `true` to enable sRGB conversion, `false` to disable it
    ///
    /// \throws sf::Exception if loading was unsuccessful
    ///
    /// \see `loadFromFile`, `loadFromMemory`, `loadFromStream`, `loadFromImage`
    ///
    ////////////////////////////////////////////////////////////
    explicit Texture(InputStream& stream, bool sRgb = false);

    ////////////////////////////////////////////////////////////
    /// \brief Construct the texture from a sub-rectangle of a custom stream
    ///
    /// The `area` argument can be used to load only a sub-rectangle
    /// of the whole image. If you want the entire image then leave
    /// the default value (which is an empty `IntRect`).
    /// If the `area` rectangle crosses the bounds of the image, it
    /// is adjusted to fit the image size.
    ///
    /// The maximum size for a texture depends on the graphics
    /// driver and can be retrieved with the `getMaximumSize` function.
    ///
    /// \param stream Source stream to read from
    /// \param sRgb   `true` to enable sRGB conversion, `false` to disable it
    /// \param area   Area of the image to load
    ///
    /// \throws sf::Exception if loading was unsuccessful
    ///
    /// \see `loadFromFile`, `loadFromMemory`, `loadFromStream`, `loadFromImage`
    ///
    ////////////////////////////////////////////////////////////
    Texture(InputStream& stream, bool sRgb, const IntRect& area);

    ////////////////////////////////////////////////////////////
    /// \brief Construct the texture from an image
    ///
    /// The maximum size for a texture depends on the graphics
    /// driver and can be retrieved with the `getMaximumSize` function.
    ///
    /// \param image Image to load into the texture
    /// \param sRgb  `true` to enable sRGB conversion, `false` to disable it
    ///
    /// \throws sf::Exception if loading was unsuccessful
    ///
    /// \see `loadFromFile`, `loadFromMemory`, `loadFromStream`, `loadFromImage`
    ///
    ////////////////////////////////////////////////////////////
    explicit Texture(const Image& image, bool sRgb = false);

    ////////////////////////////////////////////////////////////
    /// \brief Construct the texture from a sub-rectangle of an image
    ///
    /// The `area` argument is used to load only a sub-rectangle
    /// of the whole image.
    /// If the `area` rectangle crosses the bounds of the image, it
    /// is adjusted to fit the image size.
    ///
    /// The maximum size for a texture depends on the graphics
    /// driver and can be retrieved with the `getMaximumSize` function.
    ///
    /// \param image Image to load into the texture
    /// \param sRgb  `true` to enable sRGB conversion, `false` to disable it
    /// \param area  Area of the image to load
    ///
    /// \throws sf::Exception if loading was unsuccessful
    ///
    /// \see `loadFromFile`, `loadFromMemory`, `loadFromStream`, `loadFromImage`
    ///
    ////////////////////////////////////////////////////////////
    Texture(const Image& image, bool sRgb, const IntRect& area);

    ////////////////////////////////////////////////////////////
    /// \brief Construct the texture with a given size
    ///
    /// \param size Width and height of the texture
    /// \param sRgb `true` to enable sRGB conversion, `false` to disable it
    ///
    /// \throws sf::Exception if construction was unsuccessful
    ///
    ////////////////////////////////////////////////////////////
    explicit Texture(Vector2u size, bool sRgb = false);

    ////////////////////////////////////////////////////////////
    /// \brief Resize the texture
    ///
    /// If this function fails, the texture is left unchanged.
    ///
    /// \param size Width and height of the texture
    /// \param sRgb `true` to enable sRGB conversion, `false` to disable it
    ///
    /// \return `true` if resizing was successful, `false` if it failed
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool resize(Vector2u size, bool sRgb = false);

    ////////////////////////////////////////////////////////////
    /// \brief Load the texture from a file on disk
    ///
    /// The `area` argument can be used to load only a sub-rectangle
    /// of the whole image. If you want the entire image then leave
    /// the default value (which is an empty `IntRect`).
    /// If the `area` rectangle crosses the bounds of the image, it
    /// is adjusted to fit the image size.
    ///
    /// The maximum size for a texture depends on the graphics
    /// driver and can be retrieved with the `getMaximumSize` function.
    ///
    /// If this function fails, the texture is left unchanged.
    ///
    /// \param filename Path of the image file to load
    /// \param sRgb     `true` to enable sRGB conversion, `false` to disable it
    /// \param area     Area of the image to load
    ///
    /// \return `true` if loading was successful, `false` if it failed
    ///
    /// \see `loadFromMemory`, `loadFromStream`, `loadFromImage`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool loadFromFile(const std::filesystem::path& filename, bool sRgb = false, const IntRect& area = {});

    ////////////////////////////////////////////////////////////
    /// \brief Load the texture from a file in memory
    ///
    /// The `area` argument can be used to load only a sub-rectangle
    /// of the whole image. If you want the entire image then leave
    /// the default value (which is an empty `IntRect`).
    /// If the `area` rectangle crosses the bounds of the image, it
    /// is adjusted to fit the image size.
    ///
    /// The maximum size for a texture depends on the graphics
    /// driver and can be retrieved with the `getMaximumSize` function.
    ///
    /// If this function fails, the texture is left unchanged.
    ///
    /// \param data Pointer to the file data in memory
    /// \param size Size of the data to load, in bytes
    /// \param sRgb `true` to enable sRGB conversion, `false` to disable it
    /// \param area Area of the image to load
    ///
    /// \return `true` if loading was successful, `false` if it failed
    ///
    /// \see `loadFromFile`, `loadFromStream`, `loadFromImage`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool loadFromMemory(const void* data, std::size_t size, bool sRgb = false, const IntRect& area = {});

    ////////////////////////////////////////////////////////////
    /// \brief Load the texture from a custom stream
    ///
    /// The `area` argument can be used to load only a sub-rectangle
    /// of the whole image. If you want the entire image then leave
    /// the default value (which is an empty `IntRect`).
    /// If the `area` rectangle crosses the bounds of the image, it
    /// is adjusted to fit the image size.
    ///
    /// The maximum size for a texture depends on the graphics
    /// driver and can be retrieved with the `getMaximumSize` function.
    ///
    /// If this function fails, the texture is left unchanged.
    ///
    /// \param stream Source stream to read from
    /// \param sRgb   `true` to enable sRGB conversion, `false` to disable it
    /// \param area   Area of the image to load
    ///
    /// \return `true` if loading was successful, `false` if it failed
    ///
    /// \see `loadFromFile`, `loadFromMemory`, `loadFromImage`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool loadFromStream(InputStream& stream, bool sRgb = false, const IntRect& area = {});

    ////////////////////////////////////////////////////////////
    /// \brief Load the texture from an image
    ///
    /// The `area` argument can be used to load only a sub-rectangle
    /// of the whole image. If you want the entire image then leave
    /// the default value (which is an empty `IntRect`).
    /// If the `area` rectangle crosses the bounds of the image, it
    /// is adjusted to fit the image size.
    ///
    /// The maximum size for a texture depends on the graphics
    /// driver and can be retrieved with the `getMaximumSize` function.
    ///
    /// If this function fails, the texture is left unchanged.
    ///
    /// \param image Image to load into the texture
    /// \param sRgb  `true` to enable sRGB conversion, `false` to disable it
    /// \param area  Area of the image to load
    ///
    /// \return `true` if loading was successful, `false` if it failed
    ///
    /// \see `loadFromFile`, `loadFromMemory`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool loadFromImage(const Image& image, bool sRgb = false, const IntRect& area = {});

    ////////////////////////////////////////////////////////////
    /// \brief Return the size of the texture
    ///
    /// \return Size in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vector2u getSize() const;

    ////////////////////////////////////////////////////////////
    /// \brief Copy the texture pixels to an image
    ///
    /// This function performs a slow operation that downloads
    /// the texture's pixels from the graphics card and copies
    /// them to a new image, potentially applying transformations
    /// to pixels if necessary (texture may be padded or flipped).
    ///
    /// \return Image containing the texture's pixels
    ///
    /// \see `loadFromImage`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Image copyToImage() const;

    ////////////////////////////////////////////////////////////
    /// \brief Update the whole texture from an array of pixels
    ///
    /// The pixel array is assumed to have the same size as
    /// the `area` rectangle, and to contain 32-bits RGBA pixels.
    ///
    /// No additional check is performed on the size of the pixel
    /// array. Passing invalid arguments will lead to an undefined
    /// behavior.
    ///
    /// This function does nothing if `pixels` is `nullptr`
    /// or if the texture was not previously created.
    ///
    /// \param pixels Array of pixels to copy to the texture
    ///
    ////////////////////////////////////////////////////////////
    void update(const std::uint8_t* pixels);

    ////////////////////////////////////////////////////////////
    /// \brief Update a part of the texture from an array of pixels
    ///
    /// The size of the pixel array must match the `size` argument,
    /// and it must contain 32-bits RGBA pixels.
    ///
    /// No additional check is performed on the size of the pixel
    /// array or the bounds of the area to update. Passing invalid
    /// arguments will lead to an undefined behavior.
    ///
    /// This function does nothing if `pixels` is null or if the
    /// texture was not previously created.
    ///
    /// \param pixels Array of pixels to copy to the texture
    /// \param size   Width and height of the pixel region contained in `pixels`
    /// \param dest   Coordinates of the destination position
    ///
    ////////////////////////////////////////////////////////////
    void update(const std::uint8_t* pixels, Vector2u size, Vector2u dest);

    ////////////////////////////////////////////////////////////
    /// \brief Update a part of this texture from another texture
    ///
    /// Although the source texture can be smaller than this texture,
    /// this function is usually used for updating the whole texture.
    /// The other overload, which has an additional destination
    /// argument, is more convenient for updating a sub-area of this
    /// texture.
    ///
    /// No additional check is performed on the size of the passed
    /// texture. Passing a texture bigger than this texture
    /// will lead to an undefined behavior.
    ///
    /// This function does nothing if either texture was not
    /// previously created.
    ///
    /// \param texture Source texture to copy to this texture
    ///
    ////////////////////////////////////////////////////////////
    void update(const Texture& texture);

    ////////////////////////////////////////////////////////////
    /// \brief Update a part of this texture from another texture
    ///
    /// No additional check is performed on the size of the texture.
    /// Passing an invalid combination of texture size and destination
    /// will lead to an undefined behavior.
    ///
    /// This function does nothing if either texture was not
    /// previously created.
    ///
    /// \param texture Source texture to copy to this texture
    /// \param dest    Coordinates of the destination position
    ///
    ////////////////////////////////////////////////////////////
    void update(const Texture& texture, Vector2u dest);

    ////////////////////////////////////////////////////////////
    /// \brief Update the texture from an image
    ///
    /// Although the source image can be smaller than the texture,
    /// this function is usually used for updating the whole texture.
    /// The other overload, which has an additional destination
    /// argument, is more convenient for updating a sub-area of the
    /// texture.
    ///
    /// No additional check is performed on the size of the image.
    /// Passing an image bigger than the texture will lead to an
    /// undefined behavior.
    ///
    /// This function does nothing if the texture was not
    /// previously created.
    ///
    /// \param image Image to copy to the texture
    ///
    ////////////////////////////////////////////////////////////
    void update(const Image& image);

    ////////////////////////////////////////////////////////////
    /// \brief Update a part of the texture from an image
    ///
    /// No additional check is performed on the size of the image.
    /// Passing an invalid combination of image size and destination
    /// will lead to an undefined behavior.
    ///
    /// This function does nothing if the texture was not
    /// previously created.
    ///
    /// \param image Image to copy to the texture
    /// \param dest  Coordinates of the destination position
    ///
    ////////////////////////////////////////////////////////////
    void update(const Image& image, Vector2u dest);

    ////////////////////////////////////////////////////////////
    /// \brief Update the texture from the contents of a window
    ///
    /// Although the source window can be smaller than the texture,
    /// this function is usually used for updating the whole texture.
    /// The other overload, which has an additional destination
    /// argument, is more convenient for updating a sub-area of the
    /// texture.
    ///
    /// No additional check is performed on the size of the window.
    /// Passing a window bigger than the texture will lead to an
    /// undefined behavior.
    ///
    /// This function does nothing if either the texture or the window
    /// was not previously created.
    ///
    /// \param window Window to copy to the texture
    ///
    ////////////////////////////////////////////////////////////
    void update(const Window& window);

    ////////////////////////////////////////////////////////////
    /// \brief Update a part of the texture from the contents of a window
    ///
    /// No additional check is performed on the size of the window.
    /// Passing an invalid combination of window size and destination
    /// will lead to an undefined behavior.
    ///
    /// This function does nothing if either the texture or the window
    /// was not previously created.
    ///
    /// \param window Window to copy to the texture
    /// \param dest   Coordinates of the destination position
    ///
    ////////////////////////////////////////////////////////////
    void update(const Window& window, Vector2u dest);

    ////////////////////////////////////////////////////////////
    /// \brief Enable or disable the smooth filter
    ///
    /// When the filter is activated, the texture appears smoother
    /// so that pixels are less noticeable. However if you want
    /// the texture to look exactly the same as its source file,
    /// you should leave it disabled.
    /// The smooth filter is disabled by default.
    ///
    /// \param smooth `true` to enable smoothing, `false` to disable it
    ///
    /// \see `isSmooth`
    ///
    ////////////////////////////////////////////////////////////
    void setSmooth(bool smooth);

    ////////////////////////////////////////////////////////////
    /// \brief Tell whether the smooth filter is enabled or not
    ///
    /// \return `true` if smoothing is enabled, `false` if it is disabled
    ///
    /// \see `setSmooth`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isSmooth() const;

    ////////////////////////////////////////////////////////////
    /// \brief Tell whether the texture source is converted from sRGB or not
    ///
    /// \return `true` if the texture source is converted from sRGB, `false` if not
    ///
    /// \see `setSrgb`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isSrgb() const;

    ////////////////////////////////////////////////////////////
    /// \brief Enable or disable repeating
    ///
    /// Repeating is involved when using texture coordinates
    /// outside the texture rectangle [0, 0, width, height].
    /// In this case, if repeat mode is enabled, the whole texture
    /// will be repeated as many times as needed to reach the
    /// coordinate (for example, if the X texture coordinate is
    /// 3 * width, the texture will be repeated 3 times).
    /// If repeat mode is disabled, the "extra space" will instead
    /// be filled with border pixels.
    /// Warning: on very old graphics cards, white pixels may appear
    /// when the texture is repeated. With such cards, repeat mode
    /// can be used reliably only if the texture has power-of-two
    /// dimensions (such as 256x128).
    /// Repeating is disabled by default.
    ///
    /// \param repeated `true` to repeat the texture, `false` to disable repeating
    ///
    /// \see `isRepeated`
    ///
    ////////////////////////////////////////////////////////////
    void setRepeated(bool repeated);

    ////////////////////////////////////////////////////////////
    /// \brief Tell whether the texture is repeated or not
    ///
    /// \return `true` if repeat mode is enabled, `false` if it is disabled
    ///
    /// \see `setRepeated`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isRepeated() const;

    ////////////////////////////////////////////////////////////
    /// \brief Generate a mipmap using the current texture data
    ///
    /// Mipmaps are pre-computed chains of optimized textures. Each
    /// level of texture in a mipmap is generated by halving each of
    /// the previous level's dimensions. This is done until the final
    /// level has the size of 1x1. The textures generated in this process may
    /// make use of more advanced filters which might improve the visual quality
    /// of textures when they are applied to objects much smaller than they are.
    /// This is known as minification. Because fewer texels (texture elements)
    /// have to be sampled from when heavily minified, usage of mipmaps
    /// can also improve rendering performance in certain scenarios.
    ///
    /// Mipmap generation relies on the necessary OpenGL extension being
    /// available. If it is unavailable or generation fails due to another
    /// reason, this function will return `false`. Mipmap data is only valid from
    /// the time it is generated until the next time the base level image is
    /// modified, at which point this function will have to be called again to
    /// regenerate it.
    ///
    /// \return `true` if mipmap generation was successful, `false` if unsuccessful
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool generateMipmap();

    ////////////////////////////////////////////////////////////
    /// \brief Swap the contents of this texture with those of another
    ///
    /// \param right Instance to swap with
    ///
    ////////////////////////////////////////////////////////////
    void swap(Texture& right) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Get the underlying OpenGL handle of the texture.
    ///
    /// You shouldn't need to use this function, unless you have
    /// very specific stuff to implement that SFML doesn't support,
    /// or implement a temporary workaround until a bug is fixed.
    ///
    /// \return OpenGL handle of the texture or 0 if not yet created
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getNativeHandle() const;

    ////////////////////////////////////////////////////////////
    /// \brief Bind a texture for rendering
    ///
    /// This function is not part of the graphics API, it mustn't be
    /// used when drawing SFML entities. It must be used only if you
    /// mix `sf::Texture` with OpenGL code.
    ///
    /// \code
    /// sf::Texture t1, t2;
    /// ...
    /// sf::Texture::bind(&t1);
    /// // draw OpenGL stuff that use t1...
    /// sf::Texture::bind(&t2);
    /// // draw OpenGL stuff that use t2...
    /// sf::Texture::bind(nullptr);
    /// // draw OpenGL stuff that use no texture...
    /// \endcode
    ///
    /// The `coordinateType` argument controls how texture
    /// coordinates will be interpreted. If Normalized (the default), they
    /// must be in range [0 .. 1], which is the default way of handling
    /// texture coordinates with OpenGL. If Pixels, they must be given
    /// in pixels (range [0 .. size]). This mode is used internally by
    /// the graphics classes of SFML, it makes the definition of texture
    /// coordinates more intuitive for the high-level API, users don't need
    /// to compute normalized values.
    ///
    /// \param texture Pointer to the texture to bind, can be null to use no texture
    /// \param coordinateType Type of texture coordinates to use
    ///
    ////////////////////////////////////////////////////////////
    static void bind(const Texture* texture, CoordinateType coordinateType = CoordinateType::Normalized);

    ////////////////////////////////////////////////////////////
    /// \brief Get the maximum texture size allowed
    ///
    /// This maximum size is defined by the graphics driver.
    /// You can expect a value of 512 pixels for low-end graphics
    /// card, and up to 8192 pixels or more for newer hardware.
    ///
    /// \return Maximum size allowed for textures, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static unsigned int getMaximumSize();

    ////////////////////////////////////////////////////////////
    /// \brief Get the state of whether the Y orientatino of pixels are flipped
    ///
    /// \return `true` if pixels are flipped, otherwise `false`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool getPixelsFlipped() const;

private:
    friend class Text;
    friend class RenderTexture;
    friend class RenderTarget;

    ////////////////////////////////////////////////////////////
    /// \brief Get a valid image size according to hardware support
    ///
    /// This function checks whether the graphics driver supports
    /// non power of two sizes or not, and adjusts the size
    /// accordingly.
    /// The returned size is greater than or equal to the original size.
    ///
    /// \param size size to convert
    ///
    /// \return Valid nearest size (greater than or equal to specified size)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static unsigned int getValidSize(unsigned int size);

    ////////////////////////////////////////////////////////////
    /// \brief Invalidate the mipmap if one exists
    ///
    /// This also resets the texture's minifying function.
    /// This function is mainly for internal use by RenderTexture.
    ///
    ////////////////////////////////////////////////////////////
    void invalidateMipmap();

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    Vector2u      m_size;            //!< Public texture size
    Vector2u      m_actualSize;      //!< Actual texture size (can be greater than public size because of padding)
    unsigned int  m_texture{};       //!< Internal texture identifier
    bool          m_isSmooth{};      //!< Status of the smooth filter
    bool          m_sRgb{};          //!< Should the texture source be converted from sRGB?
    bool          m_isRepeated{};    //!< Is the texture in repeat mode?
    mutable bool  m_pixelsFlipped{}; //!< To work around the inconsistency in Y orientation
    bool          m_fboAttachment{}; //!< Is this texture owned by a framebuffer object?
    bool          m_hasMipmap{};     //!< Has the mipmap been generated?
    std::uint64_t m_cacheId;         //!< Unique number that identifies the texture to the render target's cache
};

////////////////////////////////////////////////////////////
/// \brief Swap the contents of one texture with those of another
///
/// \param left First instance to swap
/// \param right Second instance to swap
///
////////////////////////////////////////////////////////////
SFML_GRAPHICS_API void swap(Texture& left, Texture& right) noexcept;

} // namespace sf


////////////////////////////////////////////////////////////
/// \class sf::Texture
/// \ingroup graphics
///
/// `sf::Texture` stores pixels that can be drawn, with a sprite
/// for example. A texture lives in the graphics card memory,
/// therefore it is very fast to draw a texture to a render target,
/// or copy a render target to a texture (the graphics card can
/// access both directly).
///
/// Being stored in the graphics card memory has some drawbacks.
/// A texture cannot be manipulated as freely as a `sf::Image`,
/// you need to prepare the pixels first and then upload them
/// to the texture in a single operation (see `Texture::update`).
///
/// `sf::Texture` makes it easy to convert from/to `sf::Image`, but
/// keep in mind that these calls require transfers between
/// the graphics card and the central memory, therefore they are
/// slow operations.
///
/// A texture can be loaded from an image, but also directly
/// from a file/memory/stream. The necessary shortcuts are defined
/// so that you don't need an image first for the most common cases.
/// However, if you want to perform some modifications on the pixels
/// before creating the final texture, you can load your file to a
/// `sf::Image`, do whatever you need with the pixels, and then call
/// `Texture(const Image&)`.
///
/// Since they live in the graphics card memory, the pixels of a texture
/// cannot be accessed without a slow copy first. And they cannot be
/// accessed individually. Therefore, if you need to read the texture's
/// pixels (like for pixel-perfect collisions), it is recommended to
/// store the collision information separately, for example in an array
/// of booleans.
///
/// Like `sf::Image`, `sf::Texture` can handle a unique internal
/// representation of pixels, which is RGBA 32 bits. This means
/// that a pixel must be composed of 8 bit red, green, blue and
/// alpha channels -- just like a `sf::Color`.
///
/// When providing texture data from an image file or memory, it can
/// either be stored in a linear color space or an sRGB color space.
/// Most digital images account for gamma correction already, so they
/// would need to be "uncorrected" back to linear color space before
/// being processed by the hardware. The hardware can automatically
/// convert it from the sRGB color space to a linear color space when
/// it gets sampled. When the rendered image gets output to the final
/// framebuffer, it gets converted back to sRGB.
///
/// This option is only useful in conjunction with an sRGB capable
/// framebuffer. This can be requested during window creation.
///
/// Usage example:
/// \code
/// // This example shows the most common use of sf::Texture:
/// // drawing a sprite
///
/// // Load a texture from a file
/// const sf::Texture texture("texture.png");
///
/// // Assign it to a sprite
/// sf::Sprite sprite(texture);
///
/// // Draw the textured sprite
/// window.draw(sprite);
/// \endcode
///
/// \code
/// // This example shows another common use of sf::Texture:
/// // streaming real-time data, like video frames
///
/// // Create an empty texture
/// sf::Texture texture({640, 480});
///
/// // Create a sprite that will display the texture
/// sf::Sprite sprite(texture);
///
/// while (...) // the main loop
/// {
///     ...
///
///     // update the texture
///     std::uint8_t* pixels = ...; // get a fresh chunk of pixels (the next frame of a movie, for example)
///     texture.update(pixels);
///
///     // draw it
///     window.draw(sprite);
///
///     ...
/// }
///
/// \endcode
///
/// Like `sf::Shader` that can be used as a raw OpenGL shader,
/// `sf::Texture` can also be used directly as a raw texture for
/// custom OpenGL geometry.
/// \code
/// sf::Texture::bind(&texture);
/// ... render OpenGL geometry ...
/// sf::Texture::bind(nullptr);
/// \endcode
///
/// \see `sf::Sprite`, `sf::Image`, `sf::RenderTexture`
///
////////////////////////////////////////////////////////////
