#include <SFML/Graphics/VertexArray.hpp>

#include <catch2/catch_test_macros.hpp>

#include <GraphicsUtil.hpp>
#include <type_traits>
#include <utility>

TEST_CASE("[Graphics] sf::VertexArray")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(std::is_copy_constructible_v<sf::VertexArray>);
        STATIC_CHECK(std::is_copy_assignable_v<sf::VertexArray>);
        STATIC_CHECK(std::is_nothrow_move_constructible_v<sf::VertexArray>);
        STATIC_CHECK(std::is_nothrow_move_assignable_v<sf::VertexArray>);
    }

    SECTION("Construction")
    {
        SECTION("Default constructor")
        {
            const sf::VertexArray vertexArray;
            CHECK(vertexArray.getVertexCount() == 0);
            CHECK(vertexArray.getPrimitiveType() == sf::PrimitiveType::Points);
            CHECK(vertexArray.getBounds() == sf::FloatRect({0, 0}, {0, 0}));
        }

        SECTION("Explicit constructor with default argument")
        {
            const sf::VertexArray vertexArray(sf::PrimitiveType::Lines);
            CHECK(vertexArray.getVertexCount() == 0);
            CHECK(vertexArray.getPrimitiveType() == sf::PrimitiveType::Lines);
            CHECK(vertexArray.getBounds() == sf::FloatRect({0, 0}, {0, 0}));
        }

        SECTION("Explicit constructor")
        {
            const sf::VertexArray vertexArray(sf::PrimitiveType::Lines, 10);
            CHECK(vertexArray.getVertexCount() == 10);
            CHECK(vertexArray.getPrimitiveType() == sf::PrimitiveType::Lines);
            CHECK(vertexArray.getBounds() == sf::FloatRect({0, 0}, {0, 0}));
            for (const auto& vertex : vertexArray)
            {
                CHECK(vertex.position == sf::Vertex{}.position);
                CHECK(vertex.color == sf::Vertex{}.color);
                CHECK(vertex.texCoords == sf::Vertex{}.texCoords);
            }
        }
    }

    SECTION("Resize array")
    {
        sf::VertexArray vertexArray;
        vertexArray.resize(42);
        CHECK(vertexArray.getVertexCount() == 42);
        for (const auto& vertex : vertexArray)
        {
            CHECK(vertex.position == sf::Vertex{}.position);
            CHECK(vertex.color == sf::Vertex{}.color);
            CHECK(vertex.texCoords == sf::Vertex{}.texCoords);
        }
    }

    SECTION("Clear array")
    {
        sf::VertexArray vertexArray;
        vertexArray.append({});
        vertexArray.clear();
        CHECK(vertexArray.getVertexCount() == 0);
    }

    SECTION("Append to array")
    {
        sf::VertexArray  vertexArray;
        const sf::Vertex vertex{{1.0f, 2.0f}, {3, 4, 5, 6}, {7.0f, 8.0f}};
        vertexArray.append(vertex);
        CHECK(vertexArray.getVertexCount() == 1);
        CHECK(vertexArray[0].position == vertex.position);
        CHECK(vertexArray[0].color == vertex.color);
        CHECK(vertexArray[0].texCoords == vertex.texCoords);
    }

    SECTION("Write to array")
    {
        sf::VertexArray vertexArray;
        vertexArray.resize(10);
        const sf::Vertex otherVertex{{2.0f, 3.0f}, {4, 5, 6, 7}, {8.0f, 9.0f}};
        vertexArray[9] = otherVertex;
        CHECK(vertexArray[9].position == otherVertex.position);
        CHECK(vertexArray[9].color == otherVertex.color);
        CHECK(vertexArray[9].texCoords == otherVertex.texCoords);
    }

    SECTION("Read from array")
    {
        sf::VertexArray vertexArray;
        vertexArray.append({{1.0f, 2.0f}});
        vertexArray.append({{10.0f, 20.0f}});

        SECTION("Const")
        {
            const auto& constVertexArray = vertexArray;
            CHECK(constVertexArray[0].position == sf::Vector2f(1, 2));
            CHECK(constVertexArray[1].position == sf::Vector2f(10, 20));
        }

        SECTION("Non const")
        {
            auto& nonConstVertexArray = vertexArray;
            CHECK(nonConstVertexArray[0].position == sf::Vector2f(1, 2));
            CHECK(nonConstVertexArray[1].position == sf::Vector2f(10, 20));
        }
    }

    SECTION("Set primitive type")
    {
        sf::VertexArray vertexArray;
        vertexArray.setPrimitiveType(sf::PrimitiveType::TriangleStrip);
        CHECK(vertexArray.getPrimitiveType() == sf::PrimitiveType::TriangleStrip);
    }

    SECTION("Get bounds")
    {
        sf::VertexArray vertexArray;
        vertexArray.append({{1, 1}});
        vertexArray.append({{2, 2}});
        CHECK(vertexArray.getBounds() == sf::FloatRect({1, 1}, {1, 1}));
        vertexArray[0] = {{0, 0}};
        CHECK(vertexArray.getBounds() == sf::FloatRect({0, 0}, {2, 2}));
        vertexArray[0] = {{5, 5}};
        CHECK(vertexArray.getBounds() == sf::FloatRect({2, 2}, {3, 3}));
        vertexArray.append({{10, 10}});
        CHECK(vertexArray.getBounds() == sf::FloatRect({2, 2}, {8, 8}));
    }

    SECTION("Ranged loop")
    {
        sf::VertexArray vertexArray;
        vertexArray.append({{1, 1}});
        vertexArray.append({{1, 1}});
        vertexArray.append({{1, 1}});

        std::size_t count = 0;

        SECTION("Const")
        {
            for (const auto& vertex : std::as_const(vertexArray))
            {
                CHECK(vertex.position == sf::Vector2f(1, 1));
                ++count;
            }
        }

        SECTION("Non const")
        {
            for (const auto& vertex : vertexArray)
            {
                CHECK(vertex.position == sf::Vector2f(1, 1));
                ++count;
            }
        }

        CHECK(count == vertexArray.getVertexCount());
    }
}
