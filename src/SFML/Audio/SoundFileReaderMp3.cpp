////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2023 Laurent Gomila (laurent@sfml-dev.org)
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
#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#include <dr_mp3.h>
#undef DR_MP3_NO_STDIO

#include <SFML/Audio/SoundFileReaderMp3.hpp>
#include <SFML/System/InputStream.hpp>
#include <SFML/System/Err.hpp>
#include <algorithm>
#include <cstring>
#include <stdint.h>


namespace
{
std::size_t readCallback(void* userData, void* bufferOut, std::size_t bytesToRead)
{
    sf::InputStream* stream = static_cast<sf::InputStream*>(userData);
    return static_cast<std::size_t>(stream->read(bufferOut, static_cast<sf::Int64>(bytesToRead)));
}

drmp3_bool32 seekCallback(void* userData, int offset, drmp3_seek_origin /* origin */)
{
    sf::InputStream* stream = static_cast<sf::InputStream*>(userData);
    const sf::Int64 position = stream->seek(static_cast<sf::Int64>(offset));
    return position >= 0;
}

bool hasValidId3Tag(const sf::Uint8* header)
{
    return std::memcmp(header, "ID3", 3) == 0 && !((header[5] & 15) || (header[6] & 0x80) || (header[7] & 0x80) || (header[8] & 0x80) || (header[9] & 0x80));
}
}

namespace sf
{
namespace priv
{
////////////////////////////////////////////////////////////
bool SoundFileReaderMp3::check(InputStream& stream)
{
    Uint8 header[10];

    if (static_cast<std::size_t>(stream.read(header, static_cast<Int64>(sizeof(header)))) < sizeof(header))
        return false;

    if (hasValidId3Tag(header))
        return true;

    if (drmp3_hdr_valid(header))
        return true;

    return false;
}


////////////////////////////////////////////////////////////
SoundFileReaderMp3::SoundFileReaderMp3() :
m_numSamples(0),
m_position(0)
{
}


////////////////////////////////////////////////////////////
SoundFileReaderMp3::~SoundFileReaderMp3()
{
    drmp3_uninit(&m_decoder);
}


////////////////////////////////////////////////////////////
bool SoundFileReaderMp3::open(InputStream& stream, Info& info)
{
    // Init mp3 decoder
    if (drmp3_init(&m_decoder, readCallback, seekCallback, &stream, NULL) != 1)
        return false;

    // Retrieve the music attributes
    info.channelCount = static_cast<unsigned int>(m_decoder.channels);
    info.sampleRate   = static_cast<unsigned int>(m_decoder.sampleRate);
    info.sampleCount  = drmp3_get_pcm_frame_count(&m_decoder) * m_decoder.channels;

    m_numSamples      = info.sampleCount;
    return true;
}


////////////////////////////////////////////////////////////
void SoundFileReaderMp3::seek(Uint64 sampleOffset)
{
    m_position = std::min(sampleOffset, m_numSamples);
    drmp3_seek_to_pcm_frame(&m_decoder, m_position);
}


////////////////////////////////////////////////////////////
Uint64 SoundFileReaderMp3::read(Int16* samples, Uint64 maxCount)
{
    // dr_mp3's PCM frame contains one sample per channel
    Uint64 toRead = std::min(maxCount, m_numSamples - m_position) / m_decoder.channels;
    toRead = static_cast<Uint64>(drmp3_read_pcm_frames_s16(&m_decoder, toRead, samples)) * m_decoder.channels;
    m_position += toRead;
    return toRead;
}

} // namespace priv

} // namespace sf
