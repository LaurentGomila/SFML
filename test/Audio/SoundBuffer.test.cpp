#include <SFML/Audio/SoundBuffer.hpp>

// Other 1st party headers
#include <SFML/System/Exception.hpp>
#include <SFML/System/FileInputStream.hpp>

#include <catch2/catch_test_macros.hpp>

#include <AudioUtil.hpp>
#include <SystemUtil.hpp>
#include <array>
#include <type_traits>

TEST_CASE("[Audio] sf::SoundBuffer", runAudioDeviceTests())
{
    SECTION("Type traits")
    {
        STATIC_CHECK(std::is_copy_constructible_v<sf::SoundBuffer>);
        STATIC_CHECK(std::is_copy_assignable_v<sf::SoundBuffer>);
        STATIC_CHECK(std::is_move_constructible_v<sf::SoundBuffer>);
        STATIC_CHECK(!std::is_nothrow_move_constructible_v<sf::SoundBuffer>);
        STATIC_CHECK(std::is_move_assignable_v<sf::SoundBuffer>);
        STATIC_CHECK(!std::is_nothrow_move_assignable_v<sf::SoundBuffer>);
    }

    SECTION("Construction")
    {
        SECTION("Default constructor")
        {
            const sf::SoundBuffer soundBuffer;
            CHECK(soundBuffer.getSamples() == nullptr);
            CHECK(soundBuffer.getSampleCount() == 0);
            CHECK(soundBuffer.getSampleRate() == 44100);
            CHECK(soundBuffer.getChannelCount() == 1);
            CHECK(soundBuffer.getDuration() == sf::Time::Zero);
        }

        SECTION("File")
        {
            SECTION("Invalid filename")
            {
                CHECK_THROWS_AS(sf::SoundBuffer("does/not/exist.wav"), sf::Exception);
            }

            SECTION("Valid file")
            {
                const sf::SoundBuffer soundBuffer("Audio/ding.flac");
                CHECK(soundBuffer.getSamples() != nullptr);
                CHECK(soundBuffer.getSampleCount() == 87798);
                CHECK(soundBuffer.getSampleRate() == 44100);
                CHECK(soundBuffer.getChannelCount() == 1);
                CHECK(soundBuffer.getDuration() == sf::microseconds(1990884));
            }
        }

        SECTION("Memory")
        {
            SECTION("Invalid memory")
            {
                constexpr std::array<std::byte, 5> memory{};
                CHECK_THROWS_AS(sf::SoundBuffer(memory.data(), memory.size()), sf::Exception);
            }

            SECTION("Valid memory")
            {
                const auto            memory = loadIntoMemory("Audio/ding.flac");
                const sf::SoundBuffer soundBuffer(memory.data(), memory.size());
                CHECK(soundBuffer.getSamples() != nullptr);
                CHECK(soundBuffer.getSampleCount() == 87798);
                CHECK(soundBuffer.getSampleRate() == 44100);
                CHECK(soundBuffer.getChannelCount() == 1);
                CHECK(soundBuffer.getDuration() == sf::microseconds(1990884));
            }
        }

        SECTION("Stream")
        {
            sf::FileInputStream   stream("Audio/ding.flac");
            const sf::SoundBuffer soundBuffer(stream);
            CHECK(soundBuffer.getSamples() != nullptr);
            CHECK(soundBuffer.getSampleCount() == 87798);
            CHECK(soundBuffer.getSampleRate() == 44100);
            CHECK(soundBuffer.getChannelCount() == 1);
            CHECK(soundBuffer.getDuration() == sf::microseconds(1990884));
        }
    }

    SECTION("Copy semantics")
    {
        const sf::SoundBuffer soundBuffer("Audio/ding.flac");

        SECTION("Construction")
        {
            const sf::SoundBuffer soundBufferCopy(soundBuffer); // NOLINT(performance-unnecessary-copy-initialization)
            CHECK(soundBufferCopy.getSamples() != nullptr);
            CHECK(soundBufferCopy.getSampleCount() == 87798);
            CHECK(soundBufferCopy.getSampleRate() == 44100);
            CHECK(soundBufferCopy.getChannelCount() == 1);
            CHECK(soundBufferCopy.getDuration() == sf::microseconds(1990884));
        }

        SECTION("Assignment")
        {
            sf::SoundBuffer soundBufferCopy("Audio/doodle_pop.ogg");
            soundBufferCopy = soundBuffer;
            CHECK(soundBufferCopy.getSamples() != nullptr);
            CHECK(soundBufferCopy.getSampleCount() == 87798);
            CHECK(soundBufferCopy.getSampleRate() == 44100);
            CHECK(soundBufferCopy.getChannelCount() == 1);
            CHECK(soundBufferCopy.getDuration() == sf::microseconds(1990884));
        }
    }

    SECTION("loadFromFile()")
    {
        sf::SoundBuffer soundBuffer;

        SECTION("Invalid filename")
        {
            CHECK(!soundBuffer.loadFromFile("does/not/exist.wav"));
        }

        SECTION("Valid file")
        {
            SECTION("ASCII filename")
            {
                REQUIRE(soundBuffer.loadFromFile("Audio/ding.flac"));
            }

            SECTION("Polish filename")
            {
                REQUIRE(soundBuffer.loadFromFile(U"Audio/ding-ń.flac"));
            }

            SECTION("Emoji filename")
            {
                REQUIRE(soundBuffer.loadFromFile(U"Audio/ding-🐌.flac"));
            }

            CHECK(soundBuffer.getSamples() != nullptr);
            CHECK(soundBuffer.getSampleCount() == 87798);
            CHECK(soundBuffer.getSampleRate() == 44100);
            CHECK(soundBuffer.getChannelCount() == 1);
            CHECK(soundBuffer.getDuration() == sf::microseconds(1990884));
        }
    }

    SECTION("loadFromMemory()")
    {
        sf::SoundBuffer soundBuffer;

        SECTION("Invalid memory")
        {
            CHECK(!soundBuffer.loadFromMemory(nullptr, 0));
            constexpr std::array<std::byte, 5> memory{};
            CHECK(!soundBuffer.loadFromMemory(memory.data(), memory.size()));
        }

        SECTION("Valid memory")
        {
            const auto memory = loadIntoMemory("Audio/ding.flac");
            REQUIRE(soundBuffer.loadFromMemory(memory.data(), memory.size()));
            CHECK(soundBuffer.getSamples() != nullptr);
            CHECK(soundBuffer.getSampleCount() == 87798);
            CHECK(soundBuffer.getSampleRate() == 44100);
            CHECK(soundBuffer.getChannelCount() == 1);
            CHECK(soundBuffer.getDuration() == sf::microseconds(1990884));
        }
    }

    SECTION("loadFromStream()")
    {
        sf::FileInputStream stream;
        sf::SoundBuffer     soundBuffer;

        SECTION("Invalid stream")
        {
            CHECK(!soundBuffer.loadFromStream(stream));
        }

        SECTION("Valid stream")
        {
            REQUIRE(stream.open("Audio/ding.flac"));
            REQUIRE(soundBuffer.loadFromStream(stream));
            CHECK(soundBuffer.getSamples() != nullptr);
            CHECK(soundBuffer.getSampleCount() == 87798);
            CHECK(soundBuffer.getSampleRate() == 44100);
            CHECK(soundBuffer.getChannelCount() == 1);
            CHECK(soundBuffer.getDuration() == sf::microseconds(1990884));
        }
    }

    SECTION("saveToFile()")
    {
        const auto filename = std::filesystem::temp_directory_path() / "ding.flac";

        {
            const sf::SoundBuffer soundBuffer("Audio/ding.flac");
            REQUIRE(soundBuffer.saveToFile(filename));
        }

        const sf::SoundBuffer soundBuffer(filename);
        CHECK(soundBuffer.getSamples() != nullptr);
        CHECK(soundBuffer.getSampleCount() == 87798);
        CHECK(soundBuffer.getSampleRate() == 44100);
        CHECK(soundBuffer.getChannelCount() == 1);
        CHECK(soundBuffer.getDuration() == sf::microseconds(1990884));

        CHECK(std::filesystem::remove(filename));
    }
}
