#include <SFML/Audio/InputSoundFile.hpp>

// Other 1st party headers
#include <SFML/System/Exception.hpp>
#include <SFML/System/FileInputStream.hpp>
#include <SFML/System/Time.hpp>

#include <catch2/catch_test_macros.hpp>

#include <SystemUtil.hpp>
#include <array>
#include <fstream>
#include <type_traits>

TEST_CASE("[Audio] sf::InputSoundFile")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(!std::is_copy_constructible_v<sf::InputSoundFile>);
        STATIC_CHECK(!std::is_copy_assignable_v<sf::InputSoundFile>);
        STATIC_CHECK(std::is_nothrow_move_constructible_v<sf::InputSoundFile>);
        STATIC_CHECK(std::is_nothrow_move_assignable_v<sf::InputSoundFile>);
    }

    SECTION("Construction")
    {
        SECTION("Default constructor")
        {
            const sf::InputSoundFile inputSoundFile;
            CHECK(inputSoundFile.getSampleCount() == 0);
            CHECK(inputSoundFile.getChannelCount() == 0);
            CHECK(inputSoundFile.getSampleRate() == 0);
            CHECK(inputSoundFile.getDuration() == sf::Time::Zero);
            CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
            CHECK(inputSoundFile.getSampleOffset() == 0);
        }

        SECTION("Invalid file")
        {
            CHECK_THROWS_AS(sf::InputSoundFile("does/not/exist.wav"), sf::Exception);
        }

        SECTION("Valid file")
        {
            SECTION("flac")
            {
                const sf::InputSoundFile inputSoundFile("Audio/ding.flac");
                CHECK(inputSoundFile.getSampleCount() == 87'798);
                CHECK(inputSoundFile.getChannelCount() == 1);
                CHECK(inputSoundFile.getSampleRate() == 44'100);
                CHECK(inputSoundFile.getDuration() == sf::microseconds(1'990'884));
                CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
                CHECK(inputSoundFile.getSampleOffset() == 0);
            }

            SECTION("mp3")
            {
                const sf::InputSoundFile inputSoundFile("Audio/ding.mp3");
                CHECK(inputSoundFile.getSampleCount() == 87'798);
                CHECK(inputSoundFile.getChannelCount() == 1);
                CHECK(inputSoundFile.getSampleRate() == 44'100);
                CHECK(inputSoundFile.getDuration() == sf::microseconds(1'990'884));
                CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
                CHECK(inputSoundFile.getSampleOffset() == 0);
            }

            SECTION("ogg")
            {
                const sf::InputSoundFile inputSoundFile("Audio/doodle_pop.ogg");
                CHECK(inputSoundFile.getSampleCount() == 2'116'992);
                CHECK(inputSoundFile.getChannelCount() == 2);
                CHECK(inputSoundFile.getSampleRate() == 44'100);
                CHECK(inputSoundFile.getDuration() == sf::microseconds(24'002'176));
                CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
                CHECK(inputSoundFile.getSampleOffset() == 0);
            }

            SECTION("wav")
            {
                const sf::InputSoundFile inputSoundFile("Audio/killdeer.wav");
                CHECK(inputSoundFile.getSampleCount() == 112'941);
                CHECK(inputSoundFile.getChannelCount() == 1);
                CHECK(inputSoundFile.getSampleRate() == 22'050);
                CHECK(inputSoundFile.getDuration() == sf::microseconds(5'122'040));
                CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
                CHECK(inputSoundFile.getSampleOffset() == 0);
            }
        }

        SECTION("Memory")
        {
            const auto               memory = loadIntoMemory("Audio/killdeer.wav");
            const sf::InputSoundFile inputSoundFile(memory.data(), memory.size());
            CHECK(inputSoundFile.getSampleCount() == 112'941);
            CHECK(inputSoundFile.getChannelCount() == 1);
            CHECK(inputSoundFile.getSampleRate() == 22'050);
            CHECK(inputSoundFile.getDuration() == sf::microseconds(5'122'040));
            CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
            CHECK(inputSoundFile.getSampleOffset() == 0);
        }

        SECTION("Stream")
        {
            SECTION("flac")
            {
                sf::FileInputStream      stream("Audio/ding.flac");
                const sf::InputSoundFile inputSoundFile(stream);
                CHECK(inputSoundFile.getSampleCount() == 87'798);
                CHECK(inputSoundFile.getChannelCount() == 1);
                CHECK(inputSoundFile.getSampleRate() == 44'100);
                CHECK(inputSoundFile.getDuration() == sf::microseconds(1'990'884));
                CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
                CHECK(inputSoundFile.getSampleOffset() == 0);
            }

            SECTION("mp3")
            {
                sf::FileInputStream      stream("Audio/ding.mp3");
                const sf::InputSoundFile inputSoundFile(stream);
                CHECK(inputSoundFile.getSampleCount() == 87'798);
                CHECK(inputSoundFile.getChannelCount() == 1);
                CHECK(inputSoundFile.getSampleRate() == 44'100);
                CHECK(inputSoundFile.getDuration() == sf::microseconds(1'990'884));
                CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
                CHECK(inputSoundFile.getSampleOffset() == 0);
            }

            SECTION("ogg")
            {
                sf::FileInputStream      stream("Audio/doodle_pop.ogg");
                const sf::InputSoundFile inputSoundFile(stream);
                CHECK(inputSoundFile.getSampleCount() == 2'116'992);
                CHECK(inputSoundFile.getChannelCount() == 2);
                CHECK(inputSoundFile.getSampleRate() == 44'100);
                CHECK(inputSoundFile.getDuration() == sf::microseconds(24'002'176));
                CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
                CHECK(inputSoundFile.getSampleOffset() == 0);
            }

            SECTION("wav")
            {
                sf::FileInputStream      stream("Audio/killdeer.wav");
                const sf::InputSoundFile inputSoundFile(stream);
                CHECK(inputSoundFile.getSampleCount() == 112'941);
                CHECK(inputSoundFile.getChannelCount() == 1);
                CHECK(inputSoundFile.getSampleRate() == 22'050);
                CHECK(inputSoundFile.getDuration() == sf::microseconds(5'122'040));
                CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
                CHECK(inputSoundFile.getSampleOffset() == 0);
            }
        }
    }

    SECTION("openFromFile()")
    {
        sf::InputSoundFile inputSoundFile;

        SECTION("Invalid file")
        {
            CHECK(!inputSoundFile.openFromFile("does/not/exist.wav"));
        }

        SECTION("Valid file")
        {
            SECTION("flac")
            {
                SECTION("ASCII filename")
                {
                    REQUIRE(inputSoundFile.openFromFile("Audio/ding.flac"));
                }

                SECTION("Polish filename")
                {
                    REQUIRE(inputSoundFile.openFromFile(U"Audio/ding-ń.flac"));
                }

                SECTION("Emoji filename")
                {
                    REQUIRE(inputSoundFile.openFromFile(U"Audio/ding-🐌.flac"));
                }

                CHECK(inputSoundFile.getSampleCount() == 87'798);
                CHECK(inputSoundFile.getChannelCount() == 1);
                CHECK(inputSoundFile.getSampleRate() == 44'100);
                CHECK(inputSoundFile.getDuration() == sf::microseconds(1'990'884));
                CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
                CHECK(inputSoundFile.getSampleOffset() == 0);
            }

            SECTION("mp3")
            {
                SECTION("ASCII filename")
                {
                    REQUIRE(inputSoundFile.openFromFile("Audio/ding.mp3"));
                }

                SECTION("Polish filename")
                {
                    REQUIRE(inputSoundFile.openFromFile(U"Audio/ding-ń.mp3"));
                }

                SECTION("Emoji filename")
                {
                    REQUIRE(inputSoundFile.openFromFile(U"Audio/ding-🐌.mp3"));
                }

                CHECK(inputSoundFile.getSampleCount() == 87'798);
                CHECK(inputSoundFile.getChannelCount() == 1);
                CHECK(inputSoundFile.getSampleRate() == 44'100);
                CHECK(inputSoundFile.getDuration() == sf::microseconds(1'990'884));
                CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
                CHECK(inputSoundFile.getSampleOffset() == 0);
            }

            SECTION("ogg")
            {
                SECTION("ASCII filename")
                {
                    REQUIRE(inputSoundFile.openFromFile("Audio/doodle_pop.ogg"));
                }

                SECTION("Polish filename")
                {
                    REQUIRE(inputSoundFile.openFromFile(U"Audio/doodle_pop-ń.ogg"));
                }

                SECTION("Emoji filename")
                {
                    REQUIRE(inputSoundFile.openFromFile(U"Audio/doodle_pop-🐌.ogg"));
                }

                CHECK(inputSoundFile.getSampleCount() == 2'116'992);
                CHECK(inputSoundFile.getChannelCount() == 2);
                CHECK(inputSoundFile.getSampleRate() == 44'100);
                CHECK(inputSoundFile.getDuration() == sf::microseconds(24'002'176));
                CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
                CHECK(inputSoundFile.getSampleOffset() == 0);
            }

            SECTION("wav")
            {
                SECTION("ASCII filename")
                {
                    REQUIRE(inputSoundFile.openFromFile("Audio/killdeer.wav"));
                }

                SECTION("Polish filename")
                {
                    REQUIRE(inputSoundFile.openFromFile(U"Audio/killdeer-ń.wav"));
                }

                SECTION("Emoji filename")
                {
                    REQUIRE(inputSoundFile.openFromFile(U"Audio/killdeer-🐌.wav"));
                }

                CHECK(inputSoundFile.getSampleCount() == 112'941);
                CHECK(inputSoundFile.getChannelCount() == 1);
                CHECK(inputSoundFile.getSampleRate() == 22'050);
                CHECK(inputSoundFile.getDuration() == sf::microseconds(5'122'040));
                CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
                CHECK(inputSoundFile.getSampleOffset() == 0);
            }
        }
    }

    SECTION("openFromMemory()")
    {
        const auto         memory = loadIntoMemory("Audio/killdeer.wav");
        sf::InputSoundFile inputSoundFile;
        REQUIRE(inputSoundFile.openFromMemory(memory.data(), memory.size()));
        CHECK(inputSoundFile.getSampleCount() == 112'941);
        CHECK(inputSoundFile.getChannelCount() == 1);
        CHECK(inputSoundFile.getSampleRate() == 22'050);
        CHECK(inputSoundFile.getDuration() == sf::microseconds(5'122'040));
        CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
        CHECK(inputSoundFile.getSampleOffset() == 0);
    }

    SECTION("openFromStream()")
    {
        sf::InputSoundFile  inputSoundFile;
        sf::FileInputStream stream;

        SECTION("Invalid stream")
        {
            CHECK(!inputSoundFile.openFromStream(stream));
        }

        SECTION("Valid stream")
        {
            SECTION("flac")
            {
                REQUIRE(stream.open("Audio/ding.flac"));
                REQUIRE(inputSoundFile.openFromStream(stream));
                CHECK(inputSoundFile.getSampleCount() == 87'798);
                CHECK(inputSoundFile.getChannelCount() == 1);
                CHECK(inputSoundFile.getSampleRate() == 44'100);
                CHECK(inputSoundFile.getDuration() == sf::microseconds(1'990'884));
                CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
                CHECK(inputSoundFile.getSampleOffset() == 0);
            }

            SECTION("mp3")
            {
                REQUIRE(stream.open("Audio/ding.mp3"));
                REQUIRE(inputSoundFile.openFromStream(stream));
                CHECK(inputSoundFile.getSampleCount() == 87'798);
                CHECK(inputSoundFile.getChannelCount() == 1);
                CHECK(inputSoundFile.getSampleRate() == 44'100);
                CHECK(inputSoundFile.getDuration() == sf::microseconds(1'990'884));
                CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
                CHECK(inputSoundFile.getSampleOffset() == 0);
            }

            SECTION("ogg")
            {
                REQUIRE(stream.open("Audio/doodle_pop.ogg"));
                REQUIRE(inputSoundFile.openFromStream(stream));
                CHECK(inputSoundFile.getSampleCount() == 2'116'992);
                CHECK(inputSoundFile.getChannelCount() == 2);
                CHECK(inputSoundFile.getSampleRate() == 44'100);
                CHECK(inputSoundFile.getDuration() == sf::microseconds(24'002'176));
                CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
                CHECK(inputSoundFile.getSampleOffset() == 0);
            }

            SECTION("wav")
            {
                REQUIRE(stream.open("Audio/killdeer.wav"));
                REQUIRE(inputSoundFile.openFromStream(stream));
                CHECK(inputSoundFile.getSampleCount() == 112'941);
                CHECK(inputSoundFile.getChannelCount() == 1);
                CHECK(inputSoundFile.getSampleRate() == 22'050);
                CHECK(inputSoundFile.getDuration() == sf::microseconds(5'122'040));
                CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
                CHECK(inputSoundFile.getSampleOffset() == 0);
            }
        }
    }

    SECTION("seek(std::uint64_t)")
    {
        SECTION("flac")
        {
            sf::InputSoundFile inputSoundFile("Audio/ding.flac");
            inputSoundFile.seek(1'000);
            CHECK(inputSoundFile.getTimeOffset() == sf::microseconds(22'675));
            CHECK(inputSoundFile.getSampleOffset() == 1'000);
        }

        SECTION("mp3")
        {
            sf::InputSoundFile inputSoundFile("Audio/ding.mp3");
            inputSoundFile.seek(1'000);
            CHECK(inputSoundFile.getTimeOffset() == sf::microseconds(22'675));
            CHECK(inputSoundFile.getSampleOffset() == 1'000);
        }

        SECTION("ogg")
        {
            sf::InputSoundFile inputSoundFile("Audio/doodle_pop.ogg");
            inputSoundFile.seek(1'000);
            CHECK(inputSoundFile.getTimeOffset() == sf::microseconds(11'337));
            CHECK(inputSoundFile.getSampleOffset() == 1'000);
        }

        SECTION("wav")
        {
            sf::InputSoundFile inputSoundFile("Audio/killdeer.wav");
            inputSoundFile.seek(1'000);
            CHECK(inputSoundFile.getTimeOffset() == sf::microseconds(45'351));
            CHECK(inputSoundFile.getSampleOffset() == 1'000);
        }
    }

    SECTION("seek(Time)")
    {
        sf::InputSoundFile inputSoundFile("Audio/ding.flac");
        inputSoundFile.seek(sf::milliseconds(100));
        CHECK(inputSoundFile.getSampleCount() == 87'798);
        CHECK(inputSoundFile.getChannelCount() == 1);
        CHECK(inputSoundFile.getSampleRate() == 44'100);
        CHECK(inputSoundFile.getDuration() == sf::microseconds(1'990'884));
        CHECK(inputSoundFile.getTimeOffset() == sf::milliseconds(100));
        CHECK(inputSoundFile.getSampleOffset() == 4'410);
    }

    SECTION("read()")
    {
        sf::InputSoundFile inputSoundFile("Audio/ding.flac");

        SECTION("Null address")
        {
            CHECK(inputSoundFile.read(nullptr, 10) == 0);
        }

        std::array<std::int16_t, 4> samples{};

        SECTION("Zero count")
        {
            CHECK(inputSoundFile.read(samples.data(), 0) == 0);
        }

        SECTION("Successful read")
        {
            SECTION("flac")
            {
                inputSoundFile = sf::InputSoundFile("Audio/ding.flac");
                CHECK(inputSoundFile.read(samples.data(), samples.size()) == 4);
                CHECK(samples == std::array<std::int16_t, 4>{0, 1, -1, 4});
                CHECK(inputSoundFile.read(samples.data(), samples.size()) == 4);
                CHECK(samples == std::array<std::int16_t, 4>{1, 4, 9, 6});
            }

            SECTION("mp3")
            {
                inputSoundFile = sf::InputSoundFile("Audio/ding.mp3");
                CHECK(inputSoundFile.read(samples.data(), samples.size()) == 4);
                CHECK(samples == std::array<std::int16_t, 4>{0, -2, 0, 2});
                CHECK(inputSoundFile.read(samples.data(), samples.size()) == 4);
                CHECK(samples == std::array<std::int16_t, 4>{1, 4, 6, 8});
            }

            SECTION("ogg")
            {
                inputSoundFile = sf::InputSoundFile("Audio/doodle_pop.ogg");
                CHECK(inputSoundFile.read(samples.data(), samples.size()) == 4);
                CHECK(samples == std::array<std::int16_t, 4>{-827, -985, -1168, -1319});
                CHECK(inputSoundFile.read(samples.data(), samples.size()) == 4);
                CHECK(samples == std::array<std::int16_t, 4>{-1738, -1883, -2358, -2497});
            }

            SECTION("wav")
            {
                // Cannot be tested since reading from a .wav file triggers UB
            }
        }
    }

    SECTION("close()")
    {
        sf::InputSoundFile inputSoundFile("Audio/ding.flac");
        inputSoundFile.close();
        CHECK(inputSoundFile.getSampleCount() == 0);
        CHECK(inputSoundFile.getChannelCount() == 0);
        CHECK(inputSoundFile.getSampleRate() == 0);
        CHECK(inputSoundFile.getDuration() == sf::Time::Zero);
        CHECK(inputSoundFile.getTimeOffset() == sf::Time::Zero);
        CHECK(inputSoundFile.getSampleOffset() == 0);
    }
}
