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
#include <SFML/Audio/SoundFileReaderOpus.hpp>

#include <SFML/System/Err.hpp>
#include <SFML/System/InputStream.hpp>

#include <ostream>

#include <cassert>
#include <cstdio>


namespace
{
int read(void* data, unsigned char* ptr, int bytes)
{
    auto* stream = static_cast<sf::InputStream*>(data);
    return static_cast<int>(stream->read(ptr, static_cast<std::size_t>(bytes)).value_or(-1));
}

int seek(void* data, opus_int64 signedOffset, int whence)
{
    auto* stream = static_cast<sf::InputStream*>(data);
    auto  offset = static_cast<std::size_t>(signedOffset);
    switch (whence)
    {
        case SEEK_SET:
            break;
        case SEEK_CUR:
            offset += stream->tell().value();
            break;
        case SEEK_END:
            offset = stream->getSize().value() - offset;
    }
    const std::optional position = stream->seek(offset);
    return position ? 0 : -1;
}

opus_int64 tell(void* data)
{
    auto*               stream   = static_cast<sf::InputStream*>(data);
    const std::optional position = stream->tell();
    return position ? static_cast<long>(*position) : -1;
}

const OpusFileCallbacks callbacks = {&read, &seek, &tell, nullptr};
} // namespace

namespace sf::priv
{
////////////////////////////////////////////////////////////
bool SoundFileReaderOpus::check(InputStream& stream)
{
    int          error = 0;
    OggOpusFile* file  = op_test_callbacks(&stream, &callbacks, NULL, 0, &error);

    if (error == 0)
    {
        op_free(file);
        return true;
    }

    return false;
}


////////////////////////////////////////////////////////////
SoundFileReaderOpus::~SoundFileReaderOpus()
{
    close();
}


////////////////////////////////////////////////////////////
std::optional<SoundFileReader::Info> SoundFileReaderOpus::open(InputStream& stream)
{
    // Open the Opus stream
    int error = 0;
    m_opus    = op_open_callbacks(&stream, &callbacks, nullptr, 0, &error);
    if (error != 0)
    {
        err() << "Failed to open Opus file for reading" << std::endl;
        return std::nullopt;
    }

    // Retrieve the music attributes
    const OpusHead* opusHead = op_head(m_opus, -1);
    Info            info;
    info.channelCount = static_cast<unsigned int>(opusHead->channel_count);
    info.sampleCount  = static_cast<std::size_t>(op_pcm_total(m_opus, -1) * opusHead->channel_count);

    // All Opus audio is encoded at 48kHz
    // https://www.opus-codec.org/docs/opusfile_api-0.12/structOpusHead.html#a73b80a913eca33d829f1667caee80d9e
    info.sampleRate = 48000;

    // For Vorbis channel mapping refer to: https://xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-810004.3.9
    switch (info.channelCount)
    {
        case 0:
            err() << "No channels in Opus file" << std::endl;
            break;
        case 1:
            info.channelMap = {SoundChannel::Mono};
            break;
        case 2:
            info.channelMap = {SoundChannel::FrontLeft, SoundChannel::FrontRight};
            break;
        case 3:
            info.channelMap = {SoundChannel::FrontLeft, SoundChannel::FrontCenter, SoundChannel::FrontRight};
            break;
        case 4:
            info.channelMap = {SoundChannel::FrontLeft, SoundChannel::FrontRight, SoundChannel::BackLeft, SoundChannel::BackRight};
            break;
        case 5:
            info.channelMap = {SoundChannel::FrontLeft,
                               SoundChannel::FrontCenter,
                               SoundChannel::FrontRight,
                               SoundChannel::BackLeft,
                               SoundChannel::BackRight};
            break;
        case 6:
            info.channelMap = {SoundChannel::FrontLeft,
                               SoundChannel::FrontCenter,
                               SoundChannel::FrontRight,
                               SoundChannel::BackLeft,
                               SoundChannel::BackRight,
                               SoundChannel::LowFrequencyEffects};
            break;
        case 7:
            info.channelMap = {SoundChannel::FrontLeft,
                               SoundChannel::FrontCenter,
                               SoundChannel::FrontRight,
                               SoundChannel::SideLeft,
                               SoundChannel::SideRight,
                               SoundChannel::BackCenter,
                               SoundChannel::LowFrequencyEffects};
            break;
        case 8:
            info.channelMap = {SoundChannel::FrontLeft,
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
            assert(false);
            break;
    }

    // We must keep the channel count for the seek function
    m_channelCount = info.channelCount;

    return info;
}


////////////////////////////////////////////////////////////
void SoundFileReaderOpus::seek(std::uint64_t sampleOffset)
{
    assert(m_opus && "Opus stream is missing. Call SoundFileReaderOpus::open() to initialize it.");

    op_pcm_seek(m_opus, static_cast<ogg_int64_t>(sampleOffset / m_channelCount));
}


////////////////////////////////////////////////////////////
std::uint64_t SoundFileReaderOpus::read(std::int16_t* samples, std::uint64_t maxCount)
{
    assert(m_opus && "Opus stream is missing. Call SoundFileReaderOpus::open() to initialize it.");

    // Try to read the requested number of samples, stop only on error or end of file
    std::uint64_t count = 0;
    while (count < maxCount)
    {
        const int bytesToRead = static_cast<int>(maxCount - count) * static_cast<int>(sizeof(std::int16_t));
        const long bytesRead  = op_read(m_opus, static_cast<opus_int16*>(samples), bytesToRead, nullptr);
        if (bytesRead > 0)
        {
            const long samplesRead = bytesRead / static_cast<long>(sizeof(std::int16_t));
            count += static_cast<std::uint64_t>(samplesRead);
            samples += samplesRead;
        }
        else
        {
            // error or end of file
            break;
        }
    }

    return count;
}


////////////////////////////////////////////////////////////
void SoundFileReaderOpus::close()
{
    if (m_opus)
    {
        op_free(m_opus);
        m_channelCount = 0;
    }
}

} // namespace sf::priv
