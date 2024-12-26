#include <SFML/Graphics/Shape.hpp>

// Other 1st party headers
#include <SFML/Graphics/Texture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <GraphicsUtil.hpp>
#include <WindowUtil.hpp>
#include <type_traits>

class TriangleShape : public sf::Shape
{
public:
    explicit TriangleShape(sf::Vector2f size) : m_size(size)
    {
        update();
    }

    std::size_t getPointCount() const override
    {
        return 3;
    }

    sf::Vector2f getPoint(std::size_t index) const override
    {
        switch (index)
        {
            default:
            case 0:
                return {m_size.x / 2, 0};
            case 1:
                return {0, m_size.y};
            case 2:
                return {m_size.x, m_size.y};
        }
    }

private:
    sf::Vector2f m_size;
};

TEST_CASE("[Graphics] sf::Shape", runDisplayTests())
{
    SECTION("Type traits")
    {
        STATIC_CHECK(!std::is_constructible_v<sf::Shape>);
        STATIC_CHECK(!std::is_copy_constructible_v<sf::Shape>);
        STATIC_CHECK(std::is_copy_assignable_v<sf::Shape>);
        STATIC_CHECK(!std::is_move_constructible_v<sf::Shape>);
        STATIC_CHECK(std::is_nothrow_move_assignable_v<sf::Shape>);
        STATIC_CHECK(std::has_virtual_destructor_v<sf::Shape>);
    }

    SECTION("Default constructor")
    {
        const TriangleShape triangleShape({0, 0});
        CHECK(triangleShape.getTexture() == nullptr);
        CHECK(triangleShape.getTextureRect() == sf::IntRect());
        CHECK(triangleShape.getFillColor() == sf::Color::White);
        CHECK(triangleShape.getOutlineColor() == sf::Color::White);
        CHECK(triangleShape.getOutlineThickness() == 0.0f);
        CHECK(triangleShape.getMiterLimit() == 10.0f);
        CHECK(triangleShape.getLocalBounds() == sf::FloatRect());
        CHECK(triangleShape.getGlobalBounds() == sf::FloatRect());
    }

    SECTION("Set/get texture")
    {
        const sf::Texture texture(sf::Vector2u(64, 64));
        TriangleShape     triangleShape({});
        triangleShape.setTexture(&texture, true);
        CHECK(triangleShape.getTexture() == &texture);
    }

    SECTION("Set/get texture rect")
    {
        TriangleShape triangleShape({});
        triangleShape.setTextureRect({{4, 5}, {6, 7}});
        CHECK(triangleShape.getTextureRect() == sf::IntRect({4, 5}, {6, 7}));
    }

    SECTION("Set/get fill color")
    {
        TriangleShape triangleShape({});
        triangleShape.setFillColor(sf::Color::Cyan);
        CHECK(triangleShape.getFillColor() == sf::Color::Cyan);
    }

    SECTION("Set/get outline color")
    {
        TriangleShape triangleShape({});
        triangleShape.setOutlineColor(sf::Color::Magenta);
        CHECK(triangleShape.getOutlineColor() == sf::Color::Magenta);
    }

    SECTION("Set/get outline thickness")
    {
        TriangleShape triangleShape({});
        triangleShape.setOutlineThickness(3.14f);
        CHECK(triangleShape.getOutlineThickness() == 3.14f);
    }

    SECTION("Set/get miter limit")
    {
        TriangleShape triangleShape({});
        triangleShape.setMiterLimit(6.28f);
        CHECK(triangleShape.getMiterLimit() == 6.28f);
    }

    SECTION("Virtual functions: getPoint, getPointCount, getGeometricCenter")
    {
        const TriangleShape triangleShape({2, 2});
        CHECK(triangleShape.getPointCount() == 3);
        CHECK(triangleShape.getPoint(0) == sf::Vector2f(1, 0));
        CHECK(triangleShape.getPoint(1) == sf::Vector2f(0, 2));
        CHECK(triangleShape.getPoint(2) == sf::Vector2f(2, 2));
        CHECK(triangleShape.getGeometricCenter() == sf::Vector2f(1.f, 4.f / 3.f));
    }

    SECTION("Get bounds")
    {
        TriangleShape triangleShape({30, 40});
        CHECK(triangleShape.getLocalBounds() == sf::FloatRect({0, 0}, {30, 40}));
        CHECK(triangleShape.getGlobalBounds() == sf::FloatRect({0, 0}, {30, 40}));

        SECTION("Move and rotate")
        {
            triangleShape.move({1, 1});
            triangleShape.rotate(sf::degrees(90));
            CHECK(triangleShape.getLocalBounds() == sf::FloatRect({0, 0}, {30, 40}));
            CHECK(triangleShape.getGlobalBounds() == Approx(sf::FloatRect({-39, 1}, {40, 30})));
        }

        SECTION("Add outline")
        {
            triangleShape.setOutlineThickness(5);
            CHECK(triangleShape.getLocalBounds() == Approx(sf::FloatRect({-7.2150f, -14.2400f}, {44.4300f, 59.2400f})));
            CHECK(triangleShape.getGlobalBounds() == Approx(sf::FloatRect({-7.2150f, -14.2400f}, {44.4300f, 59.2400f})));
        }

        SECTION("Add beveled outline")
        {
            triangleShape.setMiterLimit(2);
            triangleShape.setOutlineThickness(5);
            CHECK(triangleShape.getLocalBounds() == Approx(sf::FloatRect({-7.2150f, -10.f}, {44.4300f, 55.f})));
            CHECK(triangleShape.getGlobalBounds() == Approx(sf::FloatRect({-7.2150f, -10.f}, {44.4300f, 55.f})));
        }
    }
}
