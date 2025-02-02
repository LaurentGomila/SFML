////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Audio.hpp>

#include <iostream>


namespace
{
////////////////////////////////////////////////////////////
/// Play a sound
///
////////////////////////////////////////////////////////////
void playSound()
{
    // Load a sound buffer from a wav file
    const sf::SoundBuffer buffer("resources/killdeer.wav");

    // Display sound information
    std::cout << "killdeer.wav:" << '\n'
              << " " << buffer.getDuration().asSeconds() << " seconds" << '\n'
              << " " << buffer.getSampleRate() << " samples / sec" << '\n'
              << " " << buffer.getChannelCount() << " channels" << '\n';

    // Create a sound instance and play it
    sf::Sound sound(buffer);
    sound.play();

    // Loop while the sound is playing
    while (sound.getStatus() == sf::Sound::Status::Playing)
    {
        // Leave some CPU time for other processes
        sf::sleep(sf::milliseconds(100));

        // Display the playing position
        std::cout << "\rPlaying... " << sound.getPlayingOffset().asSeconds() << " sec        " << std::flush;
    }

    std::cout << '\n' << std::endl;
}


////////////////////////////////////////////////////////////
/// Play a music
///
////////////////////////////////////////////////////////////
void playMusic(const std::filesystem::path& filename)
{
    // Load an ogg music file
    sf::Music music("resources" / filename);

    // Display music information
    std::cout << filename << ":" << '\n'
              << " " << music.getDuration().asSeconds() << " seconds" << '\n'
              << " " << music.getSampleRate() << " samples / sec" << '\n'
              << " " << music.getChannelCount() << " channels" << '\n';

    // Play it
    music.play();

    // Loop while the music is playing
    while (music.getStatus() == sf::Music::Status::Playing)
    {
        // Leave some CPU time for other processes
        sf::sleep(sf::milliseconds(100));

        // Display the playing position
        std::cout << "\rPlaying... " << music.getPlayingOffset().asSeconds() << " sec        " << std::flush;
    }

    std::cout << '\n' << std::endl;
}
} // namespace


////////////////////////////////////////////////////////////
/// Entry point of application
///
/// \return Application exit code
///
////////////////////////////////////////////////////////////
int main()
{
    // Play a sound
    playSound();

    // Play music from an opus file
    playMusic("error.opus");

    // Play music from an ogg file
    playMusic("doodle_pop.ogg");

    // Play music from a flac file
    playMusic("ding.flac");

    // Play music from a mp3 file
    playMusic("ding.mp3");

    // Wait until the user presses 'enter' key
    std::cout << "Press enter to exit..." << std::endl;
    std::cin.ignore(10000, '\n');
}
