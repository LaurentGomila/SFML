#include <SFML/Graphics/Image.hpp>

// Other 1st party headers
#include <SFML/System/Exception.hpp>
#include <SFML/System/FileInputStream.hpp>

#include <catch2/catch_test_macros.hpp>

#include <GraphicsUtil.hpp>
#include <array>
#include <type_traits>

TEST_CASE("[Graphics] sf::Image")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(std::is_copy_constructible_v<sf::Image>);
        STATIC_CHECK(std::is_copy_assignable_v<sf::Image>);
        STATIC_CHECK(std::is_nothrow_move_constructible_v<sf::Image>);
        STATIC_CHECK(std::is_nothrow_move_assignable_v<sf::Image>);
    }

    SECTION("Construction")
    {
        SECTION("Default constructor")
        {
            const sf::Image image;
            CHECK(image.getSize() == sf::Vector2u());
            CHECK(image.getPixelsPtr() == nullptr);
        }

        SECTION("File constructor")
        {
            SECTION("Invalid file")
            {
                CHECK_THROWS_AS(sf::Image("."), sf::Exception);
                CHECK_THROWS_AS(sf::Image("this/does/not/exist.jpg"), sf::Exception);
            }

            SECTION("Successful load")
            {
                SECTION("bmp")
                {
                    const sf::Image image("Graphics/sfml-logo-big.bmp");
                    CHECK(image.getPixel({0, 0}) == sf::Color::White);
                    CHECK(image.getPixel({200, 150}) == sf::Color(144, 208, 62));
                    CHECK(image.getSize() == sf::Vector2u(1001, 304));
                    CHECK(image.getPixelsPtr() != nullptr);
                }

                SECTION("png")
                {
                    const sf::Image image("Graphics/sfml-logo-big.png");
                    CHECK(image.getPixel({0, 0}) == sf::Color(255, 255, 255, 0));
                    CHECK(image.getPixel({200, 150}) == sf::Color(144, 208, 62));
                    CHECK(image.getSize() == sf::Vector2u(1001, 304));
                    CHECK(image.getPixelsPtr() != nullptr);
                }

                SECTION("jpg")
                {
                    const sf::Image image("Graphics/sfml-logo-big.jpg");
                    CHECK(image.getPixel({0, 0}) == sf::Color::White);
                    CHECK(image.getPixel({200, 150}) == sf::Color(144, 208, 62));
                    CHECK(image.getSize() == sf::Vector2u(1001, 304));
                    CHECK(image.getPixelsPtr() != nullptr);
                }

                SECTION("gif")
                {
                    const sf::Image image("Graphics/sfml-logo-big.gif");
                    CHECK(image.getPixel({0, 0}) == sf::Color::White);
                    CHECK(image.getPixel({200, 150}) == sf::Color(146, 210, 62));
                    CHECK(image.getSize() == sf::Vector2u(1001, 304));
                    CHECK(image.getPixelsPtr() != nullptr);
                }

                SECTION("psd")
                {
                    const sf::Image image("Graphics/sfml-logo-big.psd");
                    CHECK(image.getPixel({0, 0}) == sf::Color::White);
                    CHECK(image.getPixel({200, 150}) == sf::Color(144, 208, 62));
                    CHECK(image.getSize() == sf::Vector2u(1001, 304));
                    CHECK(image.getPixelsPtr() != nullptr);
                }
            }
        }

        SECTION("Memory constructor")
        {
            SECTION("Invalid pointer")
            {
                CHECK_THROWS_AS(sf::Image(nullptr, 1), sf::Exception);
            }

            SECTION("Invalid size")
            {
                const std::byte testByte{0xAB};
                CHECK_THROWS_AS(sf::Image(&testByte, 0), sf::Exception);
            }

            SECTION("Failed load")
            {
                std::vector<std::uint8_t> memory;

                SECTION("Empty")
                {
                    memory.clear();
                }

                SECTION("Junk data")
                {
                    memory = {1, 2, 3, 4};
                }

                CHECK_THROWS_AS(sf::Image(memory.data(), memory.size()), sf::Exception);
            }

            SECTION("Successful load")
            {
                const auto      memory = sf::Image({24, 24}, sf::Color::Green).saveToMemory("png").value();
                const sf::Image image(memory.data(), memory.size());
                CHECK(image.getSize() == sf::Vector2u(24, 24));
                CHECK(image.getPixelsPtr() != nullptr);
                CHECK(image.getPixel({0, 0}) == sf::Color::Green);
                CHECK(image.getPixel({23, 23}) == sf::Color::Green);
            }
        }

        SECTION("Stream constructor")
        {
            sf::FileInputStream stream("Graphics/sfml-logo-big.png");
            const sf::Image     image(stream);
            CHECK(image.getSize() == sf::Vector2u(1001, 304));
            CHECK(image.getPixelsPtr() != nullptr);
            CHECK(image.getPixel({0, 0}) == sf::Color(255, 255, 255, 0));
            CHECK(image.getPixel({200, 150}) == sf::Color(144, 208, 62));
        }

        SECTION("Vector2 constructor")
        {
            const sf::Image image(sf::Vector2u(10, 10));
            CHECK(image.getSize() == sf::Vector2u(10, 10));
            CHECK(image.getPixelsPtr() != nullptr);

            for (std::uint32_t i = 0; i < 10; ++i)
            {
                for (std::uint32_t j = 0; j < 10; ++j)
                {
                    CHECK(image.getPixel(sf::Vector2u(i, j)) == sf::Color::Black);
                }
            }
        }

        SECTION("Vector2 and color constructor")
        {
            const sf::Image image(sf::Vector2u(10, 10), sf::Color::Red);
            CHECK(image.getSize() == sf::Vector2u(10, 10));
            CHECK(image.getPixelsPtr() != nullptr);

            for (std::uint32_t i = 0; i < 10; ++i)
            {
                for (std::uint32_t j = 0; j < 10; ++j)
                {
                    CHECK(image.getPixel(sf::Vector2u(i, j)) == sf::Color::Red);
                }
            }
        }

        SECTION("Vector2 and std::uint8_t* constructor")
        {
            // 10 x 10, with 4 color channels array
            std::array<std::uint8_t, 400> pixels{};
            for (std::size_t i = 0; i < pixels.size(); i += 4)
            {
                pixels[i]     = 255; // r
                pixels[i + 1] = 0;   // g
                pixels[i + 2] = 0;   // b
                pixels[i + 3] = 255; // a
            }

            const sf::Image image(sf::Vector2u(10, 10), pixels.data());
            CHECK(image.getSize() == sf::Vector2u(10, 10));
            CHECK(image.getPixelsPtr() != nullptr);

            for (std::uint32_t i = 0; i < 10; ++i)
            {
                for (std::uint32_t j = 0; j < 10; ++j)
                {
                    CHECK(image.getPixel(sf::Vector2u(i, j)) == sf::Color::Red);
                }
            }
        }
    }

    SECTION("Resize")
    {
        SECTION("resize(Vector2)")
        {
            sf::Image image;
            image.resize(sf::Vector2u(10, 10));
            CHECK(image.getSize() == sf::Vector2u(10, 10));
            CHECK(image.getPixelsPtr() != nullptr);

            for (std::uint32_t i = 0; i < 10; ++i)
            {
                for (std::uint32_t j = 0; j < 10; ++j)
                {
                    CHECK(image.getPixel(sf::Vector2u(i, j)) == sf::Color::Black);
                }
            }
        }

        SECTION("resize(Vector2, Color)")
        {
            sf::Image image;
            image.resize(sf::Vector2u(10, 10), sf::Color::Red);

            CHECK(image.getSize() == sf::Vector2u(10, 10));
            CHECK(image.getPixelsPtr() != nullptr);

            for (std::uint32_t i = 0; i < 10; ++i)
            {
                for (std::uint32_t j = 0; j < 10; ++j)
                {
                    CHECK(image.getPixel(sf::Vector2u(i, j)) == sf::Color::Red);
                }
            }
        }

        SECTION("resize(Vector2, std::uint8_t*)")
        {
            // 10 x 10, with 4 colour channels array
            std::array<std::uint8_t, 400> pixels{};
            for (std::size_t i = 0; i < pixels.size(); i += 4)
            {
                pixels[i]     = 255; // r
                pixels[i + 1] = 0;   // g
                pixels[i + 2] = 0;   // b
                pixels[i + 3] = 255; // a
            }

            sf::Image image;
            image.resize(sf::Vector2u(10, 10), pixels.data());

            CHECK(image.getSize() == sf::Vector2u(10, 10));
            CHECK(image.getPixelsPtr() != nullptr);

            for (std::uint32_t i = 0; i < 10; ++i)
            {
                for (std::uint32_t j = 0; j < 10; ++j)
                {
                    CHECK(image.getPixel(sf::Vector2u(i, j)) == sf::Color::Red);
                }
            }
        }
    }

    SECTION("loadFromFile()")
    {
        sf::Image image;

        SECTION("Invalid file")
        {
            CHECK(!image.loadFromFile("."));
            CHECK(!image.loadFromFile("this/does/not/exist.jpg"));

            // small n with tilde, from Spanish, outside of ASCII, inside common Latin 1 codepage
            CHECK(!image.loadFromFile(std::filesystem::path(U"missing-file-ñ.png")));

            // small n with acute accent, from Polish, outside of Latin 1 codepage
            CHECK(!image.loadFromFile(std::filesystem::path(U"missing-file-ń.png")));

            // CJK symbol for Sun, outside of any European language codepage
            CHECK(!image.loadFromFile(std::filesystem::path(U"missing-file-日.png")));

            // snail emoji, outside of Unicode Basic Multilingual Plane
            CHECK(!image.loadFromFile(std::filesystem::path(U"missing-file-🐌.png")));

            CHECK(image.getSize() == sf::Vector2u(0, 0));
            CHECK(image.getPixelsPtr() == nullptr);
        }

        SECTION("Successful load")
        {
            SECTION("bmp")
            {
                REQUIRE(image.loadFromFile("Graphics/sfml-logo-big.bmp"));
                CHECK(image.getPixel({0, 0}) == sf::Color::White);
                CHECK(image.getPixel({200, 150}) == sf::Color(144, 208, 62));
            }

            SECTION("png")
            {
                REQUIRE(image.loadFromFile("Graphics/sfml-logo-big.png"));
                CHECK(image.getPixel({0, 0}) == sf::Color(255, 255, 255, 0));
                CHECK(image.getPixel({200, 150}) == sf::Color(144, 208, 62));
            }

            SECTION("jpg")
            {
                REQUIRE(image.loadFromFile("Graphics/sfml-logo-big.jpg"));
                CHECK(image.getPixel({0, 0}) == sf::Color::White);
                CHECK(image.getPixel({200, 150}) == sf::Color(144, 208, 62));
            }

            SECTION("gif")
            {
                REQUIRE(image.loadFromFile("Graphics/sfml-logo-big.gif"));
                CHECK(image.getPixel({0, 0}) == sf::Color::White);
                CHECK(image.getPixel({200, 150}) == sf::Color(146, 210, 62));
            }

            SECTION("psd")
            {
                REQUIRE(image.loadFromFile("Graphics/sfml-logo-big.psd"));
                CHECK(image.getPixel({0, 0}) == sf::Color::White);
                CHECK(image.getPixel({200, 150}) == sf::Color(144, 208, 62));
            }

            CHECK(image.getSize() == sf::Vector2u(1001, 304));
            CHECK(image.getPixelsPtr() != nullptr);
        }

        SECTION("Successful then unsuccessful load")
        {
            REQUIRE(image.loadFromFile("Graphics/sfml-logo-big.jpg"));
            CHECK(image.getSize() == sf::Vector2u(1001, 304));
            CHECK(image.getPixelsPtr() != nullptr);

            REQUIRE(!image.loadFromFile("does-not-exist.jpg"));
            CHECK(image.getSize() == sf::Vector2u(1001, 304));
            CHECK(image.getPixelsPtr() != nullptr);
        }
    }

    SECTION("loadFromMemory()")
    {
        sf::Image image;

        SECTION("Invalid pointer")
        {
            CHECK(!image.loadFromMemory(nullptr, 1));
        }

        SECTION("Invalid size")
        {
            const std::byte testByte{0xAB};
            CHECK(!image.loadFromMemory(&testByte, 0));
        }

        SECTION("Failed load")
        {
            std::vector<std::uint8_t> memory;

            SECTION("Empty")
            {
                memory.clear();
            }

            SECTION("Junk data")
            {
                memory = {1, 2, 3, 4};
            }

            CHECK(!image.loadFromMemory(memory.data(), memory.size()));
        }

        const auto memory = []()
        {
            sf::Image savedImage;
            savedImage.resize({24, 24}, sf::Color::Green);
            return savedImage.saveToMemory("png").value();
        }();

        SECTION("Successful load")
        {
            CHECK(image.loadFromMemory(memory.data(), memory.size()));
            CHECK(image.getSize() == sf::Vector2u(24, 24));
            CHECK(image.getPixelsPtr() != nullptr);
            CHECK(image.getPixel({0, 0}) == sf::Color::Green);
            CHECK(image.getPixel({23, 23}) == sf::Color::Green);
        }

        SECTION("Successful then unsuccessful load")
        {
            REQUIRE(image.loadFromMemory(memory.data(), memory.size()));
            CHECK(image.getSize() == sf::Vector2u(24, 24));
            CHECK(image.getPixelsPtr() != nullptr);

            REQUIRE(!image.loadFromMemory(memory.data(), 1));
            CHECK(image.getSize() == sf::Vector2u(24, 24));
            CHECK(image.getPixelsPtr() != nullptr);
        }
    }

    SECTION("loadFromStream()")
    {
        sf::Image           image;
        sf::FileInputStream stream;

        SECTION("Invalid stream")
        {
            CHECK(!image.loadFromStream(stream));
        }

        REQUIRE(stream.open("Graphics/sfml-logo-big.png"));

        SECTION("Successful load")
        {
            REQUIRE(image.loadFromStream(stream));
            CHECK(image.getSize() == sf::Vector2u(1001, 304));
            CHECK(image.getPixelsPtr() != nullptr);
            CHECK(image.getPixel({0, 0}) == sf::Color(255, 255, 255, 0));
            CHECK(image.getPixel({200, 150}) == sf::Color(144, 208, 62));
        }

        SECTION("Successful then unsuccessful load")
        {
            REQUIRE(image.loadFromStream(stream));
            CHECK(image.getSize() == sf::Vector2u(1001, 304));
            CHECK(image.getPixelsPtr() != nullptr);

            sf::FileInputStream emptyStream;
            REQUIRE(!image.loadFromStream(emptyStream));
            CHECK(image.getSize() == sf::Vector2u(1001, 304));
            CHECK(image.getPixelsPtr() != nullptr);
        }
    }

    SECTION("saveToFile()")
    {
        SECTION("Invalid size")
        {
            CHECK(!sf::Image({10, 0}, sf::Color::Magenta).saveToFile("test.jpg"));
            CHECK(!sf::Image({0, 10}, sf::Color::Magenta).saveToFile("test.jpg"));
        }

        const sf::Image image({256, 256}, sf::Color::Magenta);

        SECTION("No extension")
        {
            CHECK(!image.saveToFile("wheresmyextension"));
            CHECK(!image.saveToFile("pls/add/extension"));
        }

        SECTION("Invalid extension")
        {
            CHECK(!image.saveToFile("test.ps"));
            CHECK(!image.saveToFile("test.foo"));
        }

        SECTION("Successful save")
        {
            auto filename = std::filesystem::temp_directory_path();

            SECTION("To .bmp")
            {
                filename /= "test.bmp";
            }

            SECTION("To .tga")
            {
                filename /= "test.tga";
            }

            SECTION("To .png")
            {
                filename /= "test.png";
            }

            SECTION("To Spanish Latin1 filename .png")
            {
                // small n with tilde, from Spanish, outside of ASCII, inside common Latin 1 codepage
                filename /= U"test-ñ.png";
            }

            SECTION("To Polish filename .png")
            {
                // small n with acute accent, from Polish, outside of Latin 1 codepage
                filename /= U"test-ń.png";
            }

            SECTION("To Japanese CJK filename .png")
            {
                // CJK symbol for Sun, outside of any European language codepage
                filename /= U"test-日.png";
            }

            SECTION("To emoji non-BMP Unicode filename .png")
            {
                // snail emoji, outside of Unicode Basic Multilingual Plane
                filename /= U"test-🐌.png";
            }

            // Cannot test JPEG encoding due to it triggering UB in stbiw__jpg_writeBits

            REQUIRE(image.saveToFile(filename));
            REQUIRE(std::filesystem::exists(filename));

            const sf::Image loadedImage(filename);
            CHECK(loadedImage.getSize() == sf::Vector2u(256, 256));
            CHECK(loadedImage.getPixelsPtr() != nullptr);

            CHECK(std::filesystem::remove(filename));
        }
    }

    SECTION("saveToMemory()")
    {
        SECTION("Invalid size")
        {
            CHECK(!sf::Image({10, 0}, sf::Color::Magenta).saveToMemory("test.jpg"));
            CHECK(!sf::Image({0, 10}, sf::Color::Magenta).saveToMemory("test.jpg"));
        }

        const sf::Image image({16, 16}, sf::Color::Magenta);

        SECTION("No extension")
        {
            CHECK(!image.saveToMemory(""));
        }

        SECTION("Invalid extension")
        {
            CHECK(!image.saveToMemory("."));
            CHECK(!image.saveToMemory("gif"));
            CHECK(!image.saveToMemory(".jpg")); // Supposed to be "jpg"
        }

        SECTION("Successful save")
        {
            std::optional<std::vector<std::uint8_t>> maybeOutput;

            SECTION("To bmp")
            {
                maybeOutput = image.saveToMemory("bmp");
                REQUIRE(maybeOutput.has_value());
                const auto& output = *maybeOutput;
                REQUIRE(output.size() == 1146);
                CHECK(output[0] == 66);
                CHECK(output[1] == 77);
                CHECK(output[2] == 122);
                CHECK(output[3] == 4);
                CHECK(output[1000] == 255);
                CHECK(output[1001] == 255);
                CHECK(output[1002] == 255);
                CHECK(output[1003] == 0);
            }

            SECTION("To tga")
            {
                maybeOutput = image.saveToMemory("tga");
                REQUIRE(maybeOutput.has_value());
                const auto& output = *maybeOutput;
                REQUIRE(output.size() == 98);
                CHECK(output[0] == 0);
                CHECK(output[1] == 0);
                CHECK(output[2] == 10);
                CHECK(output[3] == 0);
            }

            SECTION("To png")
            {
                maybeOutput = image.saveToMemory("png");
                REQUIRE(maybeOutput.has_value());
                const auto& output = *maybeOutput;
                REQUIRE(output.size() == 92);
                CHECK(output[0] == 137);
                CHECK(output[1] == 80);
                CHECK(output[2] == 78);
                CHECK(output[3] == 71);
            }

            // Cannot test JPEG encoding due to it triggering UB in stbiw__jpg_writeBits
        }
    }

    SECTION("Set/get pixel")
    {
        sf::Image image(sf::Vector2u(10, 10), sf::Color::Green);
        CHECK(image.getPixel(sf::Vector2u(2, 2)) == sf::Color::Green);

        image.setPixel(sf::Vector2u(2, 2), sf::Color::Blue);
        CHECK(image.getPixel(sf::Vector2u(2, 2)) == sf::Color::Blue);
    }

    SECTION("Copy from Image")
    {
        SECTION("Copy (Image, Vector2u)")
        {
            const sf::Image image1(sf::Vector2u(10, 10), sf::Color::Blue);
            sf::Image       image2(sf::Vector2u(10, 10));
            CHECK(image2.copy(image1, sf::Vector2u(0, 0)));

            for (std::uint32_t i = 0; i < 10; ++i)
            {
                for (std::uint32_t j = 0; j < 10; ++j)
                {
                    CHECK(image1.getPixel(sf::Vector2u(i, j)) == image2.getPixel(sf::Vector2u(i, j)));
                }
            }
        }

        SECTION("Copy (Image, Vector2u, IntRect)")
        {
            const sf::Image image1(sf::Vector2u(5, 5), sf::Color::Blue);
            sf::Image       image2(sf::Vector2u(10, 10));
            CHECK(image2.copy(image1, sf::Vector2u(0, 0), sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(5, 5))));

            for (std::uint32_t i = 0; i < 10; ++i)
            {
                for (std::uint32_t j = 0; j < 10; ++j)
                {
                    if (i <= 4 && j <= 4)
                        CHECK(image2.getPixel(sf::Vector2u(i, j)) == sf::Color::Blue);
                    else
                        CHECK(image2.getPixel(sf::Vector2u(i, j)) == sf::Color::Black);
                }
            }
        }

        SECTION("Copy (Image, Vector2u, IntRect, bool)")
        {
            const sf::Color dest(255, 0, 0, 255);
            const sf::Color source(5, 255, 78, 232);

            // Create the composited color for via the alpha composite over operation
            const auto a = static_cast<std::uint8_t>(source.a + (dest.a * (255 - source.a)) / 255);
            const auto r = static_cast<std::uint8_t>(
                ((source.r * source.a) + ((dest.r * dest.a) * (255 - source.a)) / 255) / a);
            const auto g = static_cast<std::uint8_t>(
                ((source.g * source.a) + ((dest.g * dest.a) * (255 - source.a)) / 255) / a);
            const auto b = static_cast<std::uint8_t>(
                ((source.b * source.a) + ((dest.b * dest.a) * (255 - source.a)) / 255) / a);
            const sf::Color composite(r, g, b, a);

            sf::Image       image1(sf::Vector2u(10, 10), dest);
            const sf::Image image2(sf::Vector2u(10, 10), source);
            CHECK(image1.copy(image2, sf::Vector2u(0, 0), sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(10, 10)), true));

            for (std::uint32_t i = 0; i < 10; ++i)
            {
                for (std::uint32_t j = 0; j < 10; ++j)
                {
                    CHECK(image1.getPixel(sf::Vector2u(i, j)) == composite);
                }
            }
        }

        SECTION("Copy (Out of bounds sourceRect)")
        {
            const sf::Image image1(sf::Vector2u(5, 5), sf::Color::Blue);
            sf::Image       image2(sf::Vector2u(10, 10), sf::Color::Red);
            CHECK(!image2.copy(image1, sf::Vector2u(0, 0), sf::IntRect(sf::Vector2i(5, 5), sf::Vector2i(9, 9))));

            for (std::uint32_t i = 0; i < 10; ++i)
            {
                for (std::uint32_t j = 0; j < 10; ++j)
                {
                    CHECK(image2.getPixel(sf::Vector2u(i, j)) == sf::Color::Red);
                }
            }
        }
    }

    SECTION("Create mask from color")
    {
        SECTION("createMaskFromColor(Color)")
        {
            sf::Image image(sf::Vector2u(10, 10), sf::Color::Blue);
            image.createMaskFromColor(sf::Color::Blue);

            for (std::uint32_t i = 0; i < 10; ++i)
            {
                for (std::uint32_t j = 0; j < 10; ++j)
                {
                    CHECK(image.getPixel(sf::Vector2u(i, j)) == sf::Color(0, 0, 255, 0));
                }
            }
        }

        SECTION("createMaskFromColor(Color, std::uint8_t)")
        {
            sf::Image image(sf::Vector2u(10, 10), sf::Color::Blue);
            image.createMaskFromColor(sf::Color::Blue, 100);

            for (std::uint32_t i = 0; i < 10; ++i)
            {
                for (std::uint32_t j = 0; j < 10; ++j)
                {
                    CHECK(image.getPixel(sf::Vector2u(i, j)) == sf::Color(0, 0, 255, 100));
                }
            }
        }
    }

    SECTION("Flip horizontally")
    {
        sf::Image image(sf::Vector2u(10, 10), sf::Color::Red);
        image.setPixel(sf::Vector2u(0, 0), sf::Color::Green);
        image.flipHorizontally();

        CHECK(image.getPixel(sf::Vector2u(9, 0)) == sf::Color::Green);
    }

    SECTION("Flip vertically")
    {
        sf::Image image(sf::Vector2u(10, 10), sf::Color::Red);
        image.setPixel(sf::Vector2u(0, 0), sf::Color::Green);
        image.flipVertically();

        CHECK(image.getPixel(sf::Vector2u(0, 9)) == sf::Color::Green);
    }
}
