#include <SFML/System/FileInputStream.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string_view>
#include <type_traits>


TEST_CASE("[System] sf::FileInputStream")
{
    using namespace std::string_view_literals;

    SECTION("Type traits")
    {
        STATIC_CHECK(!std::is_copy_constructible_v<sf::FileInputStream>);
        STATIC_CHECK(!std::is_copy_assignable_v<sf::FileInputStream>);
        STATIC_CHECK(std::is_nothrow_move_constructible_v<sf::FileInputStream>);
        STATIC_CHECK(std::is_nothrow_move_assignable_v<sf::FileInputStream>);
    }

    std::array<char, 32> buffer{};

    SECTION("Construction")
    {
        SECTION("Default constructor")
        {
            sf::FileInputStream fileInputStream;
            CHECK(fileInputStream.read(nullptr, 0) == std::nullopt);
            CHECK(fileInputStream.seek(0) == std::nullopt);
            CHECK(fileInputStream.tell() == std::nullopt);
            CHECK(fileInputStream.getSize() == std::nullopt);
        }

        SECTION("File path constructor")
        {
            sf::FileInputStream fileInputStream("System/test.txt");
            CHECK(fileInputStream.read(buffer.data(), 5) == 5);
            CHECK(fileInputStream.tell() == 5);
            CHECK(fileInputStream.getSize() == 12);
            CHECK(std::string_view(buffer.data(), 5) == "Hello"sv);
            CHECK(fileInputStream.seek(6) == 6);
            CHECK(fileInputStream.tell() == 6);
        }
    }

    SECTION("Move semantics")
    {
        SECTION("Move constructor")
        {
            sf::FileInputStream movedFileInputStream("System/test.txt");
            sf::FileInputStream fileInputStream = std::move(movedFileInputStream);
            CHECK(fileInputStream.read(buffer.data(), 6) == 6);
            CHECK(fileInputStream.tell() == 6);
            CHECK(fileInputStream.getSize() == 12);
            CHECK(std::string_view(buffer.data(), 6) == "Hello "sv);
        }

        SECTION("Move assignment")
        {
            sf::FileInputStream movedFileInputStream("System/test.txt");
            sf::FileInputStream fileInputStream("System/test2.txt");
            fileInputStream = std::move(movedFileInputStream);
            CHECK(fileInputStream.read(buffer.data(), 6) == 6);
            CHECK(fileInputStream.tell() == 6);
            CHECK(fileInputStream.getSize() == 12);
            CHECK(std::string_view(buffer.data(), 6) == "Hello "sv);
        }
    }

    SECTION("Temporary file stream open")
    {
        sf::FileInputStream fileInputStream;
        REQUIRE(fileInputStream.open("System/test.txt"));
        CHECK(fileInputStream.read(buffer.data(), 5) == 5);
        CHECK(fileInputStream.tell() == 5);
        CHECK(fileInputStream.getSize() == 12);
        CHECK(std::string_view(buffer.data(), 5) == "Hello"sv);
        CHECK(fileInputStream.seek(6) == 6);
        CHECK(fileInputStream.tell() == 6);
    }

    SECTION("Temporary file stream create")
    {
        sf::FileInputStream fileInputStream("System/test.txt");
        CHECK(fileInputStream.read(buffer.data(), 5) == 5);
        CHECK(fileInputStream.tell() == 5);
        CHECK(fileInputStream.getSize() == 12);
        CHECK(std::string_view(buffer.data(), 5) == "Hello"sv);
        CHECK(fileInputStream.seek(6) == 6);
        CHECK(fileInputStream.tell() == 6);
    }

    SECTION("open()")
    {
        sf::FileInputStream fileInputStream;

        SECTION("From ASCII filename")
        {
            REQUIRE(fileInputStream.open("System/test.txt"));
        }

        SECTION("From Polish filename")
        {
            REQUIRE(fileInputStream.open(U"System/test-ń.txt"));
        }

        SECTION("From emoji filename")
        {
            REQUIRE(fileInputStream.open(U"System/test-🐌.txt"));
        }

        CHECK(fileInputStream.read(buffer.data(), 5) == 5);
        CHECK(fileInputStream.tell() == 5);
        CHECK(fileInputStream.getSize() == 12);
        CHECK(std::string_view(buffer.data(), 5) == "Hello"sv);
        CHECK(fileInputStream.seek(6) == 6);
        CHECK(fileInputStream.tell() == 6);
    }
}
