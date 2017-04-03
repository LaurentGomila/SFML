////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2017 Laurent Gomila (laurent@sfml-dev.org)
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
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/ALCheck.hpp>
#include <SFML/System/Err.hpp>
#include <fstream>


namespace sf
{
////////////////////////////////////////////////////////////
Music::Music() :
m_file    ()
{

}


////////////////////////////////////////////////////////////
Music::~Music()
{
    // We must stop before destroying the file
    stop();
}


////////////////////////////////////////////////////////////
bool Music::openFromFile(const std::string& filename)
{
    // First stop the music if it was already running
    stop();

    // Open the underlying sound file
    if (!m_file.openFromFile(filename))
        return false;

    // Perform common initializations
    initialize();

    return true;
}


////////////////////////////////////////////////////////////
bool Music::openFromMemory(const void* data, std::size_t sizeInBytes)
{
    // First stop the music if it was already running
    stop();

    // Open the underlying sound file
    if (!m_file.openFromMemory(data, sizeInBytes))
        return false;

    // Perform common initializations
    initialize();

    return true;
}


////////////////////////////////////////////////////////////
bool Music::openFromStream(InputStream& stream)
{
    // First stop the music if it was already running
    stop();

    // Open the underlying sound file
    if (!m_file.openFromStream(stream))
        return false;

    // Perform common initializations
    initialize();

    return true;
}


////////////////////////////////////////////////////////////
Time Music::getDuration() const
{
    return m_file.getDuration();
}


////////////////////////////////////////////////////////////
bool Music::onGetData(SoundStream::Chunk& data)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Fill the chunk parameters
    data.samples     = m_samples.data();
    data.sampleCount = static_cast<std::size_t>(m_file.read(m_samples.data(), m_samples.size()));

    // Check if we have stopped obtaining samples or reached the end of the audio file
    return (data.sampleCount != 0) && (m_file.getSampleOffset() < m_file.getSampleCount());
}


////////////////////////////////////////////////////////////
void Music::onSeek(Time timeOffset)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_file.seek(timeOffset);
}


////////////////////////////////////////////////////////////
void Music::initialize()
{
    // Resize the internal buffer so that it can contain 1 second of audio samples
    m_samples.resize(m_file.getSampleRate() * m_file.getChannelCount());

    // Initialize the stream
    SoundStream::initialize(m_file.getChannelCount(), m_file.getSampleRate());
}

} // namespace sf
