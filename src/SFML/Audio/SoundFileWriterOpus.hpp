////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2024 Laurent Gomila (laurent@sfml-dev.org)
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
#include <SFML/Audio/SoundFileWriter.hpp>

#include <opus.h>
#include <ogg/ogg.h>

#include <array>
#include <filesystem>
#include <fstream>

#include <cstdint>


namespace sf::priv
{
////////////////////////////////////////////////////////////
/// \brief Implementation of sound file writer that handles Opus files
///
////////////////////////////////////////////////////////////
class SoundFileWriterOpus : public SoundFileWriter
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Check if this writer can handle a file on disk
    ///
    /// \param filename Path of the sound file to check
    ///
    /// \return `true` if the file can be written by this writer
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static bool check(const std::filesystem::path& filename);

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~SoundFileWriterOpus() override;

    ////////////////////////////////////////////////////////////
    /// \brief Open a sound file for writing
    ///
    /// \param filename     Path of the file to open
    /// \param sampleRate   Sample rate of the sound
    /// \param channelCount Number of channels of the sound
    /// \param channelMap   Map of position in sample frame to sound channel
    ///
    /// \return `true` if the file was successfully opened
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool open(const std::filesystem::path&     filename,
                            unsigned int                     sampleRate,
                            unsigned int                     channelCount,
                            const std::vector<SoundChannel>& channelMap) override;

    ////////////////////////////////////////////////////////////
    /// \brief Write audio samples to the open file
    ///
    /// \param samples Pointer to the sample array to write
    /// \param count   Number of samples to write
    ///
    ////////////////////////////////////////////////////////////
    void write(const std::int16_t* samples, std::uint64_t count) override;

private:
    ////////////////////////////////////////////////////////////
    /// \brief Flush blocks produced by the ogg stream, if any
    ///
    ////////////////////////////////////////////////////////////
    void flushBlocks();

    ////////////////////////////////////////////////////////////
    /// \brief Close the file
    ///
    ////////////////////////////////////////////////////////////
    void close();

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    unsigned int               m_channelCount{};  //!< Channel count of the sound being written
    std::array<std::size_t, 8> m_remapTable{};    //!< Table we use to remap source to target channel order
    std::ofstream              m_file;            //!< Output file
    ogg_stream_state           m_ogg{};           //!< OGG stream
    OpusEncoder*               m_opus{};          //!< Opus handle
    std::uint64_t              m_packageNumber{}; //!<
    std::uint32_t              m_sampleRate{};    //!<
};

} // namespace sf::priv
