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
#include <SFML/System/Err.hpp>
#include <SFML/System/Lock.hpp>
#include <SFML/System/Mutex.hpp>
#include <SFML/System/ThreadLocalPtr.hpp>
#include <SFML/System/NonCopyable.hpp>
#include <streambuf>
#include <cstdio>
#include <vector>


namespace
{
// This class will be used as the default streambuf of sf::Err,
// it outputs to stderr by default (to keep the default behavior)
class DefaultErrStreamBuf : public std::streambuf
{
public:

    DefaultErrStreamBuf()
    {
        // Allocate the write buffer
        static const int size = 64;
        char* buffer = new char[size];
        setp(buffer, buffer + size);
    }

    ~DefaultErrStreamBuf()
    {
        // Synchronize
        sync();

        // Delete the write buffer
        delete[] pbase();
    }

private:

    virtual int overflow(int character)
    {
        if ((character != EOF) && (pptr() != epptr()))
        {
            // Valid character
            return sputc(static_cast<char>(character));
        }
        else if (character != EOF)
        {
            // Not enough space in the buffer: synchronize output and try again
            sync();
            return overflow(character);
        }
        else
        {
            // Invalid character: synchronize output
            return sync();
        }
    }

    virtual int sync()
    {
        // Check if there is something into the write buffer
        if (pbase() != pptr())
        {
            // Print the contents of the write buffer into the standard error output
            std::size_t size = static_cast<std::size_t>(pptr() - pbase());
            fwrite(pbase(), 1, size, stderr);

            // Reset the pointer position to the beginning of the write buffer
            setp(pbase(), epptr());
        }

        return 0;
    }
};

// No-op buffer
class NullBuffer : public std::streambuf
{
public:

    int overflow(int character)
    {
        return character;
    }
};

// Groups a std::ostream with its std::streambuf
struct ThreadLocalErr : sf::NonCopyable
{
    DefaultErrStreamBuf buffer;
    std::ostream stream;

    ThreadLocalErr() :
    buffer(),
    stream(&buffer)
    {
    }
};

// Note: the below contraption makes thread-safe usage of sf::err() possible, however it relies 
// on several global variables to be alive when used (i.e. initialized and not yet destroyed).
// If sf::err() is called during global init/exit, UB might occur. There is a check for invocations
// during global exit, but it's best-effort and will leak memory.

typedef sf::ThreadLocalPtr<ThreadLocalErr>   ErrPtr;

ErrPtr                       currentErr(NULL);   //< Per-thread variable holds the current instance of DefaultErrStreamBuf
std::vector<ThreadLocalErr*> activePtrs;         //< Pointers to Err objects across all threads
bool                         isAtexitRegistered; //< Whether the global cleanup has been initialized
bool                         isAtexitExecuted;   //< Whether the global cleanup has run
sf::Mutex                    mutex;              //< Synchronize access to the above book-keeping

// Function called at program exit, destroys all active ThreadLocalErr instances
void destroyErrs()
{
    // Note: since this function has been registered with atexit() *after* the above global variables
    // have been initialized*, it should be invoked *before* they are destroyed, ensuring their liveness.
    // ______
    // * that is, unless err() is called during global init, in which case nothing is guaranteed (likely UB).

    for (std::vector<ThreadLocalErr*>::iterator it = activePtrs.begin(); it != activePtrs.end(); ++it)
        delete *it;

    activePtrs.clear();
    isAtexitExecuted = true;
}

}

namespace sf
{
////////////////////////////////////////////////////////////
std::ostream& err()
{
    // Note: access not synchronized, to not incur locking on every err() invocation.
    if (isAtexitExecuted)
    {
        // Last resort if err() is called after the thread-local instance has been destroyed.
        // If this happens, all that counts is avoiding UB and data races, so the stream
        // will leak memory and won't be operational (standard facilities might anyway be in
        // the process of destruction).
        NullBuffer* buf = new NullBuffer();
        return *new std::ostream(buf);
    }

    // Lazy-initialize thread-local Err provider
    if (currentErr == NULL)
    {
        sf::Lock lock(mutex);

        ThreadLocalErr* newErr = new ThreadLocalErr();

        currentErr = newErr;
        activePtrs.push_back(newErr);

        if (!isAtexitRegistered)
        {
            std::atexit(&destroyErrs);
            isAtexitRegistered = true;
        }
    }

    return currentErr->stream;
}

} // namespace sf
