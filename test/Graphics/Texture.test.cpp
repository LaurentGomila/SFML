#include <SFML/Graphics/Texture.hpp>

// Other 1st party headers
#include <SFML/Graphics/Image.hpp>

#include <SFML/System/Exception.hpp>
#include <SFML/System/FileInputStream.hpp>

#include <catch2/catch_test_macros.hpp>

#include <GraphicsUtil.hpp>
#include <WindowUtil.hpp>
#include <array>
#include <type_traits>

TEST_CASE("[Graphics] sf::Texture", runDisplayTests())
{
    SECTION("Type traits")
    {
        STATIC_CHECK(std::is_copy_constructible_v<sf::Texture>);
        STATIC_CHECK(std::is_copy_assignable_v<sf::Texture>);
        STATIC_CHECK(std::is_nothrow_move_constructible_v<sf::Texture>);
        STATIC_CHECK(std::is_nothrow_move_assignable_v<sf::Texture>);
        STATIC_CHECK(std::is_nothrow_swappable_v<sf::Texture>);
    }

    SECTION("Construction")
    {
        SECTION("Default constructor")
        {
            const sf::Texture texture;
            CHECK(texture.getSize() == sf::Vector2u());
            CHECK(!texture.isSmooth());
            CHECK(!texture.isSrgb());
            CHECK(!texture.isRepeated());
            CHECK(texture.getNativeHandle() == 0);
        }

        SECTION("Vector")
        {
            SECTION("At least one zero dimension")
            {
                CHECK_THROWS_AS(sf::Texture(sf::Vector2u()), sf::Exception);
                CHECK_THROWS_AS(sf::Texture(sf::Vector2u(0, 1)), sf::Exception);
                CHECK_THROWS_AS(sf::Texture(sf::Vector2u(1, 0)), sf::Exception);
            }

            SECTION("Valid size")
            {
                const sf::Texture texture(sf::Vector2u(100, 100));
                CHECK(texture.getSize() == sf::Vector2u(100, 100));
                CHECK(texture.getNativeHandle() != 0);
            }

            SECTION("Too large")
            {
                CHECK_THROWS_AS(sf::Texture(sf::Vector2u(100'000, 100'000)), sf::Exception);
                CHECK_THROWS_AS(sf::Texture(sf::Vector2u(1'000'000, 1'000'000)), sf::Exception);
            }
        }

        SECTION("File")
        {
            const sf::Texture texture("Graphics/sfml-logo-big.png");
            CHECK(texture.getSize() == sf::Vector2u(1001, 304));
            CHECK(!texture.isSmooth());
            CHECK(!texture.isSrgb());
            CHECK(!texture.isRepeated());
            CHECK(texture.getNativeHandle() != 0);
        }

        SECTION("Memory")
        {
            const auto        memory = loadIntoMemory("Graphics/sfml-logo-big.png");
            const sf::Texture texture(memory.data(), memory.size());
            CHECK(texture.getSize() == sf::Vector2u(1001, 304));
            CHECK(!texture.isSmooth());
            CHECK(!texture.isSrgb());
            CHECK(!texture.isRepeated());
            CHECK(texture.getNativeHandle() != 0);
        }

        SECTION("Stream")
        {
            sf::FileInputStream stream("Graphics/sfml-logo-big.png");
            const sf::Texture   texture(stream);
            CHECK(texture.getSize() == sf::Vector2u(1001, 304));
            CHECK(!texture.isSmooth());
            CHECK(!texture.isSrgb());
            CHECK(!texture.isRepeated());
            CHECK(texture.getNativeHandle() != 0);
        }

        SECTION("Image")
        {
            SECTION("Subarea of image")
            {
                const sf::Image image(sf::Vector2u(10, 15));

                SECTION("Non-truncated area")
                {
                    const sf::Texture texture(image, false, {{0, 0}, {5, 10}});
                    CHECK(texture.getSize() == sf::Vector2u(5, 10));
                    CHECK(texture.getNativeHandle() != 0);
                }

                SECTION("Truncated area (negative position)")
                {
                    const sf::Texture texture(image, false, {{-5, -5}, {4, 8}});
                    CHECK(texture.getSize() == sf::Vector2u(4, 8));
                    CHECK(texture.getNativeHandle() != 0);
                }

                SECTION("Truncated area (width/height too big)")
                {
                    const sf::Texture texture(image, false, {{5, 5}, {12, 18}});
                    CHECK(texture.getSize() == sf::Vector2u(5, 10));
                    CHECK(texture.getNativeHandle() != 0);
                }
            }
        }
    }

    SECTION("Move semantics")
    {
        SECTION("Construction")
        {
            sf::Texture       movedTexture;
            const sf::Texture texture = std::move(movedTexture);
            CHECK(texture.getSize() == sf::Vector2u());
            CHECK(!texture.isSmooth());
            CHECK(!texture.isSrgb());
            CHECK(!texture.isRepeated());
            CHECK(texture.getNativeHandle() == 0);
            CHECK(!texture.getPixelsFlipped());
        }

        SECTION("Assignment")
        {
            sf::Texture movedTexture;
            sf::Texture texture;
            texture = std::move(movedTexture);
            CHECK(texture.getSize() == sf::Vector2u());
            CHECK(!texture.isSmooth());
            CHECK(!texture.isSrgb());
            CHECK(!texture.isRepeated());
            CHECK(texture.getNativeHandle() == 0);
            CHECK(!texture.getPixelsFlipped());
        }
    }

    SECTION("Move semantics")
    {
        SECTION("Construction")
        {
            sf::Texture       movedTexture(sf::Vector2u(64, 64));
            const sf::Texture texture = std::move(movedTexture);
            CHECK(texture.getSize() == sf::Vector2u(64, 64));
            CHECK(!texture.isSmooth());
            CHECK(!texture.isSrgb());
            CHECK(!texture.isRepeated());
            CHECK(texture.getNativeHandle() != 0);
            CHECK(!texture.getPixelsFlipped());
        }

        SECTION("Assignment")
        {
            sf::Texture movedTexture(sf::Vector2u(64, 64));
            sf::Texture texture(sf::Vector2u(128, 128));
            texture = std::move(movedTexture);
            CHECK(texture.getSize() == sf::Vector2u(64, 64));
            CHECK(!texture.isSmooth());
            CHECK(!texture.isSrgb());
            CHECK(!texture.isRepeated());
            CHECK(texture.getNativeHandle() != 0);
            CHECK(!texture.getPixelsFlipped());
        }
    }

    SECTION("resize()")
    {
        sf::Texture texture;

        SECTION("At least one zero dimension")
        {
            CHECK(!texture.resize({}));
            CHECK(!texture.resize({0, 1}));
            CHECK(!texture.resize({1, 0}));
        }

        SECTION("Valid size")
        {
            CHECK(texture.resize({100, 100}));
            CHECK(texture.getSize() == sf::Vector2u(100, 100));
            CHECK(texture.getNativeHandle() != 0);
        }

        SECTION("Too large")
        {
            CHECK(!texture.resize({100'000, 100'000}));
            CHECK(!texture.resize({1'000'000, 1'000'000}));
        }
    }

    SECTION("loadFromFile()")
    {
        sf::Texture texture;
        REQUIRE(texture.loadFromFile("Graphics/sfml-logo-big.png"));
        CHECK(texture.getSize() == sf::Vector2u(1001, 304));
        CHECK(!texture.isSmooth());
        CHECK(!texture.isSrgb());
        CHECK(!texture.isRepeated());
        CHECK(texture.getNativeHandle() != 0);
        CHECK(!texture.getPixelsFlipped());
    }

    SECTION("loadFromMemory()")
    {
        const auto  memory = loadIntoMemory("Graphics/sfml-logo-big.png");
        sf::Texture texture;
        REQUIRE(texture.loadFromMemory(memory.data(), memory.size()));
        CHECK(texture.getSize() == sf::Vector2u(1001, 304));
        CHECK(!texture.isSmooth());
        CHECK(!texture.isSrgb());
        CHECK(!texture.isRepeated());
        CHECK(texture.getNativeHandle() != 0);
        CHECK(!texture.getPixelsFlipped());
    }

    SECTION("loadFromStream()")
    {
        sf::Texture         texture;
        sf::FileInputStream stream;
        REQUIRE(stream.open("Graphics/sfml-logo-big.png"));
        REQUIRE(texture.loadFromStream(stream));
        CHECK(texture.getSize() == sf::Vector2u(1001, 304));
        CHECK(!texture.isSmooth());
        CHECK(!texture.isSrgb());
        CHECK(!texture.isRepeated());
        CHECK(texture.getNativeHandle() != 0);
        CHECK(!texture.getPixelsFlipped());
    }

    SECTION("loadFromImage()")
    {
        SECTION("Empty image")
        {
            const sf::Image image;
            sf::Texture     texture;
            REQUIRE(!texture.loadFromImage(image));
            REQUIRE(!texture.loadFromImage(image, false, {{0, 0}, {1, 1}}));
        }

        SECTION("Subarea of image")
        {
            sf::Image image;
            image.resize(sf::Vector2u(10, 15));
            sf::Texture texture;

            SECTION("Non-truncated area")
            {
                REQUIRE(texture.loadFromImage(image, false, {{0, 0}, {5, 10}}));
                CHECK(texture.getSize() == sf::Vector2u(5, 10));
            }

            SECTION("Truncated area (negative position)")
            {
                REQUIRE(texture.loadFromImage(image, false, {{-5, -5}, {4, 8}}));
                CHECK(texture.getSize() == sf::Vector2u(4, 8));
            }

            SECTION("Truncated area (width/height too big)")
            {
                REQUIRE(texture.loadFromImage(image, false, {{5, 5}, {12, 18}}));
                CHECK(texture.getSize() == sf::Vector2u(5, 10));
            }

            CHECK(texture.getNativeHandle() != 0);
            CHECK(!texture.getPixelsFlipped());
        }
    }

    SECTION("Copy semantics")
    {
        static constexpr std::array<std::uint8_t, 8> red = {0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF};

        sf::Texture texture(sf::Vector2u(1, 2));
        texture.update(red.data());

        SECTION("Construction")
        {
            const sf::Texture textureCopy(texture); // NOLINT(performance-unnecessary-copy-initialization)
            REQUIRE(textureCopy.getSize() == sf::Vector2u(1, 2));
            CHECK(textureCopy.copyToImage().getPixel(sf::Vector2u(0, 1)) == sf::Color::Red);
        }

        SECTION("Assignment")
        {
            sf::Texture textureCopy(sf::Vector2u(64, 64));
            textureCopy = texture;
            REQUIRE(textureCopy.getSize() == sf::Vector2u(1, 2));
            CHECK(textureCopy.copyToImage().getPixel(sf::Vector2u(0, 1)) == sf::Color::Red);
        }

        CHECK(!texture.getPixelsFlipped());
    }

    SECTION("update()")
    {
        static constexpr std::array<std::uint8_t, 4> yellow = {0xFF, 0xFF, 0x00, 0xFF};
        static constexpr std::array<std::uint8_t, 4> cyan   = {0x00, 0xFF, 0xFF, 0xFF};

        SECTION("Pixels")
        {
            sf::Texture texture(sf::Vector2u(1, 1));
            texture.update(yellow.data());
            CHECK(texture.copyToImage().getPixel(sf::Vector2u(0, 0)) == sf::Color::Yellow);
        }

        SECTION("Pixels, size and destination")
        {
            sf::Texture texture(sf::Vector2u(2, 1));
            texture.update(yellow.data(), sf::Vector2u(1, 1), sf::Vector2u(0, 0));
            texture.update(cyan.data(), sf::Vector2u(1, 1), sf::Vector2u(1, 0));
            CHECK(texture.copyToImage().getPixel(sf::Vector2u(0, 0)) == sf::Color::Yellow);
            CHECK(texture.copyToImage().getPixel(sf::Vector2u(1, 0)) == sf::Color::Cyan);
        }

        SECTION("Another texture")
        {
            sf::Texture otherTexture(sf::Vector2u(1, 1));
            otherTexture.update(cyan.data());
            sf::Texture texture(sf::Vector2u(1, 1));
            texture.update(otherTexture);
            CHECK(texture.copyToImage().getPixel(sf::Vector2u(0, 0)) == sf::Color::Cyan);
        }

        SECTION("Another texture and destination")
        {
            sf::Texture texture(sf::Vector2u(2, 1));
            sf::Texture otherTexture1(sf::Vector2u(1, 1));
            otherTexture1.update(cyan.data());
            sf::Texture otherTexture2(sf::Vector2u(1, 1));
            otherTexture2.update(yellow.data());
            texture.update(otherTexture1, sf::Vector2u(0, 0));
            texture.update(otherTexture2, sf::Vector2u(1, 0));
            CHECK(texture.copyToImage().getPixel(sf::Vector2u(0, 0)) == sf::Color::Cyan);
            CHECK(texture.copyToImage().getPixel(sf::Vector2u(1, 0)) == sf::Color::Yellow);
        }

        SECTION("Image")
        {
            sf::Texture     texture(sf::Vector2u(16, 32));
            const sf::Image image(sf::Vector2u(16, 32), sf::Color::Red);
            texture.update(image);
            CHECK(texture.copyToImage().getPixel(sf::Vector2u(7, 15)) == sf::Color::Red);
        }

        SECTION("Image and destination")
        {
            sf::Texture     texture(sf::Vector2u(16, 32));
            const sf::Image image1(sf::Vector2u(16, 16), sf::Color::Red);
            texture.update(image1);
            const sf::Image image2(sf::Vector2u(16, 16), sf::Color::Green);
            texture.update(image1, sf::Vector2u(0, 0));
            texture.update(image2, sf::Vector2u(0, 16));
            CHECK(texture.copyToImage().getPixel(sf::Vector2u(7, 7)) == sf::Color::Red);
            CHECK(texture.copyToImage().getPixel(sf::Vector2u(7, 22)) == sf::Color::Green);
        }
    }

    SECTION("Set/get smooth")
    {
        sf::Texture texture(sf::Vector2u(64, 64));
        CHECK(!texture.isSmooth());
        texture.setSmooth(true);
        CHECK(texture.isSmooth());
        texture.setSmooth(false);
        CHECK(!texture.isSmooth());
    }

    SECTION("Set/get repeated")
    {
        sf::Texture texture(sf::Vector2u(64, 64));
        CHECK(!texture.isRepeated());
        texture.setRepeated(true);
        CHECK(texture.isRepeated());
        texture.setRepeated(false);
        CHECK(!texture.isRepeated());
    }

    SECTION("generateMipmap()")
    {
        sf::Texture texture(sf::Vector2u(100, 100));
        CHECK(texture.generateMipmap());
    }

    SECTION("swap()")
    {
        static constexpr std::array<std::uint8_t, 4> blue  = {0x00, 0x00, 0xFF, 0xFF};
        static constexpr std::array<std::uint8_t, 8> green = {0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF};

        sf::Texture texture1(sf::Vector2u(1, 1), true);
        texture1.update(blue.data());
        texture1.setSmooth(false);
        texture1.setRepeated(true);

        sf::Texture texture2(sf::Vector2u(2, 1), false);
        texture2.update(green.data());
        texture2.setSmooth(true);
        texture2.setRepeated(false);

        sf::swap(texture1, texture2);
        CHECK_FALSE(texture1.isSrgb());
        CHECK(texture1.isSmooth());
        CHECK_FALSE(texture1.isRepeated());
        // Cannot check texture2.isSrgb() because Srgb is sometimes disabled when using OpenGL ES
        CHECK_FALSE(texture2.isSmooth());
        CHECK(texture2.isRepeated());

        const sf::Image image1 = texture1.copyToImage();
        const sf::Image image2 = texture2.copyToImage();
        REQUIRE(image1.getSize() == sf::Vector2u(2, 1));
        REQUIRE(image2.getSize() == sf::Vector2u(1, 1));
        CHECK(image1.getPixel(sf::Vector2u(1, 0)) == sf::Color::Green);
        CHECK(image2.getPixel(sf::Vector2u(0, 0)) == sf::Color::Blue);
    }

    SECTION("Get Maximum Size")
    {
        CHECK(sf::Texture::getMaximumSize() > 0);
    }
}
