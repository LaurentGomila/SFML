////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2024-2025 Andrew Mickelson
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
#include <SFML/Window/Event.hpp>
#include <SFML/Window/InputImpl.hpp>

#include <SFML/System/EnumArray.hpp>
#include <SFML/System/Err.hpp>

#include <algorithm>
#include <array>
#include <fcntl.h>
#include <linux/input.h>
#include <mutex>
#include <optional>
#include <queue>
#include <sstream>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>


namespace
{
struct TouchSlot
{
    std::optional<unsigned int> oldId;
    std::optional<unsigned int> id;
    sf::Vector2i                pos;
};

std::recursive_mutex inputMutex; // threadsafe? maybe...
sf::Vector2i         mousePos;   // current mouse position

std::vector<int> fileDescriptors; // list of open file descriptors for /dev/input
sf::priv::EnumArray<sf::Mouse::Button, bool, sf::Mouse::ButtonCount> mouseMap{}; // track whether mouse buttons are down
sf::priv::EnumArray<sf::Keyboard::Key, bool, sf::Keyboard::KeyCount> keyMap{};   // track whether keys are down

int                    touchFd = -1;    // file descriptor we have seen MT events on; assumes only 1
std::vector<TouchSlot> touchSlots;      // track the state of each touch "slot"
int                    currentSlot = 0; // which slot are we currently updating?

std::queue<sf::Event> eventQueue;    // events received and waiting to be consumed
constexpr int         maxQueue = 64; // The maximum size we let eventQueue grow to

termios newTerminalConfig, oldTerminalConfig; // Terminal configurations

bool altDown()
{
    return keyMap[sf::Keyboard::Key::LAlt] || keyMap[sf::Keyboard::Key::RAlt];
}
bool controlDown()
{
    return keyMap[sf::Keyboard::Key::LControl] || keyMap[sf::Keyboard::Key::RControl];
}
bool shiftDown()
{
    return keyMap[sf::Keyboard::Key::LShift] || keyMap[sf::Keyboard::Key::RShift];
}
bool systemDown()
{
    return keyMap[sf::Keyboard::Key::LSystem] || keyMap[sf::Keyboard::Key::RSystem];
}
bool capsLockDown()
{
    return false; // TODO: To be implemented, potentially with scancode support
}
bool numLockDown()
{
    return false; // TODO: To be implemented, potentially with scancode support
}
bool scrollLockDown()
{
    return false; // TODO: To be implemented, potentially with scancode support
}

void uninitFileDescriptors()
{
    for (const auto& fileDescriptor : fileDescriptors)
        close(fileDescriptor);
}

#define BITS_PER_LONG        (sizeof(unsigned long) * 8)
#define NBITS(x)             ((((x)-1) / BITS_PER_LONG) + 1)
#define OFF(x)               ((x) % BITS_PER_LONG)
#define LONG(x)              ((x) / BITS_PER_LONG)
#define TEST_BIT(bit, array) (((array)[LONG(bit)] >> OFF(bit)) & 1)

// Only keep fileDescriptors that we think are a keyboard, mouse or touchpad/touchscreen
// Joysticks are handled in /src/SFML/Window/Unix/JoystickImpl.cpp
bool keepFileDescriptor(int fileDesc)
{
    std::array<unsigned long, NBITS(EV_MAX)>  bitmaskEv{};
    std::array<unsigned long, NBITS(KEY_MAX)> bitmaskKey{};
    std::array<unsigned long, NBITS(ABS_MAX)> bitmaskAbs{};
    std::array<unsigned long, NBITS(REL_MAX)> bitmaskRel{};

    ioctl(fileDesc, EVIOCGBIT(0, sizeof(bitmaskEv)), bitmaskEv.data());
    ioctl(fileDesc, EVIOCGBIT(EV_KEY, sizeof(bitmaskKey)), bitmaskKey.data());
    ioctl(fileDesc, EVIOCGBIT(EV_ABS, sizeof(bitmaskAbs)), bitmaskAbs.data());
    ioctl(fileDesc, EVIOCGBIT(EV_REL, sizeof(bitmaskRel)), bitmaskRel.data());

    // This is the keyboard test used by SDL.
    // The first 32 bits are ESC, numbers and Q to D;  If we have any of those,
    // consider it a keyboard device; do not test for KEY_RESERVED, though
    const bool isKeyboard = (bitmaskKey[0] & 0xFFFFFFFE);

    const bool isAbs = TEST_BIT(EV_ABS, bitmaskEv) && TEST_BIT(ABS_X, bitmaskAbs) && TEST_BIT(ABS_Y, bitmaskAbs);

    const bool isRel = TEST_BIT(EV_REL, bitmaskEv) && TEST_BIT(REL_X, bitmaskRel) && TEST_BIT(REL_Y, bitmaskRel);

    const bool isMouse = (isAbs || isRel) && TEST_BIT(BTN_MOUSE, bitmaskKey);

    const bool isTouch = isAbs && (TEST_BIT(BTN_TOOL_FINGER, bitmaskKey) || TEST_BIT(BTN_TOUCH, bitmaskKey));

    return isKeyboard || isMouse || isTouch;
}

void initFileDescriptors()
{
    static bool initialized = false;
    if (initialized)
        return;

    initialized = true;

    for (int i = 0; i < 32; i++)
    {
        std::string        name("/dev/input/event");
        std::ostringstream stream;
        stream << i;
        name += stream.str();

        const int tempFD = open(name.c_str(), O_RDONLY | O_NONBLOCK);

        if (tempFD < 0)
        {
            if (errno != ENOENT)
                sf::err() << "Error opening " << name << ": " << std::strerror(errno) << std::endl;

            continue;
        }

        if (keepFileDescriptor(tempFD))
            fileDescriptors.push_back(tempFD);
        else
            close(tempFD);
    }

    std::atexit(uninitFileDescriptors);
}

std::optional<sf::Mouse::Button> toMouseButton(int code)
{
    switch (code)
    {
        case BTN_LEFT:
            return sf::Mouse::Button::Left;
        case BTN_RIGHT:
            return sf::Mouse::Button::Right;
        case BTN_MIDDLE:
            return sf::Mouse::Button::Middle;
        case BTN_SIDE:
            return sf::Mouse::Button::Extra1;
        case BTN_EXTRA:
            return sf::Mouse::Button::Extra2;

        default:
            return std::nullopt;
    }
}

sf::Keyboard::Key toKey(int code)
{
    switch (code)
    {
            // clang-format off
        case KEY_ESC:           return sf::Keyboard::Key::Escape;
        case KEY_1:             return sf::Keyboard::Key::Num1;
        case KEY_2:             return sf::Keyboard::Key::Num2;
        case KEY_3:             return sf::Keyboard::Key::Num3;
        case KEY_4:             return sf::Keyboard::Key::Num4;
        case KEY_5:             return sf::Keyboard::Key::Num5;
        case KEY_6:             return sf::Keyboard::Key::Num6;
        case KEY_7:             return sf::Keyboard::Key::Num7;
        case KEY_8:             return sf::Keyboard::Key::Num8;
        case KEY_9:             return sf::Keyboard::Key::Num9;
        case KEY_0:             return sf::Keyboard::Key::Num0;
        case KEY_MINUS:         return sf::Keyboard::Key::Hyphen;
        case KEY_EQUAL:         return sf::Keyboard::Key::Equal;
        case KEY_BACKSPACE:     return sf::Keyboard::Key::Backspace;
        case KEY_TAB:           return sf::Keyboard::Key::Tab;
        case KEY_Q:             return sf::Keyboard::Key::Q;
        case KEY_W:             return sf::Keyboard::Key::W;
        case KEY_E:             return sf::Keyboard::Key::E;
        case KEY_R:             return sf::Keyboard::Key::R;
        case KEY_T:             return sf::Keyboard::Key::T;
        case KEY_Y:             return sf::Keyboard::Key::Y;
        case KEY_U:             return sf::Keyboard::Key::U;
        case KEY_I:             return sf::Keyboard::Key::I;
        case KEY_O:             return sf::Keyboard::Key::O;
        case KEY_P:             return sf::Keyboard::Key::P;
        case KEY_LEFTBRACE:     return sf::Keyboard::Key::LBracket;
        case KEY_RIGHTBRACE:    return sf::Keyboard::Key::RBracket;
        case KEY_KPENTER:
        case KEY_ENTER:         return sf::Keyboard::Key::Enter;
        case KEY_LEFTCTRL:      return sf::Keyboard::Key::LControl;
        case KEY_A:             return sf::Keyboard::Key::A;
        case KEY_S:             return sf::Keyboard::Key::S;
        case KEY_D:             return sf::Keyboard::Key::D;
        case KEY_F:             return sf::Keyboard::Key::F;
        case KEY_G:             return sf::Keyboard::Key::G;
        case KEY_H:             return sf::Keyboard::Key::H;
        case KEY_J:             return sf::Keyboard::Key::J;
        case KEY_K:             return sf::Keyboard::Key::K;
        case KEY_L:             return sf::Keyboard::Key::L;
        case KEY_SEMICOLON:     return sf::Keyboard::Key::Semicolon;
        case KEY_APOSTROPHE:    return sf::Keyboard::Key::Apostrophe;
        case KEY_GRAVE:         return sf::Keyboard::Key::Grave;
        case KEY_LEFTSHIFT:     return sf::Keyboard::Key::LShift;
        case KEY_BACKSLASH:     return sf::Keyboard::Key::Backslash;
        case KEY_Z:             return sf::Keyboard::Key::Z;
        case KEY_X:             return sf::Keyboard::Key::X;
        case KEY_C:             return sf::Keyboard::Key::C;
        case KEY_V:             return sf::Keyboard::Key::V;
        case KEY_B:             return sf::Keyboard::Key::B;
        case KEY_N:             return sf::Keyboard::Key::N;
        case KEY_M:             return sf::Keyboard::Key::M;
        case KEY_COMMA:         return sf::Keyboard::Key::Comma;
        case KEY_DOT:           return sf::Keyboard::Key::Period;
        case KEY_SLASH:         return sf::Keyboard::Key::Slash;
        case KEY_RIGHTSHIFT:    return sf::Keyboard::Key::RShift;
        case KEY_KPASTERISK:    return sf::Keyboard::Key::Multiply;
        case KEY_LEFTALT:       return sf::Keyboard::Key::LAlt;
        case KEY_SPACE:         return sf::Keyboard::Key::Space;
        case KEY_F1:            return sf::Keyboard::Key::F1;
        case KEY_F2:            return sf::Keyboard::Key::F2;
        case KEY_F3:            return sf::Keyboard::Key::F3;
        case KEY_F4:            return sf::Keyboard::Key::F4;
        case KEY_F5:            return sf::Keyboard::Key::F5;
        case KEY_F6:            return sf::Keyboard::Key::F6;
        case KEY_F7:            return sf::Keyboard::Key::F7;
        case KEY_F8:            return sf::Keyboard::Key::F8;
        case KEY_F9:            return sf::Keyboard::Key::F9;
        case KEY_F10:           return sf::Keyboard::Key::F10;
        case KEY_F11:           return sf::Keyboard::Key::F11;
        case KEY_F12:           return sf::Keyboard::Key::F12;
        case KEY_F13:           return sf::Keyboard::Key::F13;
        case KEY_F14:           return sf::Keyboard::Key::F14;
        case KEY_F15:           return sf::Keyboard::Key::F15;
        case KEY_KP7:           return sf::Keyboard::Key::Numpad7;
        case KEY_KP8:           return sf::Keyboard::Key::Numpad8;
        case KEY_KP9:           return sf::Keyboard::Key::Numpad9;
        case KEY_KPMINUS:       return sf::Keyboard::Key::Subtract;
        case KEY_KP4:           return sf::Keyboard::Key::Numpad4;
        case KEY_KP5:           return sf::Keyboard::Key::Numpad5;
        case KEY_KP6:           return sf::Keyboard::Key::Numpad6;
        case KEY_KPPLUS:        return sf::Keyboard::Key::Add;
        case KEY_KP1:           return sf::Keyboard::Key::Numpad1;
        case KEY_KP2:           return sf::Keyboard::Key::Numpad2;
        case KEY_KP3:           return sf::Keyboard::Key::Numpad3;
        case KEY_KP0:           return sf::Keyboard::Key::Numpad0;
        case KEY_KPDOT:         return sf::Keyboard::Key::Delete;
        case KEY_RIGHTCTRL:     return sf::Keyboard::Key::RControl;
        case KEY_KPSLASH:       return sf::Keyboard::Key::Divide;
        case KEY_RIGHTALT:      return sf::Keyboard::Key::RAlt;
        case KEY_HOME:          return sf::Keyboard::Key::Home;
        case KEY_UP:            return sf::Keyboard::Key::Up;
        case KEY_PAGEUP:        return sf::Keyboard::Key::PageUp;
        case KEY_LEFT:          return sf::Keyboard::Key::Left;
        case KEY_RIGHT:         return sf::Keyboard::Key::Right;
        case KEY_END:           return sf::Keyboard::Key::End;
        case KEY_DOWN:          return sf::Keyboard::Key::Down;
        case KEY_PAGEDOWN:      return sf::Keyboard::Key::PageDown;
        case KEY_INSERT:        return sf::Keyboard::Key::Insert;
        case KEY_DELETE:        return sf::Keyboard::Key::Delete;
        case KEY_PAUSE:         return sf::Keyboard::Key::Pause;
        case KEY_LEFTMETA:      return sf::Keyboard::Key::LSystem;
        case KEY_RIGHTMETA:     return sf::Keyboard::Key::RSystem;

        case KEY_RESERVED:
        case KEY_SYSRQ:
        case KEY_CAPSLOCK:
        case KEY_NUMLOCK:
        case KEY_SCROLLLOCK:
        default:
            return sf::Keyboard::Key::Unknown;
            // clang-format on
    }
}

void pushEvent(const sf::Event& event)
{
    if (eventQueue.size() >= maxQueue)
        eventQueue.pop();

    eventQueue.push(event);
}

TouchSlot& atSlot(int idx)
{
    if (idx >= static_cast<int>(touchSlots.size()))
        touchSlots.resize(static_cast<std::size_t>(idx + 1));
    return touchSlots.at(static_cast<std::size_t>(idx));
}

void processSlots()
{
    for (auto& slot : touchSlots)
    {
        if (slot.oldId == slot.id)
        {
            pushEvent(sf::Event::TouchMoved{*slot.id, slot.pos});
        }
        else
        {
            if (slot.oldId.has_value())
                pushEvent(sf::Event::TouchEnded{*slot.oldId, slot.pos});
            if (slot.id.has_value())
                pushEvent(sf::Event::TouchBegan{*slot.id, slot.pos});

            slot.oldId = slot.id;
        }
    }
}

std::optional<sf::Event> eventProcess()
{
    const std::lock_guard lock(inputMutex);

    // Ensure that we are initialized
    initFileDescriptors();

    // This is for handling the Backspace and DEL text events, which we
    // generate based on keystrokes (and not stdin)
    static unsigned int doDeferredText = 0;
    if (doDeferredText)
    {
        const auto event = sf::Event::TextEntered{doDeferredText};

        doDeferredText = 0;
        return event;
    }

    ssize_t bytesRead = 0;

    // Check all the open file descriptors for the next event
    for (auto& fileDescriptor : fileDescriptors)
    {
        input_event inputEvent{};
        bytesRead = read(fileDescriptor, &inputEvent, sizeof(inputEvent));

        while (bytesRead > 0)
        {
            if (inputEvent.type == EV_KEY)
            {
                if (const std::optional<sf::Mouse::Button> mb = toMouseButton(inputEvent.code))
                {
                    mouseMap[*mb] = inputEvent.value;

                    if (inputEvent.value)
                        return sf::Event::MouseButtonPressed{*mb, mousePos};

                    return sf::Event::MouseButtonReleased{*mb, mousePos};
                }

                const sf::Keyboard::Key kb = toKey(inputEvent.code);

                unsigned int special = 0;
                if ((kb == sf::Keyboard::Key::Delete) || (kb == sf::Keyboard::Key::Backspace))
                    special = (kb == sf::Keyboard::Key::Delete) ? 127 : 8;

                if (inputEvent.value == 2)
                {
                    // key repeat events
                    //
                    if (special)
                    {
                        return sf::Event::TextEntered{special};
                    }
                }
                else if (kb != sf::Keyboard::Key::Unknown)
                {
                    // key down and key up events
                    //
                    keyMap[kb] = inputEvent.value;

                    if (special && inputEvent.value)
                        doDeferredText = special;

                    const auto makeKeyEvent = [&](auto keyEvent)
                    {
                        keyEvent.code       = kb;
                        keyEvent.scancode   = sf::Keyboard::Scan::Unknown; // TODO: not implemented
                        keyEvent.alt        = altDown();
                        keyEvent.control    = controlDown();
                        keyEvent.shift      = shiftDown();
                        keyEvent.system     = systemDown();
                        keyEvent.capsLock   = capsLockDown();
                        keyEvent.numLock    = numLockDown();
                        keyEvent.scrollLock = scrollLockDown();
                        return keyEvent;
                    };

                    if (inputEvent.value)
                        return makeKeyEvent(sf::Event::KeyPressed{});

                    return makeKeyEvent(sf::Event::KeyReleased{});
                }
            }
            else if (inputEvent.type == EV_REL)
            {
                bool posChange = false;
                switch (inputEvent.code)
                {
                    case REL_X:
                        mousePos.x += inputEvent.value;
                        posChange = true;
                        break;

                    case REL_Y:
                        mousePos.y += inputEvent.value;
                        posChange = true;
                        break;

                    case REL_WHEEL:
                        sf::Event::MouseWheelScrolled mouseWheelScrolled;
                        mouseWheelScrolled.delta    = static_cast<float>(inputEvent.value);
                        mouseWheelScrolled.position = mousePos;
                        return mouseWheelScrolled;
                }
                if (posChange)
                {
                    return sf::Event::MouseMoved{mousePos};
                }
            }
            else if (inputEvent.type == EV_ABS)
            {
                switch (inputEvent.code)
                {
                    case ABS_MT_SLOT:
                        currentSlot = inputEvent.value;
                        touchFd     = fileDescriptor;
                        break;
                    case ABS_MT_TRACKING_ID:
                        atSlot(currentSlot).id = inputEvent.value >= 0 ? std::optional(inputEvent.value) : std::nullopt;
                        touchFd                = fileDescriptor;
                        break;
                    case ABS_MT_POSITION_X:
                        atSlot(currentSlot).pos.x = inputEvent.value;
                        touchFd                   = fileDescriptor;
                        break;
                    case ABS_MT_POSITION_Y:
                        atSlot(currentSlot).pos.y = inputEvent.value;
                        touchFd                   = fileDescriptor;
                        break;
                }
            }
            else if (inputEvent.type == EV_SYN && inputEvent.code == SYN_REPORT && fileDescriptor == touchFd)
            {
                // This pushes events directly to the queue, because it
                // can generate more than one event.
                processSlots();
            }

            bytesRead = read(fileDescriptor, &inputEvent, sizeof(inputEvent));
        }

        if ((bytesRead < 0) && (errno != EAGAIN))
            sf::err() << " Error: " << std::strerror(errno) << std::endl;
    }
    // Finally check if there is a Text event on stdin
    //
    // We only clear the ICANON flag for the time of reading

    newTerminalConfig.c_lflag &= ~static_cast<tcflag_t>(ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newTerminalConfig);

    timeval timeout{};
    timeout.tv_sec  = 0;
    timeout.tv_usec = 0;

    unsigned char code = 0;

    fd_set readFDSet;
    FD_ZERO(&readFDSet);
    FD_SET(STDIN_FILENO, &readFDSet);
    int ready = select(STDIN_FILENO + 1, &readFDSet, nullptr, nullptr, &timeout);

    if (ready > 0 && FD_ISSET(STDIN_FILENO, &readFDSet))
        bytesRead = read(STDIN_FILENO, &code, 1);

    if ((code == 127) || (code == 8)) // Suppress 127 (DEL) to 8 (BACKSPACE)
        code = 0;
    else if (code == 27) // ESC
    {
        // Suppress ANSI escape sequences
        FD_ZERO(&readFDSet);
        FD_SET(STDIN_FILENO, &readFDSet);
        ready = select(STDIN_FILENO + 1, &readFDSet, nullptr, nullptr, &timeout);
        if (ready > 0 && FD_ISSET(STDIN_FILENO, &readFDSet))
        {
            std::array<unsigned char, 16> tempBuffer{};
            bytesRead = read(STDIN_FILENO, tempBuffer.data(), tempBuffer.size());
            code      = 0;
        }
    }

    (void)bytesRead; // Ignore clang-tidy dead store warning

    newTerminalConfig.c_lflag |= ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &newTerminalConfig);

