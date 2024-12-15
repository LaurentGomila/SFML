////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2016 Andrew Mickelson
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
#include <SFML/Window/DRM/InputImplUDev.hpp>
#include <SFML/Window/DRM/DRMContext.hpp>
#include <SFML/System/Err.hpp>
#include <SFML/System/Lock.hpp>
#include <SFML/System/Mutex.hpp>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <linux/input.h>
#include <queue>
#include <sstream>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <vector>


namespace
{
    struct TouchSlot
    {
        int oldId;
        int id;
        sf::Vector2i pos;

        TouchSlot() :
        oldId(-1),
        id(-1),
        pos(0, 0)
        {
        }
    };

    sf::Mutex         inputMutex;                              // threadsafe? maybe...

    std::vector<int>  fileDescriptors;                         // list of open file descriptors for /dev/input
    std::vector<bool> mouseMap(sf::Mouse::ButtonCount, false); // track whether keys are down
    std::vector<bool> keyMap(sf::Keyboard::KeyCount, false);   // track whether mouse buttons are down

    int touchFd = -1;                                          // file descriptor we have seen MT events on; assumes only 1
    std::vector<TouchSlot> touchSlots;                         // track the state of each touch "slot"
    int currentSlot = 0;                                       // which slot are we currently updating?

    std::queue<sf::Event> eventQueue;                          // events received and waiting to be consumed
    const int MAX_QUEUE = 64;                                  // The maximum size we let eventQueue grow to

    termios newTerminalConfig, oldTerminalConfig;              // Terminal configurations

    bool altDown() { return (keyMap[sf::Keyboard::LAlt] || keyMap[sf::Keyboard::RAlt]); }
    bool controlDown() { return (keyMap[sf::Keyboard::LControl] || keyMap[sf::Keyboard::RControl]); }
    bool shiftDown() { return (keyMap[sf::Keyboard::LShift] || keyMap[sf::Keyboard::RShift]); }
    bool systemDown() { return (keyMap[sf::Keyboard::LSystem] || keyMap[sf::Keyboard::RSystem]); }

    void uninitFileDescriptors(void)
    {
        for (std::vector<int>::iterator itr = fileDescriptors.begin(); itr != fileDescriptors.end(); ++itr)
            close(*itr);
    }

#define BITS_PER_LONG           (sizeof(unsigned long) * 8)
#define NBITS(x)                (((x - 1) / BITS_PER_LONG) + 1)
#define OFF(x)                  (x % BITS_PER_LONG)
#define LONG(x)                 (x / BITS_PER_LONG)
#define TEST_BIT(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)

    // Only keep fileDescriptors that we think are a keyboard, mouse or touchpad/touchscreen
    // Joysticks are handled in /src/SFML/Window/Unix/JoystickImpl.cpp
    bool keepFileDescriptor(int fileDesc)
    {
        unsigned long bitmask_ev[NBITS(EV_MAX)];
        unsigned long bitmask_key[NBITS(KEY_MAX)];
        unsigned long bitmask_abs[NBITS(ABS_MAX)];
        unsigned long bitmask_rel[NBITS(REL_MAX)];

        ioctl(fileDesc, EVIOCGBIT(0, sizeof(bitmask_ev)), &bitmask_ev);
        ioctl(fileDesc, EVIOCGBIT(EV_KEY, sizeof(bitmask_key)), &bitmask_key);
        ioctl(fileDesc, EVIOCGBIT(EV_ABS, sizeof(bitmask_abs)), &bitmask_abs);
        ioctl(fileDesc, EVIOCGBIT(EV_REL, sizeof(bitmask_rel)), &bitmask_rel);

        // This is the keyboard test used by SDL.
        // The first 32 bits are ESC, numbers and Q to D;  If we have any of those,
        // consider it a keyboard device; do not test for KEY_RESERVED, though
        bool is_keyboard = (bitmask_key[0] & 0xFFFFFFFE);

        bool is_abs = TEST_BIT(EV_ABS, bitmask_ev)
            && TEST_BIT(ABS_X, bitmask_abs) && TEST_BIT(ABS_Y, bitmask_abs);

        bool is_rel = TEST_BIT(EV_REL, bitmask_ev)
            && TEST_BIT(REL_X, bitmask_rel) && TEST_BIT(REL_Y, bitmask_rel);

        bool is_mouse = (is_abs || is_rel) && TEST_BIT(BTN_MOUSE, bitmask_key);

        bool is_touch = is_abs && (TEST_BIT(BTN_TOOL_FINGER, bitmask_key) || TEST_BIT(BTN_TOUCH, bitmask_key));
        return is_keyboard || is_mouse || is_touch;
    }

