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
#include <SFML/Audio/Export.hpp>

#include <SFML/Audio/SoundFileReader.hpp>

#include <filesystem>
#include <memory>
#include <vector>

#include <cstddef>
#include <cstdint>


namespace sf
{
class Time;
class InputStream;

////////////////////////////////////////////////////////////
/// \brief Provide read access to sound files
///
////////////////////////////////////////////////////////////
class SFML_AUDIO_API InputSoundFile
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor
    ///
    /// Construct an input sound file that is not associated
    /// with a file to read.
    ///
    ////////////////////////////////////////////////////////////
    InputSoundFile() = default;

    ////////////////////////////////////////////////////////////
    /// \brief Construct a sound file from the disk for reading
    ///
    /// The supported audio formats are: WAV (PCM only), OGG/Vorbis, FLAC, MP3.
    /// The supported sample sizes for FLAC and WAV are 8, 16, 24 and 32 bit.
    ///
    /// Because of minimp3_ex limitation, for MP3 files with big (>16kb) APEv2 tag,
    /// it may not be properly removed, tag data will be treated as MP3 data
    /// and there is a low chance of garbage decoded at the end of file.
    /// See also: https://github.com/lieff/minimp3
    ///
    /// \param filename Path of the sound file to load
    ///
    /// \throws sf::Exception if opening the file was unsuccessful
    ///
    ////////////////////////////////////////////////////////////
    explicit InputSoundFile(const std::filesystem::path& filename);

    ////////////////////////////////////////////////////////////
    /// \brief Construct a sound file in memory for reading
    ///
    /// The supported audio formats are: WAV (PCM only), OGG/Vorbis, FLAC.
    /// The supported sample sizes for FLAC and WAV are 8, 16, 24 and 32 bit.
    ///
    /// \param data        Pointer to the file data in memory
    /// \param sizeInBytes Size of the data to load, in bytes
    ///
    /// \throws sf::Exception if opening the file was unsuccessful
    ///
    ////////////////////////////////////////////////////////////
    InputSoundFile(const void* data, std::size_t sizeInBytes);

    ////////////////////////////////////////////////////////////
    /// \brief Construct a sound file from a custom stream for reading
    ///
    /// The supported audio formats are: WAV (PCM only), OGG/Vorbis, FLAC.
    /// The supported sample sizes for FLAC and WAV are 8, 16, 24 and 32 bit.
    ///
    /// \param stream Source stream to read from
    ///
    /// \throws sf::Exception if opening the file was unsuccessful
    ///
    ////////////////////////////////////////////////////////////
    explicit InputSoundFile(InputStream& stream);

    ////////////////////////////////////////////////////////////
    /// \brief Open a sound file from the disk for reading
    ///
    /// The supported audio formats are: WAV (PCM only), OGG/Vorbis, Opus,
    /// FLAC, and MP3.
    /// The supported sample sizes for FLAC and WAV are 8, 16, 24 and 32 bit.
    ///
    /// Because of minimp3_ex limitation, for MP3 files with big (>16kb) APEv2 tag,
    /// it may not be properly removed, tag data will be treated as MP3 data
    /// and there is a low chance of garbage decoded at the end of file.
    /// See also: https://github.com/lieff/minimp3
    ///
    /// \param filename Path of the sound file to load
    ///
    /// \return `true` if the file was successfully opened
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool openFromFile(const std::filesystem::path& filename);

    ////////////////////////////////////////////////////////////
    /// \brief Open a sound file in memory for reading
    ///
    /// The supported audio formats are: WAV (PCM only), OGG/Vorbis, Opus,
    /// FLAC, and MP3.
    /// The supported sample sizes for FLAC and WAV are 8, 16, 24 and 32 bit.
    ///
    /// \param data        Pointer to the file data in memory
    /// \param sizeInBytes Size of the data to load, in bytes
    ///
    /// \return `true` if the file was successfully opened
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool openFromMemory(const void* data, std::size_t sizeInBytes);

    ////////////////////////////////////////////////////////////
    /// \brief Open a sound file from a custom stream for reading
    ///
    /// The supported audio formats are: WAV (PCM only), OGG/Vorbis, Opus,
    /// FLAC, and MP3.
    /// The supported sample sizes for FLAC and WAV are 8, 16, 24 and 32 bit.
    ///
    /// \param stream Source stream to read from
    ///
    /// \return `true` if the file was successfully opened
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool openFromStream(InputStream& stream);

