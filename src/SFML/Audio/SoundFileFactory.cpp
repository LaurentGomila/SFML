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
#include <SFML/Audio/SoundFileFactory.hpp>
#include <SFML/Audio/SoundFileReaderFlac.hpp>
#include <SFML/Audio/SoundFileReaderMp3.hpp>
#include <SFML/Audio/SoundFileReaderOgg.hpp>
#include <SFML/Audio/SoundFileReaderOpus.hpp>
#include <SFML/Audio/SoundFileReaderWav.hpp>
#include <SFML/Audio/SoundFileWriterFlac.hpp>
#include <SFML/Audio/SoundFileWriterOgg.hpp>
#include <SFML/Audio/SoundFileWriterWav.hpp>

#include <SFML/System/Err.hpp>
#include <SFML/System/FileInputStream.hpp>
#include <SFML/System/MemoryInputStream.hpp>
#include <SFML/System/Utils.hpp>

#include <ostream>


namespace sf
{
////////////////////////////////////////////////////////////
std::unique_ptr<SoundFileReader> SoundFileFactory::createReaderFromFilename(const std::filesystem::path& filename)
{
    // Wrap the input file into a file stream
    FileInputStream stream;
    if (!stream.open(filename))
    {
        err() << "Failed to open sound file (couldn't open stream)\n" << formatDebugPathInfo(filename) << std::endl;
        return nullptr;
    }

    // Test the filename in all the registered factories
    for (const auto& [fpCreate, fpCheck] : getReaderFactoryMap())
    {
        if (!stream.seek(0).has_value())
        {
            err() << "Failed to seek sound stream" << std::endl;
            return nullptr;
        }

        if (fpCheck(stream))
            return fpCreate();
    }

    // No suitable reader found
    err() << "Failed to open sound file (format not supported)\n" << formatDebugPathInfo(filename) << std::endl;
    return nullptr;
}


////////////////////////////////////////////////////////////
std::unique_ptr<SoundFileReader> SoundFileFactory::createReaderFromMemory(const void* data, std::size_t sizeInBytes)
{
    // Wrap the memory file into a file stream
    MemoryInputStream stream(data, sizeInBytes);

    // Test the stream for all the registered factories
    for (const auto& [fpCreate, fpCheck] : getReaderFactoryMap())
    {
        if (!stream.seek(0).has_value())
        {
            err() << "Failed to seek sound stream" << std::endl;
            return nullptr;
        }

        if (fpCheck(stream))
            return fpCreate();
    }

    // No suitable reader found
    err() << "Failed to open sound file from memory (format not supported)" << std::endl;
    return nullptr;
}


////////////////////////////////////////////////////////////
std::unique_ptr<SoundFileReader> SoundFileFactory::createReaderFromStream(InputStream& stream)
{
    // Test the stream for all the registered factories
    for (const auto& [fpCreate, fpCheck] : getReaderFactoryMap())
    {
        if (!stream.seek(0).has_value())
        {
            err() << "Failed to seek sound stream" << std::endl;
            return nullptr;
        }

        if (fpCheck(stream))
            return fpCreate();
    }

    // No suitable reader found
    err() << "Failed to open sound file from stream (format not supported)" << std::endl;
    return nullptr;
}


////////////////////////////////////////////////////////////
std::unique_ptr<SoundFileWriter> SoundFileFactory::createWriterFromFilename(const std::filesystem::path& filename)
{
    // Test the filename in all the registered factories
    for (const auto& [fpCreate, fpCheck] : getWriterFactoryMap())
    {
        if (fpCheck(filename))
            return fpCreate();
    }

    // No suitable writer found
    err() << "Failed to open sound file (format not supported)\n" << formatDebugPathInfo(filename) << std::endl;
    return nullptr;
}


////////////////////////////////////////////////////////////
SoundFileFactory::ReaderFactoryMap& SoundFileFactory::getReaderFactoryMap()
{
    // The map is pre-populated with default readers on construction
    static ReaderFactoryMap result{{&priv::createReader<priv::SoundFileReaderFlac>, &priv::SoundFileReaderFlac::check},
                                   {&priv::createReader<priv::SoundFileReaderMp3>, &priv::SoundFileReaderMp3::check},
                                   {&priv::createReader<priv::SoundFileReaderOgg>, &priv::SoundFileReaderOgg::check},
                                   {&priv::createReader<priv::SoundFileReaderOpus>, &priv::SoundFileReaderOpus::check},
                                   {&priv::createReader<priv::SoundFileReaderWav>, &priv::SoundFileReaderWav::check}};

    return result;
}


////////////////////////////////////////////////////////////
SoundFileFactory::WriterFactoryMap& SoundFileFactory::getWriterFactoryMap()
{
    // The map is pre-populated with default writers on construction
    static WriterFactoryMap result{{&priv::createWriter<priv::SoundFileWriterFlac>, &priv::SoundFileWriterFlac::check},
                                   {&priv::createWriter<priv::SoundFileWriterOgg>, &priv::SoundFileWriterOgg::check},
                                   {&priv::createWriter<priv::SoundFileWriterWav>, &priv::SoundFileWriterWav::check}};

    return result;
}

} // namespace sf