    void initFileDescriptors()
    {
        static bool initialized = false;
        if (initialized)
            return;

        initialized = true;

        for (int i = 0; i < 32; i++)
        {
            std::string name("/dev/input/event");
            std::ostringstream stream;
            stream << i;
            name += stream.str();

            int tempFD = open(name.c_str(), O_RDONLY | O_NONBLOCK);

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

    sf::Mouse::Button toMouseButton(int code)
    {
        switch (code)
        {
            case BTN_LEFT:          return sf::Mouse::Left;
            case BTN_RIGHT:         return sf::Mouse::Right;
            case BTN_MIDDLE:        return sf::Mouse::Middle;
            case BTN_SIDE:          return sf::Mouse::XButton1;
            case BTN_EXTRA:         return sf::Mouse::XButton2;

            default:
                return sf::Mouse::ButtonCount;
        }
    }

    sf::Keyboard::Key toKey(int code)
    {
        switch (code)
        {
            case KEY_ESC:           return sf::Keyboard::Escape;
            case KEY_1:             return sf::Keyboard::Num1;
            case KEY_2:             return sf::Keyboard::Num2;
            case KEY_3:             return sf::Keyboard::Num3;
            case KEY_4:             return sf::Keyboard::Num4;
            case KEY_5:             return sf::Keyboard::Num5;
            case KEY_6:             return sf::Keyboard::Num6;
            case KEY_7:             return sf::Keyboard::Num7;
            case KEY_8:             return sf::Keyboard::Num8;
            case KEY_9:             return sf::Keyboard::Num9;
            case KEY_0:             return sf::Keyboard::Num0;
            case KEY_MINUS:         return sf::Keyboard::Hyphen;
            case KEY_EQUAL:         return sf::Keyboard::Equal;
            case KEY_BACKSPACE:     return sf::Keyboard::Backspace;
            case KEY_TAB:           return sf::Keyboard::Tab;
            case KEY_Q:             return sf::Keyboard::Q;
            case KEY_W:             return sf::Keyboard::W;
            case KEY_E:             return sf::Keyboard::E;
            case KEY_R:             return sf::Keyboard::R;
            case KEY_T:             return sf::Keyboard::T;
            case KEY_Y:             return sf::Keyboard::Y;
            case KEY_U:             return sf::Keyboard::U;
            case KEY_I:             return sf::Keyboard::I;
            case KEY_O:             return sf::Keyboard::O;
            case KEY_P:             return sf::Keyboard::P;
            case KEY_LEFTBRACE:     return sf::Keyboard::LBracket;
            case KEY_RIGHTBRACE:    return sf::Keyboard::RBracket;
            case KEY_KPENTER:
            case KEY_ENTER:         return sf::Keyboard::Enter;
            case KEY_LEFTCTRL:      return sf::Keyboard::LControl;
            case KEY_A:             return sf::Keyboard::A;
            case KEY_S:             return sf::Keyboard::S;
            case KEY_D:             return sf::Keyboard::D;
            case KEY_F:             return sf::Keyboard::F;
            case KEY_G:             return sf::Keyboard::G;
            case KEY_H:             return sf::Keyboard::H;
            case KEY_J:             return sf::Keyboard::J;
            case KEY_K:             return sf::Keyboard::K;
            case KEY_L:             return sf::Keyboard::L;
            case KEY_SEMICOLON:     return sf::Keyboard::Semicolon;
            case KEY_APOSTROPHE:    return sf::Keyboard::Quote;
            case KEY_GRAVE:         return sf::Keyboard::Grave;
            case KEY_LEFTSHIFT:     return sf::Keyboard::LShift;
            case KEY_BACKSLASH:     return sf::Keyboard::Backslash;
            case KEY_Z:             return sf::Keyboard::Z;
            case KEY_X:             return sf::Keyboard::X;
            case KEY_C:             return sf::Keyboard::C;
            case KEY_V:             return sf::Keyboard::V;
            case KEY_B:             return sf::Keyboard::B;
            case KEY_N:             return sf::Keyboard::N;
            case KEY_M:             return sf::Keyboard::M;
            case KEY_COMMA:         return sf::Keyboard::Comma;
            case KEY_DOT:           return sf::Keyboard::Period;
            case KEY_SLASH:         return sf::Keyboard::Slash;
            case KEY_RIGHTSHIFT:    return sf::Keyboard::RShift;
            case KEY_KPASTERISK:    return sf::Keyboard::Multiply;
            case KEY_LEFTALT:       return sf::Keyboard::LAlt;
            case KEY_SPACE:         return sf::Keyboard::Space;
            case KEY_F1:            return sf::Keyboard::F1;
            case KEY_F2:            return sf::Keyboard::F2;
            case KEY_F3:            return sf::Keyboard::F3;
            case KEY_F4:            return sf::Keyboard::F4;
            case KEY_F5:            return sf::Keyboard::F5;
            case KEY_F6:            return sf::Keyboard::F6;
            case KEY_F7:            return sf::Keyboard::F7;
            case KEY_F8:            return sf::Keyboard::F8;
            case KEY_F9:            return sf::Keyboard::F9;
            case KEY_F10:           return sf::Keyboard::F10;
            case KEY_F11:           return sf::Keyboard::F11;
            case KEY_F12:           return sf::Keyboard::F12;
            case KEY_F13:           return sf::Keyboard::F13;
            case KEY_F14:           return sf::Keyboard::F14;
            case KEY_F15:           return sf::Keyboard::F15;
            case KEY_KP7:           return sf::Keyboard::Numpad7;
            case KEY_KP8:           return sf::Keyboard::Numpad8;
            case KEY_KP9:           return sf::Keyboard::Numpad9;
            case KEY_KPMINUS:       return sf::Keyboard::Subtract;
            case KEY_KP4:           return sf::Keyboard::Numpad4;
            case KEY_KP5:           return sf::Keyboard::Numpad5;
            case KEY_KP6:           return sf::Keyboard::Numpad6;
            case KEY_KPPLUS:        return sf::Keyboard::Add;
            case KEY_KP1:           return sf::Keyboard::Numpad1;
            case KEY_KP2:           return sf::Keyboard::Numpad2;
            case KEY_KP3:           return sf::Keyboard::Numpad3;
            case KEY_KP0:           return sf::Keyboard::Numpad0;
            case KEY_KPDOT:         return sf::Keyboard::Delete;
            case KEY_RIGHTCTRL:     return sf::Keyboard::RControl;
            case KEY_KPSLASH:       return sf::Keyboard::Divide;
            case KEY_RIGHTALT:      return sf::Keyboard::RAlt;
            case KEY_HOME:          return sf::Keyboard::Home;
            case KEY_UP:            return sf::Keyboard::Up;
            case KEY_PAGEUP:        return sf::Keyboard::PageUp;
            case KEY_LEFT:          return sf::Keyboard::Left;
            case KEY_RIGHT:         return sf::Keyboard::Right;
            case KEY_END:           return sf::Keyboard::End;
            case KEY_DOWN:          return sf::Keyboard::Down;
            case KEY_PAGEDOWN:      return sf::Keyboard::PageDown;
            case KEY_INSERT:        return sf::Keyboard::Insert;
            case KEY_DELETE:        return sf::Keyboard::Delete;
            case KEY_PAUSE:         return sf::Keyboard::Pause;
            case KEY_LEFTMETA:      return sf::Keyboard::LSystem;
            case KEY_RIGHTMETA:     return sf::Keyboard::RSystem;

            case KEY_RESERVED:
            case KEY_SYSRQ:
            case KEY_CAPSLOCK:
            case KEY_NUMLOCK:
            case KEY_SCROLLLOCK:
            default:
               return sf::Keyboard::Unknown;
        }
    }

    void pushEvent(sf::Event& event)
    {
        if (eventQueue.size() >= MAX_QUEUE)
            eventQueue.pop();

        eventQueue.push(event);
    }

    TouchSlot& atSlot(int idx)
    {
        if (idx >= static_cast<int>(touchSlots.size()))
            touchSlots.resize(static_cast<size_t>(idx + 1));
        return touchSlots.at(static_cast<size_t>(idx));
    }

    void processSlots()
    {
        for (std::vector<TouchSlot>::iterator slot = touchSlots.begin(); slot != touchSlots.end(); ++slot)
        {
            sf::Event event;

            event.touch.x = slot->pos.x;
            event.touch.y = slot->pos.y;

            if (slot->oldId == slot->id)
            {
                event.type = sf::Event::TouchMoved;
                event.touch.finger = static_cast<unsigned int>(slot->id);
                pushEvent(event);
            }
            else
            {
                if (slot->oldId != -1)
                {
                    event.type = sf::Event::TouchEnded;
                    event.touch.finger = static_cast<unsigned int>(slot->oldId);
                    pushEvent(event);
                }
                if (slot->id != -1)
                {
                    event.type = sf::Event::TouchBegan;
                    event.touch.finger = static_cast<unsigned int>(slot->id);
                    pushEvent(event);
                }

                slot->oldId = slot->id;
            }
        }
    }

    bool eventProcess(sf::Event& event)
    {
        sf::Lock lock(inputMutex);

        // Ensure that we are initialized
        initFileDescriptors();

        // This is for handling the Backspace and DEL text events, which we
        // generate based on keystrokes (and not stdin)
        static unsigned int doDeferredText = 0;
        if (doDeferredText)
        {
            event.type = sf::Event::TextEntered;
            event.text.unicode = doDeferredText;
            doDeferredText = 0;
            return true;
        }

        ssize_t bytesRead;
        sf::Vector2i mousePos = sf::priv::DRMContext::getCursorPos();

        // Check all the open file descriptors for the next event
        for (std::vector<int>::iterator itr = fileDescriptors.begin(); itr != fileDescriptors.end(); ++itr)
        {
            input_event inputEvent;
            bytesRead = read(*itr, &inputEvent, sizeof(inputEvent));

            while (bytesRead > 0)
            {
                if (inputEvent.type == EV_KEY)
                {
                    sf::Mouse::Button mb = toMouseButton(inputEvent.code);
                    if (mb != sf::Mouse::ButtonCount)
                    {
                        event.type = inputEvent.value ? sf::Event::MouseButtonPressed : sf::Event::MouseButtonReleased;
                        event.mouseButton.button = mb;
                        event.mouseButton.x = mousePos.x;
                        event.mouseButton.y = mousePos.y;

                        mouseMap[mb] = inputEvent.value;
                        return true;
                    }
                    else
                    {
                        sf::Keyboard::Key kb = toKey(inputEvent.code);

                        unsigned int special = 0;
                        if ((kb == sf::Keyboard::Delete)
                                || (kb == sf::Keyboard::Backspace))
                            special = (kb == sf::Keyboard::Delete) ? 127 : 8;

                        if (inputEvent.value == 2)
                        {
                            // key repeat events
                            //
                            if (special)
                            {
                                event.type = sf::Event::TextEntered;
                                event.text.unicode = special;
                                return true;
                            }
                        }
                        else if (kb != sf::Keyboard::Unknown)
                        {
                            // key down and key up events
                            //
                            event.type = inputEvent.value ? sf::Event::KeyPressed : sf::Event::KeyReleased;
                            event.key.code = kb;
                            event.key.scancode = sf::Keyboard::Scan::Unknown; // TODO: not implemented
                            event.key.alt = altDown();
                            event.key.control = controlDown();
                            event.key.shift = shiftDown();
                            event.key.system = systemDown();

                            keyMap[static_cast<std::size_t>(kb)] = inputEvent.value;

                            if (special && inputEvent.value)
                                doDeferredText = special;

                            return true;
                        }
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
                        event.type = sf::Event::MouseWheelScrolled;
                        event.mouseWheelScroll.delta = static_cast<float>(inputEvent.value);
                        event.mouseWheelScroll.x = mousePos.x;
                        event.mouseWheelScroll.y = mousePos.y;
                        return true;
                    }

                    if (posChange)
                    {
                        event.type = sf::Event::MouseMoved;
                        sf::priv::DRMContext::setCursorPos( mousePos );
                        mousePos = sf::priv::DRMContext::getCursorPos();
                        event.mouseMove.x = mousePos.x;
                        event.mouseMove.y = mousePos.y;
                        return true;
                    }
                }
                else if (inputEvent.type == EV_ABS)
                {
                    switch (inputEvent.code)
                    {
                    case ABS_MT_SLOT:
                        currentSlot = inputEvent.value;
                        touchFd = *itr;
                        break;
                    case ABS_MT_TRACKING_ID:
                        atSlot(currentSlot).id = inputEvent.value;
                        touchFd = *itr;
                        break;
                    case ABS_MT_POSITION_X:
                        atSlot(currentSlot).pos.x = inputEvent.value;
                        touchFd = *itr;
                        break;
                    case ABS_MT_POSITION_Y:
                        atSlot(currentSlot).pos.y = inputEvent.value;
                        touchFd = *itr;
                        break;
                    }
                }
                else if (inputEvent.type == EV_SYN && inputEvent.code == SYN_REPORT &&
                          *itr == touchFd)
                {
                    // This pushes events directly to the queue, because it
                    // can generate more than one event.
                    processSlots();
                }

                bytesRead = read(*itr, &inputEvent, sizeof(inputEvent));
            }

            if ((bytesRead < 0) && (errno != EAGAIN))
                sf::err() << " Error: " << std::strerror(errno) << std::endl;
        }

        // Finally check if there is a Text event on stdin
        //
        // We only clear the ICANON flag for the time of reading

        newTerminalConfig.c_lflag &= ~static_cast<tcflag_t>(ICANON);
        tcsetattr(STDIN_FILENO, TCSANOW, &newTerminalConfig);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        unsigned char code = 0;

        fd_set readFDSet;
        FD_ZERO(&readFDSet);
        FD_SET(STDIN_FILENO, &readFDSet);
        int ready = select(STDIN_FILENO + 1, &readFDSet, NULL, NULL, &timeout);

        if (ready > 0 && FD_ISSET(STDIN_FILENO, &readFDSet))
            bytesRead = read(STDIN_FILENO, &code, 1);

        if ((code == 127) || (code == 8))  // Suppress 127 (DEL) to 8 (BACKSPACE)
            code = 0;
        else if (code == 27)  // ESC
        {
            // Suppress ANSI escape sequences
            FD_ZERO(&readFDSet);
            FD_SET(STDIN_FILENO, &readFDSet);
            ready = select(STDIN_FILENO + 1, &readFDSet, NULL, NULL, &timeout);
            if (ready > 0 && FD_ISSET(STDIN_FILENO, &readFDSet))
            {
                unsigned char tempBuffer[16];
                bytesRead = read(STDIN_FILENO, tempBuffer, 16);
                code = 0;
            }
        }

        newTerminalConfig.c_lflag |= ICANON;
        tcsetattr(STDIN_FILENO, TCSANOW, &newTerminalConfig);

        if (code > 0)
        {
            // TODO: Proper unicode handling
            event.type = sf::Event::TextEntered;
            event.text.unicode = code;
            return true;
        }

        // No events available
        return false;
    }

    // assumes inputMutex is locked
    void update()
    {
        sf::Event event;
        while (eventProcess(event))
        {
            pushEvent(event);
        }
    }
}

namespace sf
{
namespace priv
{
////////////////////////////////////////////////////////////
bool InputImpl::isKeyPressed(Keyboard::Key key)
{
    Lock lock(inputMutex);
    if ((key < 0) || (key >= static_cast<int>(keyMap.size())))
        return false;

    update();
    return keyMap[static_cast<std::size_t>(key)];
}

////////////////////////////////////////////////////////////
bool InputImpl::isKeyPressed(Keyboard::Scancode /* code */)
{
    // TODO: not implemented
    err() << "sf::Keyboard::isKeyPressed(Keyboard::Scancode) is not implemented for DRM." << std::endl;
    return false;
}


////////////////////////////////////////////////////////////
Keyboard::Key InputImpl::localize(Keyboard::Scancode /* code */)
{
    // TODO: not implemented
    err() << "sf::Keyboard::localize is not implemented for DRM." << std::endl;
    return Keyboard::Unknown;
}


////////////////////////////////////////////////////////////
Keyboard::Scancode InputImpl::delocalize(Keyboard::Key /* key */)
{
    // TODO: not implemented
    err() << "sf::Keyboard::delocalize is not implemented for DRM." << std::endl;
    return Keyboard::Scan::Unknown;
}


////////////////////////////////////////////////////////////
String InputImpl::getDescription(Keyboard::Scancode /* code */)
{
    // TODO: not implemented
    err() << "sf::Keyboard::getDescription is not implemented for DRM." << std::endl;
    return "";
}


////////////////////////////////////////////////////////////
void InputImpl::setVirtualKeyboardVisible(bool /*visible*/)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
bool InputImpl::isMouseButtonPressed(Mouse::Button button)
{
    Lock lock(inputMutex);
    if ((button < 0) || (button >= static_cast<int>(mouseMap.size())))
        return false;

    update();
    return mouseMap[button];
}


////////////////////////////////////////////////////////////
Vector2i InputImpl::getMousePosition()
{
    Lock lock(inputMutex);
    return sf::priv::DRMContext::getCursorPos();
}


////////////////////////////////////////////////////////////
Vector2i InputImpl::getMousePosition(const WindowBase& /*relativeTo*/)
{
    return getMousePosition();
}


////////////////////////////////////////////////////////////
void InputImpl::setMousePosition(const Vector2i& position)
{
    Lock lock(inputMutex);
    sf::priv::DRMContext::setCursorPos( position );
}


////////////////////////////////////////////////////////////
void InputImpl::setMousePosition(const Vector2i& position, const WindowBase& /*relativeTo*/)
{
    setMousePosition(position);
}


////////////////////////////////////////////////////////////
bool InputImpl::isTouchDown(unsigned int finger)
{
    for (std::vector<TouchSlot>::iterator slot = touchSlots.begin(); slot != touchSlots.end(); ++slot)
    {
        if (slot->id == static_cast<int>(finger))
            return true;
    }

    return false;
}


////////////////////////////////////////////////////////////
Vector2i InputImpl::getTouchPosition(unsigned int finger)
{
    for (std::vector<TouchSlot>::iterator slot = touchSlots.begin(); slot != touchSlots.end(); ++slot)
    {
        if (slot->id == static_cast<int>(finger))
            return slot->pos;
    }

    return Vector2i();
}


////////////////////////////////////////////////////////////
Vector2i InputImpl::getTouchPosition(unsigned int finger, const WindowBase& /*relativeTo*/)
{
    return getTouchPosition(finger);
}


////////////////////////////////////////////////////////////
bool InputImpl::checkEvent(sf::Event& event)
{
    Lock lock(inputMutex);
    if (!eventQueue.empty())
    {
        event = eventQueue.front();
        eventQueue.pop();

        return true;
    }

    if (eventProcess(event))
    {
        return true;
    }
    else
    {
        // In the case of multitouch, eventProcess() could have returned false
        // but added events directly to the queue.  (This is ugly, but I'm not
        // sure of a good way to handle generating multiple events at once.)
        if (!eventQueue.empty())
        {
            event = eventQueue.front();
            eventQueue.pop();

            return true;
        }
    }

    return false;
}


////////////////////////////////////////////////////////////
void InputImpl::setTerminalConfig()
{
    sf::Lock lock(inputMutex);
    initFileDescriptors();

    tcgetattr(STDIN_FILENO, &newTerminalConfig);          // get current terminal config
    oldTerminalConfig = newTerminalConfig;                // create a backup
    newTerminalConfig.c_lflag &= ~static_cast<tcflag_t>(ECHO);         // disable console feedback
    newTerminalConfig.c_lflag &= ~static_cast<tcflag_t>(ISIG);         // disable signals
    newTerminalConfig.c_lflag |= ICANON;                  // disable noncanonical mode
    newTerminalConfig.c_iflag |= IGNCR;                   // ignore carriage return
    tcsetattr(STDIN_FILENO, TCSANOW, &newTerminalConfig); // set our new config
    tcflush(STDIN_FILENO, TCIFLUSH);                      // flush the buffer
}


////////////////////////////////////////////////////////////
void InputImpl::restoreTerminalConfig()
{
    sf::Lock lock(inputMutex);
    initFileDescriptors();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldTerminalConfig); // restore terminal config
    tcflush(STDIN_FILENO, TCIFLUSH);                      // flush the buffer
}

} // namespace priv

} // namespace sf
