
////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <cstring>
#include <iostream>
#include <iterator>
#include <mutex>


const sf::Uint8 serverAudioData   = 1;
const sf::Uint8 serverEndOfStream = 2;


////////////////////////////////////////////////////////////
/// Customized sound stream for acquiring audio data
/// from the network
////////////////////////////////////////////////////////////
class NetworkAudioStream : public sf::SoundStream
{
public:

    ////////////////////////////////////////////////////////////
    /// Default constructor
    ///
    ////////////////////////////////////////////////////////////
    NetworkAudioStream() :
    m_offset     (0),
    m_hasFinished(false)
    {
        // Set the sound parameters
        initialize(1, 44100);
    }

    ////////////////////////////////////////////////////////////
    /// Run the server, stream audio data from the client
    ///
    ////////////////////////////////////////////////////////////
    void start(unsigned short port)
    {
        if (!m_hasFinished)
        {
            // Listen to the given port for incoming connections
            if (m_listener.listen(port) != sf::Socket::Done)
                return;
            std::cout << "Server is listening to port " << port << ", waiting for connections... " << std::endl;

            // Wait for a connection
            if (m_listener.accept(m_client) != sf::Socket::Done)
                return;
            std::cout << "Client connected: " << m_client.getRemoteAddress() << std::endl;

            // Start playback
            play();

            // Start receiving audio data
            receiveLoop();
        }
        else
        {
            // Start playback
            play();
        }
    }

private:

    ////////////////////////////////////////////////////////////
    /// /see SoundStream::OnGetData
    ///
    ////////////////////////////////////////////////////////////
    bool onGetData(sf::Span<const sf::Int16>& data) override
    {
        // We have reached the end of the buffer and all audio data have been played: we can stop playback
        if ((m_offset >= m_samples.size()) && m_hasFinished)
            return false;

        // No new data has arrived since last update: wait until we get some
        while ((m_offset >= m_samples.size()) && !m_hasFinished)
            sf::sleep(sf::milliseconds(10));

        // Copy samples into a local buffer to avoid synchronization problems
        // (don't forget that we run in two separate threads)
        {
            std::scoped_lock lock(m_mutex);
            m_tempBuffer.assign(m_samples.begin() + static_cast<std::vector<sf::Int64>::difference_type>(m_offset), m_samples.end());
        }

        // Fill audio data to pass to the stream
        data = m_tempBuffer;

        // Update the playing offset
        m_offset += m_tempBuffer.size();

        return true;
    }

    ////////////////////////////////////////////////////////////
    /// /see SoundStream::OnSeek
    ///
    ////////////////////////////////////////////////////////////
    void onSeek(sf::Time timeOffset) override
    {
        m_offset = static_cast<std::size_t>(timeOffset.asMilliseconds()) * getSampleRate() * getChannelCount() / 1000;
    }

    ////////////////////////////////////////////////////////////
    /// Get audio data from the client until playback is stopped
    ///
    ////////////////////////////////////////////////////////////
    void receiveLoop()
    {
        while (!m_hasFinished)
        {
            // Get waiting audio data from the network
            sf::Packet packet;
            if (m_client.receive(packet) != sf::Socket::Done)
                break;

            // Extract the message ID
            sf::Uint8 id;
            packet >> id;

            if (id == serverAudioData)
            {
                // Extract audio samples from the packet, and append it to our samples buffer
                std::size_t sampleCount = (packet.getData().size() - 1) / sizeof(sf::Int16);

                // Don't forget that the other thread can access the sample array at any time
                // (so we protect any operation on it with the mutex)
                {
                    std::scoped_lock lock(m_mutex);
                    std::size_t oldSize = m_samples.size();
                    m_samples.resize(oldSize + sampleCount);
                    std::memcpy(&(m_samples[oldSize]), packet.getData().data() + 1, sampleCount * sizeof(sf::Int16));
                }
            }
            else if (id == serverEndOfStream)
            {
                // End of stream reached: we stop receiving audio data
                std::cout << "Audio data has been 100% received!" << std::endl;
                m_hasFinished = true;
            }
            else
            {
                // Something's wrong...
                std::cout << "Invalid packet received..." << std::endl;
                m_hasFinished = true;
            }
        }
    }

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    sf::TcpListener        m_listener;
    sf::TcpSocket          m_client;
    std::recursive_mutex   m_mutex;
    std::vector<sf::Int16> m_samples;
    std::vector<sf::Int16> m_tempBuffer;
    std::size_t            m_offset;
    bool                   m_hasFinished;
};


////////////////////////////////////////////////////////////
/// Launch a server and wait for incoming audio data from
/// a connected client
///
////////////////////////////////////////////////////////////
void doServer(unsigned short port)
{
    // Build an audio stream to play sound data as it is received through the network
    NetworkAudioStream audioStream;
    audioStream.start(port);

    // Loop until the sound playback is finished
    while (audioStream.getStatus() != sf::SoundStream::Stopped)
    {
        // Leave some CPU time for other threads
        sf::sleep(sf::milliseconds(100));
    }

    std::cin.ignore(10000, '\n');

    // Wait until the user presses 'enter' key
    std::cout << "Press enter to replay the sound..." << std::endl;
    std::cin.ignore(10000, '\n');

    // Replay the sound (just to make sure replaying the received data is OK)
    audioStream.play();

    // Loop until the sound playback is finished
    while (audioStream.getStatus() != sf::SoundStream::Stopped)
    {
        // Leave some CPU time for other threads
        sf::sleep(sf::milliseconds(100));
    }
}
