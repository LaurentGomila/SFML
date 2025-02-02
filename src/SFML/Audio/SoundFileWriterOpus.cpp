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

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Audio/SoundFileWriterOpus.hpp>

#include <SFML/System/Err.hpp>
#include <SFML/System/Utils.hpp>

#include <algorithm>
#include <ostream>
#include <random>

#include <cassert>
#include <cstdint>


namespace
{
// Make sure to write int into buffer little endian
void writeUint32(std::vector<unsigned char>& buffer, const std::uint32_t value)
{
    buffer.push_back(static_cast<unsigned char>(value & 0x000000FF));
    buffer.push_back(static_cast<unsigned char>((value & 0x0000FF00) >> 8));
    buffer.push_back(static_cast<unsigned char>((value & 0x00FF0000) >> 16));
    buffer.push_back(static_cast<unsigned char>((value & 0xFF000000) >> 24));
}
} // namespace

namespace sf::priv
{
////////////////////////////////////////////////////////////
bool SoundFileWriterOpus::check(const std::filesystem::path& filename)
{
    return toLower(filename.extension().string()) == ".opus";
}


////////////////////////////////////////////////////////////
SoundFileWriterOpus::~SoundFileWriterOpus()
{
    close();
}


////////////////////////////////////////////////////////////
bool SoundFileWriterOpus::open(const std::filesystem::path&     filename,
                               unsigned int                     sampleRate,
                               unsigned int                     channelCount,
                               const std::vector<SoundChannel>& channelMap)
{
    std::vector<SoundChannel> targetChannelMap;

    // For Vorbis channel mapping refer to: https://xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-810004.3.9
    switch (channelCount)
    {
        case 0:
            err() << "No channels to write to Opus file" << std::endl;
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
            err() << "Opus files with more than 8 channels not supported" << std::endl;
            return false;
    }

    // Check if the channel map contains channels that we cannot remap to a mapping supported by Opus
    if (!std::is_permutation(channelMap.begin(), channelMap.end(), targetChannelMap.begin()))
    {
        err() << "Provided channel map cannot be reordered to a channel map supported by Opus" << std::endl;
        return false;
    }

    // Build the remap table
    for (auto i = 0u; i < channelCount; ++i)
        m_remapTable[i] = static_cast<std::size_t>(
            std::find(channelMap.begin(), channelMap.end(), targetChannelMap[i]) - channelMap.begin());

    // Save the channel count
    m_channelCount = channelCount;
    m_sampleRate   = sampleRate;

    // Initialize the ogg/opus stream
    static std::mt19937 rng(std::random_device{}());
    if (ogg_stream_init(&m_ogg, std::uniform_int_distribution(0, std::numeric_limits<int>::max())(rng)) == -1)
    {
        err() << "Stream init of ogg/opus failed" << std::endl;
        close();
        return false;
    }

    int status = OPUS_INTERNAL_ERROR;
    m_opus = opus_encoder_create(static_cast<opus_int32>(sampleRate), static_cast<int>(channelCount), OPUS_APPLICATION_AUDIO, &status);
    if (status != OPUS_OK)
    {
        err() << "Failed to write ogg/opus file\n" << formatDebugPathInfo(filename) << std::endl;
        if (status == OPUS_BAD_ARG)
            err() << "Possibly wrong sample rate, allowed are 8000, 12000, 16000, 24000, or 48000 Hz." << std::endl;
        close();
        return false;
    }

    // Open the file after the opus setup is ok
    m_file.open(filename, std::ios::binary);
    if (!m_file)
    {
        err() << "Failed to write opus file (cannot open file)\n" << formatDebugPathInfo(filename) << std::endl;
        close();
        return false;
    }

    // Set bitrate (VBR is default)
    opus_encoder_ctl(m_opus, OPUS_SET_BITRATE(128000));

    // Create opus header MAGICBYTES
    std::vector<unsigned char> headerData({'O', 'p', 'u', 's', 'H', 'e', 'a', 'd'});

    headerData.push_back(1); // Version
    headerData.push_back(static_cast<unsigned char>(channelCount));
    headerData.push_back(0); // Preskip
    headerData.push_back(0);

    writeUint32(headerData, static_cast<std::uint32_t>(sampleRate));

    headerData.push_back(0); // Gain
    headerData.push_back(0);

    headerData.push_back(channelCount > 8 ? 255 : (channelCount > 2)); // Mapping family

    // Map opus header to ogg packet
    ogg_packet op;
    op.packet     = headerData.data();
    op.bytes      = static_cast<long>(headerData.size());
    op.b_o_s      = 1;
    op.e_o_s      = 0;
    op.granulepos = 0;
    op.packetno   = static_cast<ogg_int64_t>(m_packageNumber++);

    // Write the header packet to the ogg stream
    ogg_stream_packetin(&m_ogg, &op);
    flushBlocks();

    // Create comment header, needs to be in a new page
    // commentData initialized with magic bytes
    std::vector<unsigned char> commentData({'O', 'p', 'u', 's', 'T', 'a', 'g', 's'});

    // Vendor string
    const std::string opusVersion(opus_get_version_string());

    // unsigned 32bit integer: Length of vendor string (encoding library)
    writeUint32(commentData, static_cast<std::uint32_t>(opusVersion.size()));
    commentData.insert(commentData.end(), opusVersion.begin(), opusVersion.end());

    // Length of user comments (E.g. one could add an ENCODER tag for SFML)
    writeUint32(commentData, 0);

    op.packet     = &commentData.front();
    op.bytes      = static_cast<long>(commentData.size());
    op.b_o_s      = 0;
    op.e_o_s      = 0;
    op.granulepos = 0;
    op.packetno   = static_cast<ogg_int64_t>(m_packageNumber++);
    ogg_stream_packetin(&m_ogg, &op);

    // This ensures the actual audio data will start on a new page, as per spec
    flushBlocks();

    return true;
}


////////////////////////////////////////////////////////////
void SoundFileWriterOpus::write(const std::int16_t* samples, std::uint64_t count)
{
    assert(m_opus && "Opus stream is missing. Call SoundFileWriterOpus::open() to initialize it.");

    const opus_uint32          frameSize = 960;
    std::vector<unsigned char> buffer(frameSize * m_channelCount);

    std::uint32_t frameNumber = 0;
    std::uint8_t  endOfStream = 0;

    while (count > 0)
    {
        opus_int32 packetSize;

        // Check if wee need to pad the input
        if (count < frameSize * m_channelCount)
        {
            const std::uint32_t     begin = frameNumber * frameSize * m_channelCount;
            std::vector<opus_int16> pad(samples + begin, samples + begin + count);
            pad.insert(pad.end(), (frameSize * m_channelCount) - pad.size(),0);
            packetSize  = opus_encode(m_opus, &pad.front(), frameSize, buffer.data(), static_cast<opus_int32>(buffer.size()));
            endOfStream = 1;
            count       = 0;
        }
        else
        {
            packetSize = opus_encode(m_opus,
                                     samples + (frameNumber * frameSize * m_channelCount),
                                     frameSize,
                                     &buffer.front(),
                                     static_cast<opus_int32>(buffer.size()));
            count -= frameSize * m_channelCount;
        }

        if (packetSize < 0)
        {
            err() << "An error occurred when encoding sound to opus." << std::endl;
            break;
        }

        ogg_packet op;
        op.packet     = &buffer.front();
        op.bytes      = packetSize;
        op.granulepos = frameNumber * frameSize * 48000ul / m_sampleRate;
        op.packetno   = static_cast<ogg_int64_t>(m_packageNumber++);
        op.b_o_s      = 0;
        op.e_o_s      = endOfStream;
        ogg_stream_packetin(&m_ogg, &op);

        frameNumber++;
    }

    // Flush any produced block
    flushBlocks();
}


////////////////////////////////////////////////////////////
void SoundFileWriterOpus::flushBlocks()
{
    ogg_page page;
    while ((ogg_stream_pageout(&m_ogg, &page) > 0) || (ogg_stream_flush(&m_ogg, &page) > 0))
    {
        m_file.write(reinterpret_cast<const char*>(page.header), page.header_len);
        m_file.write(reinterpret_cast<const char*>(page.body), page.body_len);
    }
}


////////////////////////////////////////////////////////////
void SoundFileWriterOpus::close()
{
    if (m_file.is_open())
    {
        flushBlocks();
        m_file.close();
    }

    // Clear all the ogg/opus structures
    ogg_stream_clear(&m_ogg);

    if (m_opus != NULL)
    {
        opus_encoder_destroy(m_opus);
        m_opus = NULL;
    }
}

} // namespace sf::priv