    if (code > 0)
    {
        // TODO: Proper unicode handling
        return sf::Event::TextEntered{code};
    }

    // No events available
    return std::nullopt;
}

// assumes inputMutex is locked
void update()
{
    while (const std::optional event = eventProcess())
        pushEvent(*event);
}
} // namespace

namespace sf::priv::InputImpl
{
////////////////////////////////////////////////////////////
bool isKeyPressed(Keyboard::Key key)
{
    const std::lock_guard lock(inputMutex);
    if ((static_cast<int>(key) < 0) || (static_cast<int>(key) >= static_cast<int>(keyMap.size())))
        return false;

    update();
    return keyMap[key];
}


////////////////////////////////////////////////////////////
bool isKeyPressed(Keyboard::Scancode /* code */)
{
    // TODO: not implemented
    err() << "sf::Keyboard::isKeyPressed(Keyboard::Scancode) is not implemented for DRM." << std::endl;
    return false;
}


////////////////////////////////////////////////////////////
Keyboard::Key localize(Keyboard::Scancode /* code */)
{
    // TODO: not implemented
    err() << "sf::Keyboard::localize is not implemented for DRM." << std::endl;
    return Keyboard::Key::Unknown;
}


////////////////////////////////////////////////////////////
Keyboard::Scancode delocalize(Keyboard::Key /* key */)
{
    // TODO: not implemented
    err() << "sf::Keyboard::delocalize is not implemented for DRM." << std::endl;
    return Keyboard::Scan::Unknown;
}


////////////////////////////////////////////////////////////
String getDescription(Keyboard::Scancode /* code */)
{
    // TODO: not implemented
    err() << "sf::Keyboard::getDescription is not implemented for DRM." << std::endl;
    return "";
}


////////////////////////////////////////////////////////////
void setVirtualKeyboardVisible(bool /*visible*/)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
bool isMouseButtonPressed(Mouse::Button button)
{
    const std::lock_guard lock(inputMutex);
    if ((static_cast<int>(button) < 0) || (static_cast<int>(button) >= static_cast<int>(mouseMap.size())))
        return false;

    update();
    return mouseMap[button];
}


////////////////////////////////////////////////////////////
Vector2i getMousePosition()
{
    const std::lock_guard lock(inputMutex);
    return mousePos;
}


////////////////////////////////////////////////////////////
Vector2i getMousePosition(const WindowBase& /*relativeTo*/)
{
    return getMousePosition();
}


////////////////////////////////////////////////////////////
void setMousePosition(Vector2i position)
{
    const std::lock_guard lock(inputMutex);
    mousePos = position;
}


////////////////////////////////////////////////////////////
void setMousePosition(Vector2i position, const WindowBase& /*relativeTo*/)
{
    setMousePosition(position);
}


////////////////////////////////////////////////////////////
bool isTouchDown(unsigned int finger)
{
    return std::any_of(touchSlots.cbegin(),
                       touchSlots.cend(),
                       [finger](const TouchSlot& slot) { return slot.id == finger; });
}


////////////////////////////////////////////////////////////
Vector2i getTouchPosition(unsigned int finger)
{
    for (const auto& slot : touchSlots)
    {
        if (slot.id == finger)
            return slot.pos;
    }

    return {};
}


////////////////////////////////////////////////////////////
Vector2i getTouchPosition(unsigned int finger, const WindowBase& /*relativeTo*/)
{
    return getTouchPosition(finger);
}


////////////////////////////////////////////////////////////
std::optional<Event> checkEvent()
{
    const std::lock_guard lock(inputMutex);

    if (!eventQueue.empty())
    {
        auto event = std::make_optional(eventQueue.front());
        eventQueue.pop();

        return event;
    }

    if (const std::optional event = eventProcess())
    {
        return event;
    }

    // In the case of multitouch, eventProcess() could have returned false
    // but added events directly to the queue.  (This is ugly, but I'm not
    // sure of a good way to handle generating multiple events at once.)
    if (!eventQueue.empty())
    {
        auto event = std::make_optional(eventQueue.front());
        eventQueue.pop();

        return event;
    }

    return std::nullopt;
}


////////////////////////////////////////////////////////////
void setTerminalConfig()
{
    const std::lock_guard lock(inputMutex);
    initFileDescriptors();

    tcgetattr(STDIN_FILENO, &newTerminalConfig);               // get current terminal config
    oldTerminalConfig = newTerminalConfig;                     // create a backup
    newTerminalConfig.c_lflag &= ~static_cast<tcflag_t>(ECHO); // disable console feedback
    newTerminalConfig.c_lflag &= ~static_cast<tcflag_t>(ISIG); // disable signals
    newTerminalConfig.c_lflag |= ICANON;                       // disable noncanonical mode
    newTerminalConfig.c_iflag |= IGNCR;                        // ignore carriage return
    tcsetattr(STDIN_FILENO, TCSANOW, &newTerminalConfig);      // set our new config
    tcflush(STDIN_FILENO, TCIFLUSH);                           // flush the buffer
}


////////////////////////////////////////////////////////////
void restoreTerminalConfig()
{
    const std::lock_guard lock(inputMutex);
    initFileDescriptors();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldTerminalConfig); // restore terminal config
    tcflush(STDIN_FILENO, TCIFLUSH);                      // flush the buffer
}

} // namespace sf::priv::InputImpl
