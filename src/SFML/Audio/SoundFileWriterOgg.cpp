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

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Audio/SoundFileWriterOgg.hpp>

#include <SFML/System/Err.hpp>
#include <SFML/System/Utils.hpp>

#include <algorithm>
#include <ostream>
#include <random>

#include <cassert>


namespace sf::priv
{
////////////////////////////////////////////////////////////
bool SoundFileWriterOgg::check(const std::filesystem::path& filename)
{
    return toLower(filename.extension().string()) == ".ogg";
}


////////////////////////////////////////////////////////////
SoundFileWriterOgg::~SoundFileWriterOgg()
{
    close();
}


////////////////////////////////////////////////////////////
bool SoundFileWriterOgg::open(const std::filesystem::path&     filename,
                              unsigned int                     sampleRate,
                              unsigned int                     channelCount,
                              const std::vector<SoundChannel>& channelMap)
{
    std::vector<SoundChannel> targetChannelMap;

    // For Vorbis channel mapping refer to: https://xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-810004.3.9
    switch (channelCount)
    {
        case 0:
            err() << "No channels to write to Vorbis file" << std::endl;
            return false;
        case 1:
            targetChannelMap = {SoundChannel::Mono};
            break;
        case 2:
            targetChannelMap = {SoundChannel::FrontLeft, SoundChannel::FrontRight};
            break;
        case 3:
            targetChannelMap = {SoundChannel::FrontLeft, SoundChannel::FrontCenter, SoundChannel::FrontRight};
            break;
        case 4:
            targetChannelMap = {SoundChannel::FrontLeft, SoundChannel::FrontRight, SoundChannel::BackLeft, SoundChannel::BackRight};
            break;
        case 5:
            targetChannelMap = {SoundChannel::FrontLeft,
                                SoundChannel::FrontCenter,
                                SoundChannel::FrontRight,
                                SoundChannel::BackLeft,
                                SoundChannel::BackRight};
            break;
        case 6:
            targetChannelMap = {SoundChannel::FrontLeft,
                                SoundChannel::FrontCenter,
                                SoundChannel::FrontRight,
                                SoundChannel::BackLeft,
                                SoundChannel::BackRight,
                                SoundChannel::LowFrequencyEffects};
            break;
        case 7:
            targetChannelMap = {SoundChannel::FrontLeft,
                                SoundChannel::FrontCenter,
                                SoundChannel::FrontRight,
                                SoundChannel::SideLeft,
                                SoundChannel::SideRight,
                                SoundChannel::BackCenter,
                                SoundChannel::LowFrequencyEffects};
            break;
        case 8:
            targetChannelMap = {SoundChannel::FrontLeft,
                                SoundChannel::FrontCenter,
                                SoundChannel::FrontRight,
                                SoundChannel::SideLeft,
                                SoundChannel::SideRight,
                                SoundChannel::BackLeft,
                                SoundChannel::BackRight,
                                SoundChannel::LowFrequencyEffects};
            break;
        default:
            err() << "Vorbis files with more than 8 channels not supported" << std::endl;
            return false;
    }

    // Check if the channel map contains channels that we cannot remap to a mapping supported by Vorbis
    if (!std::is_permutation(channelMap.begin(), channelMap.end(), targetChannelMap.begin()))
    {
        err() << "Provided channel map cannot be reordered to a channel map supported by Vorbis" << std::endl;
        return false;
    }

    // Build the remap table
    for (auto i = 0u; i < channelCount; ++i)
        m_remapTable[i] = static_cast<std::size_t>(
            std::find(channelMap.begin(), channelMap.end(), targetChannelMap[i]) - channelMap.begin());

    // Save the channel count
    m_channelCount = channelCount;

    // Initialize the ogg/vorbis stream
    static std::mt19937 rng(std::random_device{}());
    ogg_stream_init(&m_ogg, std::uniform_int_distribution(0, std::numeric_limits<int>::max())(rng));
    vorbis_info_init(&m_vorbis);

    // Setup the encoder: VBR, automatic bitrate management
    // Quality is in range [-1 .. 1], 0.4 gives ~128 kbps for a 44 KHz stereo sound
    int status = vorbis_encode_init_vbr(&m_vorbis, static_cast<long>(channelCount), static_cast<long>(sampleRate), 0.4f);
    if (status < 0)
    {
        err() << "Failed to write ogg/vorbis file (unsupported bitrate)\n"
              << formatDebugPathInfo(filename) << std::endl;
        close();
        return false;
    }
    vorbis_analysis_init(&m_state, &m_vorbis);

    // Open the file after the vorbis setup is ok
    m_file.open(filename, std::ios::binary);
    if (!m_file)
    {
        err() << "Failed to write ogg/vorbis file (cannot open file)\n" << formatDebugPathInfo(filename) << std::endl;
        close();
        return false;
    }

    // Generate header metadata (leave it empty)
    vorbis_comment comment;
    vorbis_comment_init(&comment);

    // Generate the header packets
    ogg_packet header;
    ogg_packet headerComm;
    ogg_packet headerCode;
    status = vorbis_analysis_headerout(&m_state, &comment, &header, &headerComm, &headerCode);
    vorbis_comment_clear(&comment);
    if (status < 0)
    {
        err() << "Failed to write ogg/vorbis file (cannot generate the headers)\n"
              << formatDebugPathInfo(filename) << std::endl;
        close();
        return false;
    }

    // Write the header packets to the ogg stream
    ogg_stream_packetin(&m_ogg, &header);
    ogg_stream_packetin(&m_ogg, &headerComm);
    ogg_stream_packetin(&m_ogg, &headerCode);

    // This ensures the actual audio data will start on a new page, as per spec
    ogg_page page;
    while (ogg_stream_flush(&m_ogg, &page) > 0)
    {
        m_file.write(reinterpret_cast<const char*>(page.header), page.header_len);
        m_file.write(reinterpret_cast<const char*>(page.body), page.body_len);
    }

    return true;
}


////////////////////////////////////////////////////////////
void SoundFileWriterOgg::write(const std::int16_t* samples, std::uint64_t count)
{
    // Vorbis has issues with buffers that are too large, so we ask for 64K
    constexpr int bufferSize = 65536;

    // A frame contains a sample from each channel
    int frameCount = static_cast<int>(count / m_channelCount);

    while (frameCount > 0)
    {
        // Prepare a buffer to hold our samples
        float** buffer = vorbis_analysis_buffer(&m_state, bufferSize);
        assert(buffer && "Vorbis buffer failed to allocate");

        // Write the samples to the buffer, converted to float and remapped to target channels
        for (int i = 0; i < std::min(frameCount, bufferSize); ++i)
        {
            for (unsigned int j = 0; j < m_channelCount; ++j)
                buffer[j][i] = samples[m_remapTable[j]] / 32767.0f;

            samples += m_channelCount;
        }

        // Tell the library how many samples we've written
        vorbis_analysis_wrote(&m_state, std::min(frameCount, bufferSize));

        frameCount -= bufferSize;

        // Flush any produced block
        flushBlocks();
    }
}


////////////////////////////////////////////////////////////
void SoundFileWriterOgg::flushBlocks()
{
    // Let the library divide uncompressed data into blocks, and process them
    vorbis_block block;
    vorbis_block_init(&m_state, &block);
    while (vorbis_analysis_blockout(&m_state, &block) == 1)
    {
        // Let the automatic bitrate management do its job
        vorbis_analysis(&block, nullptr);
        vorbis_bitrate_addblock(&block);

        // Get new packets from the bitrate management engine
        ogg_packet packet;
        while (vorbis_bitrate_flushpacket(&m_state, &packet))
        {
            // Write the packet to the ogg stream
            ogg_stream_packetin(&m_ogg, &packet);

            // If the stream produced new pages, write them to the output file
            ogg_page page;
            while (ogg_stream_flush(&m_ogg, &page) > 0)
            {
                m_file.write(reinterpret_cast<const char*>(page.header), page.header_len);
                m_file.write(reinterpret_cast<const char*>(page.body), page.body_len);
            }
        }
    }

    // Clear the allocated block
    vorbis_block_clear(&block);
}


////////////////////////////////////////////////////////////
void SoundFileWriterOgg::close()
{
    if (m_file.is_open())
    {
        // Submit an empty packet to mark the end of stream
        vorbis_analysis_wrote(&m_state, 0);
        flushBlocks();

        // Close the file
        m_file.close();
    }

    // Clear all the ogg/vorbis structures
    ogg_stream_clear(&m_ogg);
    vorbis_dsp_clear(&m_state);
    vorbis_info_clear(&m_vorbis);
}

} // namespace sf::priv
