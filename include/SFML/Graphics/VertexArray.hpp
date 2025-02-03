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

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Vertex.hpp>

#include <vector>

#include <cstddef>


namespace sf
{
class RenderTarget;

////////////////////////////////////////////////////////////
/// \brief Set of one or more 2D primitives
///
////////////////////////////////////////////////////////////
class SFML_GRAPHICS_API VertexArray : public Drawable
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor
    ///
    /// Creates an empty vertex array.
    ///
    ////////////////////////////////////////////////////////////
    VertexArray() = default;

    ////////////////////////////////////////////////////////////
    /// \brief Construct the vertex array with a type and an initial number of vertices
    ///
    /// \param type        Type of primitives
    /// \param vertexCount Initial number of vertices in the array
    ///
    ////////////////////////////////////////////////////////////
    explicit VertexArray(PrimitiveType type, std::size_t vertexCount = 0);

    ////////////////////////////////////////////////////////////
    /// \brief Return the vertex count
    ///
    /// \return Number of vertices in the array
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] std::size_t getVertexCount() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get a read-write access to a vertex by its index
    ///
    /// This function doesn't check `index`, it must be in range
    /// [0, `getVertexCount()` - 1]. The behavior is undefined
    /// otherwise.
    ///
    /// \param index Index of the vertex to get
    ///
    /// \return Reference to the `index`-th vertex
    ///
    /// \see `getVertexCount`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vertex& operator[](std::size_t index);

    ////////////////////////////////////////////////////////////
    /// \brief Get a read-only access to a vertex by its index
    ///
    /// This function doesn't check `index`, it must be in range
    /// [0, `getVertexCount()` - 1]. The behavior is undefined
    /// otherwise.
    ///
    /// \param index Index of the vertex to get
    ///
    /// \return Const reference to the `index`-th vertex
    ///
    /// \see `getVertexCount`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const Vertex& operator[](std::size_t index) const;

    ////////////////////////////////////////////////////////////
    /// \brief Clear the vertex array
    ///
    /// This function removes all the vertices from the array.
    /// It doesn't deallocate the corresponding memory, so that
    /// adding new vertices after clearing doesn't involve
    /// reallocating all the memory.
    ///
    ////////////////////////////////////////////////////////////
    void clear();

    ////////////////////////////////////////////////////////////
    /// \brief Resize the vertex array
    ///
    /// If `vertexCount` is greater than the current size, the previous
    /// vertices are kept and new (default-constructed) vertices are
    /// added.
    /// If `vertexCount` is less than the current size, existing vertices
    /// are removed from the array.
    ///
    /// \param vertexCount New size of the array (number of vertices)
    ///
    ////////////////////////////////////////////////////////////
    void resize(std::size_t vertexCount);

    ////////////////////////////////////////////////////////////
    /// \brief Add a vertex to the array
    ///
    /// \param vertex Vertex to add
    ///
    ////////////////////////////////////////////////////////////
    void append(const Vertex& vertex);

    ////////////////////////////////////////////////////////////
    /// \brief Set the type of primitives to draw
    ///
    /// This function defines how the vertices must be interpreted
    /// when it's time to draw them:
    /// \li As points
    /// \li As lines
    /// \li As triangles
    /// The default primitive type is `sf::PrimitiveType::Points`.
    ///
    /// \param type Type of primitive
    ///
    ////////////////////////////////////////////////////////////
    void setPrimitiveType(PrimitiveType type);

    ////////////////////////////////////////////////////////////
    /// \brief Get the type of primitives drawn by the vertex array
    ///
    /// \return Primitive type
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] PrimitiveType getPrimitiveType() const;

    ////////////////////////////////////////////////////////////
    /// \brief Compute the bounding rectangle of the vertex array
    ///
    /// This function returns the minimal axis-aligned rectangle
    /// that contains all the vertices of the array.
    ///
    /// \return Bounding rectangle of the vertex array
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] FloatRect getBounds() const;

    ////////////////////////////////////////////////////////////
    /// \brief Return an iterator to the beginning of the array
    ///
    /// \return Read-write iterator to the beginning of the vertices
    ///
    /// \see `end`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] std::vector<Vertex>::iterator begin();

    ////////////////////////////////////////////////////////////
    /// \brief Return an iterator to the beginning of the array
    ///
    /// \return Read-only iterator to the beginning of the vertices
    ///
    /// \see `end`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] std::vector<Vertex>::const_iterator begin() const;

    ////////////////////////////////////////////////////////////
    /// \brief Return an iterator to the end of the array
    ///
    /// The end iterator refers to 1 position past the last vertex;
    /// thus it represents an invalid vertex and should never be
    /// accessed.
    ///
    /// \return Read-write iterator to the end of the vertices
    ///
    /// \see `begin`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] std::vector<Vertex>::iterator end();

    ////////////////////////////////////////////////////////////
    /// \brief Return an iterator to the end of the array
    ///
    /// The end iterator refers to 1 position past the last vertex;
    /// thus it represents an invalid vertex and should never be
    /// accessed.
    ///
    /// \return Read-only iterator to the end of the vertices
    ///
    /// \see `begin`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] std::vector<Vertex>::const_iterator end() const;

private:
    ////////////////////////////////////////////////////////////
    /// \brief Draw the vertex array to a render target
    ///
    /// \param target Render target to draw to
    /// \param states Current render states
    ///
    ////////////////////////////////////////////////////////////
    void draw(RenderTarget& target, RenderStates states) const override;

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    std::vector<Vertex> m_vertices;                             //!< Vertices contained in the array
    PrimitiveType       m_primitiveType{PrimitiveType::Points}; //!< Type of primitives to draw
};

} // namespace sf


////////////////////////////////////////////////////////////
/// \class sf::VertexArray
/// \ingroup graphics
///
/// `sf::VertexArray` is a very simple wrapper around a dynamic
/// array of vertices and a primitives type.
///
/// It inherits `sf::Drawable`, but unlike other drawables it
/// is not transformable.
///
/// Example:
/// \code
/// sf::VertexArray lines(sf::PrimitiveType::LineStrip, 4);
/// lines[0].position = sf::Vector2f(10, 0);
/// lines[1].position = sf::Vector2f(20, 0);
/// lines[2].position = sf::Vector2f(30, 5);
/// lines[3].position = sf::Vector2f(40, 2);
///
/// window.draw(lines);
/// \endcode
///
/// \see `sf::Vertex`
///
////////////////////////////////////////////////////////////
