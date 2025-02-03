#include <SFML/Audio/Music.hpp>

// Other 1st party headers
#include <SFML/System/Exception.hpp>
#include <SFML/System/FileInputStream.hpp>

#include <catch2/catch_test_macros.hpp>

#include <AudioUtil.hpp>
#include <SystemUtil.hpp>
#include <thread>
#include <type_traits>

TEST_CASE("[Audio] sf::Music", runAudioDeviceTests())
{
    SECTION("Type traits")
    {
        STATIC_CHECK(!std::is_copy_constructible_v<sf::Music>);
        STATIC_CHECK(!std::is_copy_assignable_v<sf::Music>);
        STATIC_CHECK(std::is_nothrow_move_constructible_v<sf::Music>);
        STATIC_CHECK(std::is_nothrow_move_assignable_v<sf::Music>);
        STATIC_CHECK(std::has_virtual_destructor_v<sf::Music>);
    }

    SECTION("Span")
    {
        const sf::Music::Span<float> span;
        CHECK(span.offset == 0);
        CHECK(span.length == 0);

        const sf::Music::TimeSpan timeSpan;
        CHECK(timeSpan.offset == sf::Time::Zero);
        CHECK(timeSpan.length == sf::Time::Zero);
    }

    SECTION("Constructor")
    {
        SECTION("Default constructor")
        {
            const sf::Music music;
            CHECK(music.getDuration() == sf::Time::Zero);
            const auto [offset, length] = music.getLoopPoints();
            CHECK(offset == sf::Time::Zero);
            CHECK(length == sf::Time::Zero);
            CHECK(music.getChannelCount() == 0);
            CHECK(music.getSampleRate() == 0);
            CHECK(music.getStatus() == sf::Music::Status::Stopped);
            CHECK(music.getPlayingOffset() == sf::Time::Zero);
            CHECK(!music.isLooping());
        }

        SECTION("File")
        {
            SECTION("Invalid file")
            {
                CHECK_THROWS_AS(sf::Music("does/not/exist.wav"), sf::Exception);
            }

            SECTION("Valid file")
            {
                const sf::Music music("Audio/ding.mp3");
                CHECK(music.getDuration() == sf::microseconds(1990884));
                const auto [offset, length] = music.getLoopPoints();
                CHECK(offset == sf::Time::Zero);
                CHECK(length == sf::microseconds(1990884));
                CHECK(music.getChannelCount() == 1);
                CHECK(music.getSampleRate() == 44100);
                CHECK(music.getStatus() == sf::Music::Status::Stopped);
                CHECK(music.getPlayingOffset() == sf::Time::Zero);
                CHECK(!music.isLooping());
            }
        }

        SECTION("Memory")
        {
            std::vector<std::byte> memory(10, std::byte{0xCA});

            SECTION("Invalid buffer")
            {
                CHECK_THROWS_AS(sf::Music(memory.data(), memory.size()), sf::Exception);
            }

            SECTION("Valid buffer")
            {
                memory = loadIntoMemory("Audio/ding.flac");

                const sf::Music music(memory.data(), memory.size());
                CHECK(music.getDuration() == sf::microseconds(1990884));
                const auto [offset, length] = music.getLoopPoints();
                CHECK(offset == sf::Time::Zero);
                CHECK(length == sf::microseconds(1990884));
                CHECK(music.getChannelCount() == 1);
                CHECK(music.getSampleRate() == 44100);
                CHECK(music.getStatus() == sf::Music::Status::Stopped);
                CHECK(music.getPlayingOffset() == sf::Time::Zero);
                CHECK(!music.isLooping());
            }
        }

        SECTION("Stream")
        {
            sf::FileInputStream stream("Audio/doodle_pop.ogg");
            const sf::Music     music(stream);
            CHECK(music.getDuration() == sf::microseconds(24002176));
            const auto [offset, length] = music.getLoopPoints();
            CHECK(offset == sf::Time::Zero);
            CHECK(length == sf::microseconds(24002176));
            CHECK(music.getChannelCount() == 2);
            CHECK(music.getSampleRate() == 44100);
            CHECK(music.getStatus() == sf::Music::Status::Stopped);
            CHECK(music.getPlayingOffset() == sf::Time::Zero);
            CHECK(!music.isLooping());
        }
    }

    SECTION("openFromFile()")
    {
        sf::Music music;

        SECTION("Invalid file")
        {
            REQUIRE(!music.openFromFile("does/not/exist.wav"));
            CHECK(music.getDuration() == sf::Time::Zero);
            const auto [offset, length] = music.getLoopPoints();
            CHECK(offset == sf::Time::Zero);
            CHECK(length == sf::Time::Zero);
            CHECK(music.getChannelCount() == 0);
            CHECK(music.getSampleRate() == 0);
            CHECK(music.getStatus() == sf::Music::Status::Stopped);
            CHECK(music.getPlayingOffset() == sf::Time::Zero);
            CHECK(!music.isLooping());
        }

        SECTION("Valid file")
        {
            SECTION("ASCII filename")
            {
                REQUIRE(music.openFromFile("Audio/ding.mp3"));
            }

            SECTION("Polish filename")
            {
                REQUIRE(music.openFromFile(U"Audio/ding-ń.mp3"));
            }

            SECTION("Emoji filename")
            {
                REQUIRE(music.openFromFile(U"Audio/ding-🐌.mp3"));
            }

            CHECK(music.getDuration() == sf::microseconds(1990884));
            const auto [offset, length] = music.getLoopPoints();
            CHECK(offset == sf::Time::Zero);
            CHECK(length == sf::microseconds(1990884));
            CHECK(music.getChannelCount() == 1);
            CHECK(music.getSampleRate() == 44100);
            CHECK(music.getStatus() == sf::Music::Status::Stopped);
            CHECK(music.getPlayingOffset() == sf::Time::Zero);
            CHECK(!music.isLooping());
        }
    }

    SECTION("openFromMemory()")
    {
        std::vector<std::byte> memory;
        sf::Music              music;

        SECTION("Invalid buffer")
        {
            REQUIRE(!music.openFromMemory(memory.data(), memory.size()));
            CHECK(music.getDuration() == sf::Time::Zero);
            const auto [offset, length] = music.getLoopPoints();
            CHECK(offset == sf::Time::Zero);
            CHECK(length == sf::Time::Zero);
            CHECK(music.getChannelCount() == 0);
            CHECK(music.getSampleRate() == 0);
            CHECK(music.getStatus() == sf::Music::Status::Stopped);
            CHECK(music.getPlayingOffset() == sf::Time::Zero);
            CHECK(!music.isLooping());
        }

        SECTION("Valid buffer")
        {
            memory = loadIntoMemory("Audio/ding.flac");
            REQUIRE(music.openFromMemory(memory.data(), memory.size()));
            CHECK(music.getDuration() == sf::microseconds(1990884));
            const auto [offset, length] = music.getLoopPoints();
            CHECK(offset == sf::Time::Zero);
            CHECK(length == sf::microseconds(1990884));
            CHECK(music.getChannelCount() == 1);
            CHECK(music.getSampleRate() == 44100);
            CHECK(music.getStatus() == sf::Music::Status::Stopped);
            CHECK(music.getPlayingOffset() == sf::Time::Zero);
            CHECK(!music.isLooping());
        }
    }

    SECTION("openFromStream()")
    {
        sf::FileInputStream stream;
        sf::Music           music;

        SECTION("Invalid stream")
        {
            CHECK(!music.openFromStream(stream));
            CHECK(music.getDuration() == sf::Time::Zero);
            const auto [offset, length] = music.getLoopPoints();
            CHECK(offset == sf::Time::Zero);
            CHECK(length == sf::Time::Zero);
            CHECK(music.getChannelCount() == 0);
            CHECK(music.getSampleRate() == 0);
            CHECK(music.getStatus() == sf::Music::Status::Stopped);
            CHECK(music.getPlayingOffset() == sf::Time::Zero);
            CHECK(!music.isLooping());
        }

        SECTION("Valid stream")
        {
            REQUIRE(stream.open("Audio/doodle_pop.ogg"));
            REQUIRE(music.openFromStream(stream));
            CHECK(music.getDuration() == sf::microseconds(24002176));
            const auto [offset, length] = music.getLoopPoints();
            CHECK(offset == sf::Time::Zero);
            CHECK(length == sf::microseconds(24002176));
            CHECK(music.getChannelCount() == 2);
            CHECK(music.getSampleRate() == 44100);
            CHECK(music.getStatus() == sf::Music::Status::Stopped);
            CHECK(music.getPlayingOffset() == sf::Time::Zero);
            CHECK(!music.isLooping());
        }
    }

    SECTION("play/pause/stop")
    {
        sf::Music music("Audio/ding.mp3");

        // Wait for background thread to start
        music.play();
        while (music.getStatus() == sf::Music::Status::Stopped)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        CHECK(music.getStatus() == sf::Music::Status::Playing);

        // Wait for background thread to pause
        music.pause();
        while (music.getStatus() == sf::Music::Status::Playing)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        CHECK(music.getStatus() == sf::Music::Status::Paused);

        // Wait for background thread to stop
        music.stop();
        while (music.getStatus() == sf::Music::Status::Paused)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        CHECK(music.getStatus() == sf::Music::Status::Stopped);
    }

    SECTION("setLoopPoints()")
    {
        sf::Music music;

        SECTION("Uninitalized")
        {
            music.setLoopPoints({sf::seconds(1), sf::seconds(2)});
            const auto [offset, length] = music.getLoopPoints();
            CHECK(offset == sf::seconds(0));
            CHECK(length == sf::seconds(0));
        }

        REQUIRE(music.openFromFile("Audio/killdeer.wav"));

        SECTION("Within range")
        {
            music.setLoopPoints({sf::seconds(1), sf::seconds(2)});
            const auto [offset, length] = music.getLoopPoints();
            CHECK(offset == sf::seconds(1));
            CHECK(length == sf::seconds(2));
        }

        SECTION("Duration too long")
        {
            music.setLoopPoints({sf::seconds(1), sf::seconds(1'000)});
            const auto [offset, length] = music.getLoopPoints();
            CHECK(offset == sf::seconds(1));
            CHECK(length == sf::microseconds(4'122'040));
        }

        SECTION("Offset too long")
        {
            music.setLoopPoints({sf::seconds(1'000), sf::milliseconds(10)});
            const auto [offset, length] = music.getLoopPoints();
            CHECK(offset == sf::seconds(0));
            CHECK(length == sf::microseconds(5'122'040));
        }

        CHECK(music.getChannelCount() == 1);
        CHECK(music.getSampleRate() == 22050);
        CHECK(music.getStatus() == sf::Music::Status::Stopped);
        CHECK(music.getPlayingOffset() == sf::Time::Zero);
        CHECK(!music.isLooping());
    }
}