    ////////////////////////////////////////////////////////////
    /// \brief Get the total number of audio samples in the file
    ///
    /// \return Number of samples
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] std::uint64_t getSampleCount() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the number of channels used by the sound
    ///
    /// \return Number of channels (1 = mono, 2 = stereo)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getChannelCount() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the sample rate of the sound
    ///
    /// \return Sample rate, in samples per second
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getSampleRate() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the map of position in sample frame to sound channel
    ///
    /// This is used to map a sample in the sample stream to a
    /// position during spatialization.
    ///
    /// \return Map of position in sample frame to sound channel
    ///
    /// \see `getSampleRate`, `getChannelCount`, `getDuration`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const std::vector<SoundChannel>& getChannelMap() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the total duration of the sound file
    ///
    /// This function is provided for convenience, the duration is
    /// deduced from the other sound file attributes.
    ///
    /// \return Duration of the sound file
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Time getDuration() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the read offset of the file in time
    ///
    /// \return Time position
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Time getTimeOffset() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the read offset of the file in samples
    ///
    /// \return Sample position
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] std::uint64_t getSampleOffset() const;

    ////////////////////////////////////////////////////////////
    /// \brief Change the current read position to the given sample offset
    ///
    /// This function takes a sample offset to provide maximum
    /// precision. If you need to jump to a given time, use the
    /// other overload.
    ///
    /// The sample offset takes the channels into account.
    /// If you have a time offset instead, you can easily find
    /// the corresponding sample offset with the following formula:
    /// `timeInSeconds * sampleRate * channelCount`
    /// If the given offset exceeds to total number of samples,
    /// this function jumps to the end of the sound file.
    ///
    /// \param sampleOffset Index of the sample to jump to, relative to the beginning
    ///
    ////////////////////////////////////////////////////////////
    void seek(std::uint64_t sampleOffset);

    ////////////////////////////////////////////////////////////
    /// \brief Change the current read position to the given time offset
    ///
    /// Using a time offset is handy but imprecise. If you need an accurate
    /// result, consider using the overload which takes a sample offset.
    ///
    /// If the given time exceeds to total duration, this function jumps
    /// to the end of the sound file.
    ///
    /// \param timeOffset Time to jump to, relative to the beginning
    ///
    ////////////////////////////////////////////////////////////
    void seek(Time timeOffset);

    ////////////////////////////////////////////////////////////
    /// \brief Read audio samples from the open file
    ///
    /// \param samples  Pointer to the sample array to fill
    /// \param maxCount Maximum number of samples to read
    ///
    /// \return Number of samples actually read (may be less than \a maxCount)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] std::uint64_t read(std::int16_t* samples, std::uint64_t maxCount);

    ////////////////////////////////////////////////////////////
    /// \brief Close the current file
    ///
    ////////////////////////////////////////////////////////////
    void close();

private:
    ////////////////////////////////////////////////////////////
    /// \brief Deleter for input streams that only conditionally deletes
    ///
    ////////////////////////////////////////////////////////////
    struct SFML_AUDIO_API StreamDeleter
    {
        StreamDeleter(bool theOwned);

        // To accept ownership transfer from usual std::unique_ptr<T>
        template <typename T>
        StreamDeleter(const std::default_delete<T>&);

        void operator()(InputStream* ptr) const;

        bool owned{true};
    };

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    std::unique_ptr<SoundFileReader> m_reader; //!< Reader that handles I/O on the file's format
    std::unique_ptr<InputStream, StreamDeleter> m_stream{nullptr, false}; //!< Input stream used to access the file's data
    std::uint64_t             m_sampleOffset{};                           //!< Sample Read Position
    std::uint64_t             m_sampleCount{};                            //!< Total number of samples in the file
    unsigned int              m_sampleRate{};                             //!< Number of samples per second
    std::vector<SoundChannel> m_channelMap; //!< The map of position in sample frame to sound channel
};

} // namespace sf


////////////////////////////////////////////////////////////
/// \class sf::InputSoundFile
/// \ingroup audio
///
/// This class decodes audio samples from a sound file. It is
/// used internally by higher-level classes such as `sf::SoundBuffer`
/// and `sf::Music`, but can also be useful if you want to process
/// or analyze audio files without playing them, or if you want to
/// implement your own version of `sf::Music` with more specific
/// features.
///
/// Usage example:
/// \code
/// // Open a sound file
/// sf::InputSoundFile file("music.ogg");
///
/// // Print the sound attributes
/// std::cout << "duration: " << file.getDuration().asSeconds() << '\n'
///           << "channels: " << file.getChannelCount() << '\n'
///           << "sample rate: " << file.getSampleRate() << '\n'
///           << "sample count: " << file.getSampleCount() << std::endl;
///
/// // Read and process batches of samples until the end of file is reached
/// std::array<std::int16_t, 1024> samples;
/// std::uint64_t count;
/// do
/// {
///     count = file.read(samples.data(), samples.size());
///
///     // process, analyze, play, convert, or whatever
///     // you want to do with the samples...
/// }
/// while (count > 0);
/// \endcode
///
/// \see `sf::SoundFileReader`, `sf::OutputSoundFile`
///
////////////////////////////////////////////////////////////
