#include <SFML/Window/Event.hpp>

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string_view>
#include <type_traits>

namespace
{
struct
{
    std::string_view operator()(sf::Event::Closed&) const
    {
        return "Closed (non-const)";
    }

    std::string_view operator()(const sf::Event::Closed&) const
    {
        return "Closed (const)";
    }

    std::string_view operator()(sf::Event::Resized&) const
    {
        return "Resized";
    }

    std::string_view operator()(sf::Event::KeyPressed) const
    {
        return "KeyPressed";
    }

    template <typename T>
    std::string_view operator()(const T&) const
    {
        return "Other";
    }
} visitor;
} // namespace

TEST_CASE("[Window] sf::Event")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(!std::is_default_constructible_v<sf::Event>);
        STATIC_CHECK(std::is_trivially_copy_constructible_v<sf::Event>);
        STATIC_CHECK(std::is_trivially_copy_assignable_v<sf::Event>);
        STATIC_CHECK(std::is_trivially_move_constructible_v<sf::Event>);
        STATIC_CHECK(std::is_trivially_move_assignable_v<sf::Event>);
    }

    SECTION("Construction")
    {
        SECTION("Template constructor")
        {
            const sf::Event event = sf::Event::Resized{{1, 2}};
            CHECK(event.is<sf::Event::Resized>());
            CHECK(event.getIf<sf::Event::Resized>());
            const auto& resized = *event.getIf<sf::Event::Resized>();
            CHECK(resized.size == sf::Vector2u(1, 2));
        }
    }

    SECTION("Assign all possible values")
    {
        sf::Event event = sf::Event::Closed{};
        CHECK(event.is<sf::Event::Closed>());
        CHECK(event.getIf<sf::Event::Closed>());

        event = sf::Event::Resized{{1, 2}};
        CHECK(event.is<sf::Event::Resized>());
        CHECK(event.getIf<sf::Event::Resized>());
        const auto& resized = *event.getIf<sf::Event::Resized>();
        CHECK(resized.size == sf::Vector2u(1, 2));

        event = sf::Event::WindowMoved{{3, 4}};
        CHECK(event.is<sf::Event::WindowMoved>());
        CHECK(event.getIf<sf::Event::WindowMoved>());
        const auto& windowMoved = *event.getIf<sf::Event::WindowMoved>();
        CHECK(windowMoved.position == sf::Vector2i(3, 4));

        event = sf::Event::FocusLost{};
        CHECK(event.is<sf::Event::FocusLost>());
        CHECK(event.getIf<sf::Event::FocusLost>());

        event = sf::Event::FocusGained{};
        CHECK(event.is<sf::Event::FocusGained>());
        CHECK(event.getIf<sf::Event::FocusGained>());

        event = sf::Event::TextEntered{123456};
        CHECK(event.is<sf::Event::TextEntered>());
        CHECK(event.getIf<sf::Event::TextEntered>());
        const auto& textEntered = *event.getIf<sf::Event::TextEntered>();
        CHECK(textEntered.unicode == 123456);

        event = sf::Event::KeyPressed{sf::Keyboard::Key::C, sf::Keyboard::Scan::C, true, true, true, true};
        CHECK(event.is<sf::Event::KeyPressed>());
        CHECK(event.getIf<sf::Event::KeyPressed>());
        const auto& keyPressed = *event.getIf<sf::Event::KeyPressed>();
        CHECK(keyPressed.code == sf::Keyboard::Key::C);
        CHECK(keyPressed.scancode == sf::Keyboard::Scan::C);
        CHECK(keyPressed.alt);
        CHECK(keyPressed.control);
        CHECK(keyPressed.shift);
        CHECK(keyPressed.system);

        event = sf::Event::KeyReleased{sf::Keyboard::Key::D, sf::Keyboard::Scan::D, true, true, true, true};
        CHECK(event.is<sf::Event::KeyReleased>());
        CHECK(event.getIf<sf::Event::KeyReleased>());
        const auto& keyReleased = *event.getIf<sf::Event::KeyReleased>();
        CHECK(keyReleased.code == sf::Keyboard::Key::D);
        CHECK(keyReleased.scancode == sf::Keyboard::Scan::D);
        CHECK(keyReleased.alt);
        CHECK(keyReleased.control);
        CHECK(keyReleased.shift);
        CHECK(keyReleased.system);

        event = sf::Event::MouseWheelScrolled{sf::Mouse::Wheel::Horizontal, 3.14f, {4, 5}};
        CHECK(event.is<sf::Event::MouseWheelScrolled>());
        CHECK(event.getIf<sf::Event::MouseWheelScrolled>());
        const auto& mouseWheelScrolled = *event.getIf<sf::Event::MouseWheelScrolled>();
        CHECK(mouseWheelScrolled.wheel == sf::Mouse::Wheel::Horizontal);
        CHECK(mouseWheelScrolled.delta == 3.14f);
        CHECK(mouseWheelScrolled.position == sf::Vector2i(4, 5));

        event = sf::Event::MouseButtonPressed{sf::Mouse::Button::Middle, {6, 7}};
        CHECK(event.is<sf::Event::MouseButtonPressed>());
        CHECK(event.getIf<sf::Event::MouseButtonPressed>());
        const auto& mouseButtonPressed = *event.getIf<sf::Event::MouseButtonPressed>();
        CHECK(mouseButtonPressed.button == sf::Mouse::Button::Middle);
        CHECK(mouseButtonPressed.position == sf::Vector2i(6, 7));

        event = sf::Event::MouseButtonReleased{sf::Mouse::Button::Extra1, {8, 9}};
        CHECK(event.is<sf::Event::MouseButtonReleased>());
        CHECK(event.getIf<sf::Event::MouseButtonReleased>());
        const auto& mouseButtonReleased = *event.getIf<sf::Event::MouseButtonReleased>();
        CHECK(mouseButtonReleased.button == sf::Mouse::Button::Extra1);
        CHECK(mouseButtonReleased.position == sf::Vector2i(8, 9));

        event = sf::Event::MouseMoved{{4, 2}};
        CHECK(event.is<sf::Event::MouseMoved>());
        CHECK(event.getIf<sf::Event::MouseMoved>());
        const auto& mouseMoved = *event.getIf<sf::Event::MouseMoved>();
        CHECK(mouseMoved.position == sf::Vector2i(4, 2));

        event = sf::Event::MouseMovedRaw{{3, 7}};
        CHECK(event.is<sf::Event::MouseMovedRaw>());
        CHECK(event.getIf<sf::Event::MouseMovedRaw>());
        const auto& mouseMovedRaw = *event.getIf<sf::Event::MouseMovedRaw>();
        CHECK(mouseMovedRaw.delta == sf::Vector2i(3, 7));

        event = sf::Event::MouseEntered{};
        CHECK(event.is<sf::Event::MouseEntered>());
        CHECK(event.getIf<sf::Event::MouseEntered>());

        event = sf::Event::MouseLeft{};
        CHECK(event.is<sf::Event::MouseLeft>());
        CHECK(event.getIf<sf::Event::MouseLeft>());

        event = sf::Event::JoystickButtonPressed{100, 200};
        CHECK(event.is<sf::Event::JoystickButtonPressed>());
        CHECK(event.getIf<sf::Event::JoystickButtonPressed>());
        const auto& joystickButtonPressed = *event.getIf<sf::Event::JoystickButtonPressed>();
        CHECK(joystickButtonPressed.joystickId == 100);
        CHECK(joystickButtonPressed.button == 200);

        event = sf::Event::JoystickButtonReleased{300, 400};
        CHECK(event.is<sf::Event::JoystickButtonReleased>());
        CHECK(event.getIf<sf::Event::JoystickButtonReleased>());
        const auto& joystickButtonReleased = *event.getIf<sf::Event::JoystickButtonReleased>();
        CHECK(joystickButtonReleased.joystickId == 300);
        CHECK(joystickButtonReleased.button == 400);

        event = sf::Event::JoystickMoved{300, sf::Joystick::Axis::Z, 1.23f};
        CHECK(event.is<sf::Event::JoystickMoved>());
        CHECK(event.getIf<sf::Event::JoystickMoved>());
        const auto& joystickMoved = *event.getIf<sf::Event::JoystickMoved>();
        CHECK(joystickMoved.joystickId == 300);
        CHECK(joystickMoved.axis == sf::Joystick::Axis::Z);
        CHECK(joystickMoved.position == 1.23f);

        event = sf::Event::JoystickConnected{42};
        CHECK(event.is<sf::Event::JoystickConnected>());
        CHECK(event.getIf<sf::Event::JoystickConnected>());
        const auto& joystickConnected = *event.getIf<sf::Event::JoystickConnected>();
        CHECK(joystickConnected.joystickId == 42);

        event = sf::Event::JoystickDisconnected{43};
        CHECK(event.is<sf::Event::JoystickDisconnected>());
        CHECK(event.getIf<sf::Event::JoystickDisconnected>());
        const auto& joystickDisconnected = *event.getIf<sf::Event::JoystickDisconnected>();
        CHECK(joystickDisconnected.joystickId == 43);

        event = sf::Event::TouchBegan{99, {98, 97}};
        CHECK(event.is<sf::Event::TouchBegan>());
        CHECK(event.getIf<sf::Event::TouchBegan>());
        const auto& touchBegan = *event.getIf<sf::Event::TouchBegan>();
        CHECK(touchBegan.finger == 99);
        CHECK(touchBegan.position == sf::Vector2i(98, 97));

        event = sf::Event::TouchMoved{96, {95, 94}};
        CHECK(event.is<sf::Event::TouchMoved>());
        CHECK(event.getIf<sf::Event::TouchMoved>());
        const auto& touchMoved = *event.getIf<sf::Event::TouchMoved>();
        CHECK(touchMoved.finger == 96);
        CHECK(touchMoved.position == sf::Vector2i(95, 94));

        event = sf::Event::TouchEnded{93, {92, 91}};
        CHECK(event.is<sf::Event::TouchEnded>());
        CHECK(event.getIf<sf::Event::TouchEnded>());
        const auto& touchEnded = *event.getIf<sf::Event::TouchEnded>();
        CHECK(touchEnded.finger == 93);
        CHECK(touchEnded.position == sf::Vector2i(92, 91));

        event = sf::Event::SensorChanged{sf::Sensor::Type::Gravity, {1.2f, 3.4f, 5.6f}};
        CHECK(event.is<sf::Event::SensorChanged>());
        CHECK(event.getIf<sf::Event::SensorChanged>());
        const auto& sensorChanged = *event.getIf<sf::Event::SensorChanged>();
        CHECK(sensorChanged.type == sf::Sensor::Type::Gravity);
        CHECK(sensorChanged.value == sf::Vector3f(1.2f, 3.4f, 5.6f));
    }

    SECTION("Subtypes")
    {
        // Empty structs
        STATIC_CHECK(std::is_empty_v<sf::Event::Closed>);
        STATIC_CHECK(std::is_empty_v<sf::Event::FocusLost>);
        STATIC_CHECK(std::is_empty_v<sf::Event::FocusGained>);
        STATIC_CHECK(std::is_empty_v<sf::Event::MouseEntered>);
        STATIC_CHECK(std::is_empty_v<sf::Event::MouseLeft>);

        // Non-empty structs
        const sf::Event::Resized resized;
        CHECK(resized.size == sf::Vector2u());

        const sf::Event::WindowMoved windowMoved;
        CHECK(windowMoved.position == sf::Vector2i());

        const sf::Event::TextEntered textEntered;
        CHECK(textEntered.unicode == 0);

        const sf::Event::KeyPressed keyPressed;
        CHECK(keyPressed.code == sf::Keyboard::Key{});
        CHECK(keyPressed.scancode == sf::Keyboard::Scancode{});
        CHECK(!keyPressed.alt);
        CHECK(!keyPressed.control);
        CHECK(!keyPressed.shift);
        CHECK(!keyPressed.system);

        const sf::Event::KeyReleased keyReleased;
        CHECK(keyReleased.code == sf::Keyboard::Key{});
        CHECK(keyReleased.scancode == sf::Keyboard::Scancode{});
        CHECK(!keyReleased.alt);
        CHECK(!keyReleased.control);
        CHECK(!keyReleased.shift);
        CHECK(!keyReleased.system);

        const sf::Event::MouseWheelScrolled mouseWheelScrolled;
        CHECK(mouseWheelScrolled.wheel == sf::Mouse::Wheel{});
        CHECK(mouseWheelScrolled.delta == 0);
        CHECK(mouseWheelScrolled.position == sf::Vector2i());

        const sf::Event::MouseButtonPressed mouseButtonPressed;
        CHECK(mouseButtonPressed.button == sf::Mouse::Button{});
        CHECK(mouseButtonPressed.position == sf::Vector2i());

        const sf::Event::MouseButtonReleased mouseButtonReleased;
        CHECK(mouseButtonReleased.button == sf::Mouse::Button{});
        CHECK(mouseButtonReleased.position == sf::Vector2i());

        const sf::Event::MouseMoved mouseMoved;
        CHECK(mouseMoved.position == sf::Vector2i());

        const sf::Event::MouseMovedRaw mouseMovedRaw;
        CHECK(mouseMovedRaw.delta == sf::Vector2i());

        const sf::Event::JoystickButtonPressed joystickButtonPressed;
        CHECK(joystickButtonPressed.joystickId == 0);
        CHECK(joystickButtonPressed.button == 0);

        const sf::Event::JoystickButtonReleased joystickButtonReleased;
        CHECK(joystickButtonReleased.joystickId == 0);
        CHECK(joystickButtonReleased.button == 0);

        const sf::Event::JoystickMoved joystickMoved;
        CHECK(joystickMoved.joystickId == 0);
        CHECK(joystickMoved.axis == sf::Joystick::Axis{});
        CHECK(joystickMoved.position == 0);

        const sf::Event::JoystickConnected joystickConnected;
        CHECK(joystickConnected.joystickId == 0);

        const sf::Event::JoystickDisconnected joystickDisconnected;
        CHECK(joystickDisconnected.joystickId == 0);

        const sf::Event::TouchBegan touchBegan;
        CHECK(touchBegan.finger == 0);
        CHECK(touchBegan.position == sf::Vector2i());

        const sf::Event::TouchMoved touchMoved;
        CHECK(touchMoved.finger == 0);
        CHECK(touchMoved.position == sf::Vector2i());

        const sf::Event::TouchEnded touchEnded;
        CHECK(touchEnded.finger == 0);
        CHECK(touchEnded.position == sf::Vector2i());

        const sf::Event::SensorChanged sensorChanged;
        CHECK(sensorChanged.type == sf::Sensor::Type{});
        CHECK(sensorChanged.value == sf::Vector3f());
    }

    SECTION("getIf()")
    {
        sf::Event event      = sf::Event::MouseMoved{{4, 2}};
        auto*     mouseMoved = event.getIf<sf::Event::MouseMoved>();
        REQUIRE(mouseMoved);
        mouseMoved->position = sf::Vector2i(6, 9);
        CHECK(mouseMoved->position == sf::Vector2i(6, 9));
    }

    SECTION("visit()")
    {
        SECTION("Non-const")
        {
            sf::Event closed = sf::Event::Closed{};
            CHECK(closed.visit(visitor) == "Closed (non-const)");

            sf::Event resized = sf::Event::Resized{};
            CHECK(resized.visit(visitor) == "Resized");

            sf::Event keyPressed = sf::Event::KeyPressed{};
            CHECK(keyPressed.visit(visitor) == "KeyPressed");

            sf::Event focusLost = sf::Event::FocusLost{};
            CHECK(focusLost.visit(visitor) == "Other");
        }

        SECTION("Const")
        {
            const sf::Event closed = sf::Event::Closed{};
            CHECK(closed.visit(visitor) == "Closed (const)");

            const sf::Event resized = sf::Event::Resized{};
            CHECK(resized.visit(visitor) == "Other"); // Cannot use non-const reference callback

            const sf::Event keyPressed = sf::Event::KeyPressed{};
            CHECK(keyPressed.visit(visitor) == "KeyPressed");

            const sf::Event focusLost = sf::Event::FocusLost{};
            CHECK(focusLost.visit(visitor) == "Other");
        }

        SECTION("Move-only visitor")
        {
            auto moveOnlyVisitor = [ptr = std::make_unique<std::string_view>("It works")](const auto&) { return *ptr; };

            const sf::Event closed = sf::Event::Closed{};
            CHECK(closed.visit(moveOnlyVisitor) == "It works");
        }
    }
}
