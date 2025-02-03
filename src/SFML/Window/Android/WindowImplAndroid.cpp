////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2013 Jonathan De Wachter (dewachter.jonathan@gmail.com)
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
#include <SFML/Window/Android/WindowImplAndroid.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/WindowEnums.hpp>

#include <SFML/System/Err.hpp>

#include <android/looper.h>

#include <mutex>
#include <ostream>

// Define missing constants for older API levels
#if __ANDROID_API__ < 13
#define AMOTION_EVENT_ACTION_HOVER_MOVE 0x00000007
#define AMOTION_EVENT_ACTION_SCROLL     0x00000008
#endif

////////////////////////////////////////////////////////////
// Private data
////////////////////////////////////////////////////////////
namespace sf::priv
{
WindowImplAndroid* WindowImplAndroid::singleInstance = nullptr;

////////////////////////////////////////////////////////////
WindowImplAndroid::WindowImplAndroid(WindowHandle /* handle */)
{
}


////////////////////////////////////////////////////////////
WindowImplAndroid::WindowImplAndroid(VideoMode mode,
                                     const String& /* title */,
                                     std::uint32_t /* style */,
                                     State state,
                                     const ContextSettings& /* settings */) :
m_size(mode.size)
{
    ActivityStates&       states = getActivity();
    const std::lock_guard lock(states.mutex);

    if (state == State::Fullscreen)
        states.fullscreen = true;

    WindowImplAndroid::singleInstance = this;
    states.forwardEvent               = forwardEvent;

    // Register process event callback
    states.processEvent = processEvent;

    states.initialized = true;
}


////////////////////////////////////////////////////////////
WindowImplAndroid::~WindowImplAndroid()
{
    WindowImplAndroid::singleInstance = nullptr;
}


////////////////////////////////////////////////////////////
WindowHandle WindowImplAndroid::getNativeHandle() const
{
    ActivityStates&       states = getActivity();
    const std::lock_guard lock(states.mutex);

    return states.window;
}


////////////////////////////////////////////////////////////
void WindowImplAndroid::processEvents()
{
    // Process incoming OS events
    while (ALooper_pollOnce(0, nullptr, nullptr, nullptr) >= 0)
        ;

    ActivityStates&       states = getActivity();
    const std::lock_guard lock(states.mutex);

    if (m_windowBeingCreated)
    {
        states.context->createSurface(states.window);
        m_windowBeingCreated = false;
    }

    if (m_windowBeingDestroyed)
    {
        states.context->destroySurface();
        m_windowBeingDestroyed = false;
    }

    states.updated = true;
}


////////////////////////////////////////////////////////////
Vector2i WindowImplAndroid::getPosition() const
{
    // Not applicable
    return {};
}


////////////////////////////////////////////////////////////
void WindowImplAndroid::setPosition(Vector2i /* position */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
Vector2u WindowImplAndroid::getSize() const
{
    return m_size;
}


////////////////////////////////////////////////////////////
void WindowImplAndroid::setSize(Vector2u /* size */)
{
}


////////////////////////////////////////////////////////////
void WindowImplAndroid::setMinimumSize(const std::optional<Vector2u>& /* minimumSize */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplAndroid::setMaximumSize(const std::optional<Vector2u>& /* maximumSize */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplAndroid::setTitle(const String& /* title */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplAndroid::setIcon(Vector2u /* size */, const std::uint8_t* /* pixels */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplAndroid::setVisible(bool /* visible */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplAndroid::setMouseCursorVisible(bool /* visible */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplAndroid::setMouseCursorGrabbed(bool /* grabbed */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplAndroid::setMouseCursor(const CursorImpl& /* cursor */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplAndroid::setKeyRepeatEnabled(bool /* enabled */)
{
    // Not applicable
}


////////////////////////////////////////////////////////////
void WindowImplAndroid::requestFocus()
{
    // Not applicable
}


////////////////////////////////////////////////////////////
bool WindowImplAndroid::hasFocus() const
{
    return m_hasFocus;
}


////////////////////////////////////////////////////////////
void WindowImplAndroid::forwardEvent(const Event& event)
{
    if (WindowImplAndroid::singleInstance != nullptr)
    {
        const ActivityStates& states = getActivity();

        if (event.is<Event::FocusGained>())
        {
            WindowImplAndroid::singleInstance->m_size = Vector2u(
                Vector2i(ANativeWindow_getWidth(states.window), ANativeWindow_getHeight(states.window)));
            WindowImplAndroid::singleInstance->m_windowBeingCreated = true;
            WindowImplAndroid::singleInstance->m_hasFocus           = true;
        }
        else if (event.is<Event::FocusLost>())
        {
            WindowImplAndroid::singleInstance->m_windowBeingDestroyed = true;
            WindowImplAndroid::singleInstance->m_hasFocus             = false;
        }

        WindowImplAndroid::singleInstance->pushEvent(event);
    }
}


////////////////////////////////////////////////////////////
int WindowImplAndroid::processEvent(int /* fd */, int /* events */, void* /* data */)
{
    ActivityStates&       states = getActivity();
    const std::lock_guard lock(states.mutex);

    AInputEvent* inputEvent = nullptr;

    if (AInputQueue_getEvent(states.inputQueue, &inputEvent) >= 0)
    {
        if (AInputQueue_preDispatchEvent(states.inputQueue, inputEvent))
            return 1;

        int handled = 0;

        const std::int32_t type = AInputEvent_getType(inputEvent);

        if (type == AINPUT_EVENT_TYPE_KEY)
        {
            const std::int32_t action = AKeyEvent_getAction(inputEvent);
            const std::int32_t key    = AKeyEvent_getKeyCode(inputEvent);

            if ((action == AKEY_EVENT_ACTION_DOWN || action == AKEY_EVENT_ACTION_UP || action == AKEY_EVENT_ACTION_MULTIPLE) &&
                key != AKEYCODE_VOLUME_UP && key != AKEYCODE_VOLUME_DOWN)
            {
                handled = processKeyEvent(inputEvent, states);
            }
        }
        else if (type == AINPUT_EVENT_TYPE_MOTION)
        {
            const std::int32_t action = AMotionEvent_getAction(inputEvent);

            switch (action & AMOTION_EVENT_ACTION_MASK)
            {
                case AMOTION_EVENT_ACTION_SCROLL:
                {
                    handled = processScrollEvent(inputEvent, states);
                    break;
                }

                // TODO: should hover_move indeed trigger the event?
                // case AMOTION_EVENT_ACTION_HOVER_MOVE:
                case AMOTION_EVENT_ACTION_MOVE:
                {
                    handled = processMotionEvent(inputEvent, states);
                    break;
                }

                // TODO: investigate AMOTION_EVENT_OUTSIDE
                case AMOTION_EVENT_ACTION_POINTER_DOWN:
                case AMOTION_EVENT_ACTION_DOWN:
                {
                    handled = processPointerEvent(true, inputEvent, states);
                    break;
                }

                case AMOTION_EVENT_ACTION_POINTER_UP:
                case AMOTION_EVENT_ACTION_UP:
                case AMOTION_EVENT_ACTION_CANCEL:
                {
                    handled = processPointerEvent(false, inputEvent, states);
                    break;
                }
            }
        }

        AInputQueue_finishEvent(states.inputQueue, inputEvent, handled);
    }

    return 1;
}


////////////////////////////////////////////////////////////
int WindowImplAndroid::processScrollEvent(AInputEvent* inputEvent, ActivityStates& states)
{
    // Prepare the Java virtual machine
    jint lResult = 0;

    JavaVM* lJavaVM = states.activity->vm;
    JNIEnv* lJNIEnv = states.activity->env;

    JavaVMAttachArgs lJavaVMAttachArgs;
    lJavaVMAttachArgs.version = JNI_VERSION_1_6;
    lJavaVMAttachArgs.name    = "NativeThread";
    lJavaVMAttachArgs.group   = nullptr;

    lResult = lJavaVM->AttachCurrentThread(&lJNIEnv, &lJavaVMAttachArgs);

    if (lResult == JNI_ERR)
    {
        err() << "Failed to initialize JNI, couldn't get the Unicode value" << std::endl;
        return 0;
    }

    // Retrieve everything we need to create this MotionEvent in Java
    const std::int64_t downTime   = AMotionEvent_getDownTime(inputEvent);
    const std::int64_t eventTime  = AMotionEvent_getEventTime(inputEvent);
    const std::int32_t action     = AMotionEvent_getAction(inputEvent);
    const float        x          = AMotionEvent_getX(inputEvent, 0);
    const float        y          = AMotionEvent_getY(inputEvent, 0);
    const float        pressure   = AMotionEvent_getPressure(inputEvent, 0);
    const float        size       = AMotionEvent_getSize(inputEvent, 0);
    const std::int32_t metaState  = AMotionEvent_getMetaState(inputEvent);
    const float        xPrecision = AMotionEvent_getXPrecision(inputEvent);
    const float        yPrecision = AMotionEvent_getYPrecision(inputEvent);
    const std::int32_t deviceId   = AInputEvent_getDeviceId(inputEvent);
    const std::int32_t edgeFlags  = AMotionEvent_getEdgeFlags(inputEvent);

    // Create the MotionEvent object in Java through its static constructor obtain()
    jclass    classMotionEvent   = lJNIEnv->FindClass("android/view/MotionEvent");
    jmethodID staticMethodObtain = lJNIEnv->GetStaticMethodID(classMotionEvent,
                                                              "obtain",
                                                              "(JJIFFFFIFFII)Landroid/view/MotionEvent;");
    // Note: C standard compatibility, varargs
    //       automatically promote floats to doubles
    //       even though the function signature declares float
    jobject objectMotionEvent = lJNIEnv->CallStaticObjectMethod(classMotionEvent,
                                                                staticMethodObtain,
                                                                downTime,
                                                                eventTime,
                                                                action,
                                                                static_cast<double>(x),
                                                                static_cast<double>(y),
                                                                static_cast<double>(pressure),
                                                                static_cast<double>(size),
                                                                metaState,
                                                                static_cast<double>(xPrecision),
                                                                static_cast<double>(yPrecision),
                                                                deviceId,
                                                                edgeFlags);

    // Call its getAxisValue() method to get the delta value of our wheel move event
    jmethodID    methodGetAxisValue = lJNIEnv->GetMethodID(classMotionEvent, "getAxisValue", "(I)F");
    const jfloat delta              = lJNIEnv->CallFloatMethod(objectMotionEvent, methodGetAxisValue, 0x00000001);

    lJNIEnv->DeleteLocalRef(classMotionEvent);
    lJNIEnv->DeleteLocalRef(objectMotionEvent);

    // Create and send our mouse wheel event
    Event::MouseWheelScrolled event;
    event.wheel    = Mouse::Wheel::Vertical;
    event.delta    = static_cast<float>(delta);
    event.position = Vector2i(Vector2(AMotionEvent_getX(inputEvent, 0), AMotionEvent_getY(inputEvent, 0)));
    forwardEvent(event);

    // Detach this thread from the JVM
    lJavaVM->DetachCurrentThread();

    return 1;
}


////////////////////////////////////////////////////////////
int WindowImplAndroid::processKeyEvent(AInputEvent* inputEvent, ActivityStates& /* states */)
{
    const std::int32_t action = AKeyEvent_getAction(inputEvent);

    const std::int32_t key     = AKeyEvent_getKeyCode(inputEvent);
    const std::int32_t metakey = AKeyEvent_getMetaState(inputEvent);

    const auto forwardKeyEvent = [&](auto keyEvent)
    {
        keyEvent.code       = androidKeyToSF(key);
        keyEvent.alt        = metakey & AMETA_ALT_ON;
        keyEvent.shift      = metakey & AMETA_SHIFT_ON;
        keyEvent.system     = metakey & AMETA_SYM_ON;
        keyEvent.capsLock   = metakey & AMETA_CAPS_LOCK_ON;
        keyEvent.numLock    = metakey & AMETA_NUM_LOCK_ON;
        keyEvent.scrollLock = metakey & AMETA_SCROLL_LOCK_ON;
        forwardEvent(keyEvent);
    };

    switch (action)
    {
        case AKEY_EVENT_ACTION_DOWN:
            forwardKeyEvent(Event::KeyPressed{});
            return 1;
        case AKEY_EVENT_ACTION_UP:
            forwardKeyEvent(Event::KeyReleased{});

            if (const auto unicode = getUnicode(inputEvent))
                forwardEvent(Event::TextEntered{unicode});
            return 1;
        case AKEY_EVENT_ACTION_MULTIPLE:
            // Since complex inputs don't get separate key down/up events
            // both have to be faked at once
            forwardKeyEvent(Event::KeyPressed{});
            forwardKeyEvent(Event::KeyReleased{});

            // This requires some special treatment, since this might represent
            // a repetition of key presses or a complete sequence
            if (key == AKEYCODE_UNKNOWN)
            {
                // This is a unique sequence, which is not yet exposed in the NDK
                // https://code.google.com/p/android/issues/detail?id=33998
                return 0;
            }
            if (const auto unicode = getUnicode(inputEvent)) // This is a repeated sequence
            {
                const Event event(Event::TextEntered{unicode});

                const std::int32_t repeats = AKeyEvent_getRepeatCount(inputEvent);
                for (std::int32_t i = 0; i < repeats; ++i)
                    forwardEvent(event);
                return 1;
            }
            break;
    }
    return 0;
}


////////////////////////////////////////////////////////////
int WindowImplAndroid::processMotionEvent(AInputEvent* inputEvent, ActivityStates& states)
{
    const std::int32_t device       = AInputEvent_getSource(inputEvent);
    const std::size_t  pointerCount = AMotionEvent_getPointerCount(inputEvent);

    for (std::size_t p = 0; p < pointerCount; ++p)
    {
        const std::int32_t id = AMotionEvent_getPointerId(inputEvent, p);

        const int x = static_cast<int>(AMotionEvent_getX(inputEvent, p));
        const int y = static_cast<int>(AMotionEvent_getY(inputEvent, p));

        if (device == AINPUT_SOURCE_MOUSE)
        {
            const Event::MouseMoved mouseMoved{{x, y}};
            forwardEvent(mouseMoved);

            states.mousePosition = mouseMoved.position;
        }
        else if (static_cast<std::uint32_t>(device) & AINPUT_SOURCE_TOUCHSCREEN)
        {
            if (states.touchEvents[id].x == x && states.touchEvents[id].y == y)
                continue;

            const Event::TouchMoved touchMoved{static_cast<unsigned int>(id), {x, y}};
            forwardEvent(touchMoved);

            states.touchEvents[id] = touchMoved.position;
        }
    }

    return 1;
}


////////////////////////////////////////////////////////////
int WindowImplAndroid::processPointerEvent(bool isDown, AInputEvent* inputEvent, ActivityStates& states)
{
    const std::int32_t device = AInputEvent_getSource(inputEvent);
    const std::int32_t action = AMotionEvent_getAction(inputEvent);

    const std::size_t index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    const std::int32_t id     = AMotionEvent_getPointerId(inputEvent, index);
    const auto         button = static_cast<Mouse::Button>(id);

    int x = static_cast<int>(AMotionEvent_getX(inputEvent, index));
    int y = static_cast<int>(AMotionEvent_getY(inputEvent, index));

    if (isDown)
    {
        if (device == AINPUT_SOURCE_MOUSE)
        {
            if (id >= 0 && id < static_cast<int>(Mouse::ButtonCount))
                states.isButtonPressed[button] = true;

            forwardEvent(Event::MouseButtonPressed{button, {x, y}});
        }
        else if (static_cast<unsigned int>(device) & AINPUT_SOURCE_TOUCHSCREEN)
        {
            Event::TouchBegan touchBegan;
            touchBegan.finger   = static_cast<unsigned int>(id);
            touchBegan.position = {x, y};

            states.touchEvents[id] = touchBegan.position;

            forwardEvent(touchBegan);
        }
    }
    else
    {
        if (device == AINPUT_SOURCE_MOUSE)
        {
            if (id >= 0 && id < static_cast<int>(Mouse::ButtonCount))
                states.isButtonPressed[button] = false;

            forwardEvent(Event::MouseButtonReleased{button, {x, y}});
        }
        else if (static_cast<std::uint32_t>(device) & AINPUT_SOURCE_TOUCHSCREEN)
        {
            states.touchEvents.erase(id);
            forwardEvent(Event::TouchEnded{static_cast<unsigned int>(id), {x, y}});
        }
    }

    return 1;
}


////////////////////////////////////////////////////////////
Keyboard::Key WindowImplAndroid::androidKeyToSF(std::int32_t key)
{
    // clang-format off
    switch (key)
    {
        case AKEYCODE_UNKNOWN:
        case AKEYCODE_SOFT_LEFT:
        case AKEYCODE_SOFT_RIGHT:
        case AKEYCODE_HOME:               return Keyboard::Key::Unknown;
        case AKEYCODE_BACK:               return Keyboard::Key::Escape;
        case AKEYCODE_CALL:
        case AKEYCODE_ENDCALL:            return Keyboard::Key::Unknown;
        case AKEYCODE_0:                  return Keyboard::Key::Num0;
        case AKEYCODE_1:                  return Keyboard::Key::Num1;
        case AKEYCODE_2:                  return Keyboard::Key::Num2;
        case AKEYCODE_3:                  return Keyboard::Key::Num3;
        case AKEYCODE_4:                  return Keyboard::Key::Num4;
        case AKEYCODE_5:                  return Keyboard::Key::Num5;
        case AKEYCODE_6:                  return Keyboard::Key::Num6;
        case AKEYCODE_7:                  return Keyboard::Key::Num7;
        case AKEYCODE_8:                  return Keyboard::Key::Num8;
        case AKEYCODE_9:                  return Keyboard::Key::Num9;
        case AKEYCODE_STAR:
        case AKEYCODE_POUND:
        case AKEYCODE_DPAD_UP:
        case AKEYCODE_DPAD_DOWN:
        case AKEYCODE_DPAD_LEFT:
        case AKEYCODE_DPAD_RIGHT:
        case AKEYCODE_DPAD_CENTER:
        case AKEYCODE_VOLUME_UP:
        case AKEYCODE_VOLUME_DOWN:
        case AKEYCODE_POWER:
        case AKEYCODE_CAMERA:
        case AKEYCODE_CLEAR:              return Keyboard::Key::Unknown;
        case AKEYCODE_A:                  return Keyboard::Key::A;
        case AKEYCODE_B:                  return Keyboard::Key::B;
        case AKEYCODE_C:                  return Keyboard::Key::C;
        case AKEYCODE_D:                  return Keyboard::Key::D;
        case AKEYCODE_E:                  return Keyboard::Key::E;
        case AKEYCODE_F:                  return Keyboard::Key::F;
        case AKEYCODE_G:                  return Keyboard::Key::G;
        case AKEYCODE_H:                  return Keyboard::Key::H;
        case AKEYCODE_I:                  return Keyboard::Key::I;
        case AKEYCODE_J:                  return Keyboard::Key::J;
        case AKEYCODE_K:                  return Keyboard::Key::K;
        case AKEYCODE_L:                  return Keyboard::Key::L;
        case AKEYCODE_M:                  return Keyboard::Key::M;
        case AKEYCODE_N:                  return Keyboard::Key::N;
        case AKEYCODE_O:                  return Keyboard::Key::O;
        case AKEYCODE_P:                  return Keyboard::Key::P;
        case AKEYCODE_Q:                  return Keyboard::Key::Q;
        case AKEYCODE_R:                  return Keyboard::Key::R;
        case AKEYCODE_S:                  return Keyboard::Key::S;
        case AKEYCODE_T:                  return Keyboard::Key::T;
        case AKEYCODE_U:                  return Keyboard::Key::U;
        case AKEYCODE_V:                  return Keyboard::Key::V;
        case AKEYCODE_W:                  return Keyboard::Key::W;
        case AKEYCODE_X:                  return Keyboard::Key::X;
        case AKEYCODE_Y:                  return Keyboard::Key::Y;
        case AKEYCODE_Z:                  return Keyboard::Key::Z;
        case AKEYCODE_COMMA:              return Keyboard::Key::Comma;
        case AKEYCODE_PERIOD:             return Keyboard::Key::Period;
        case AKEYCODE_ALT_LEFT:           return Keyboard::Key::LAlt;
        case AKEYCODE_ALT_RIGHT:          return Keyboard::Key::RAlt;
        case AKEYCODE_SHIFT_LEFT:         return Keyboard::Key::LShift;
        case AKEYCODE_SHIFT_RIGHT:        return Keyboard::Key::RShift;
        case AKEYCODE_TAB:                return Keyboard::Key::Tab;
        case AKEYCODE_SPACE:              return Keyboard::Key::Space;
        case AKEYCODE_SYM:
        case AKEYCODE_EXPLORER:
        case AKEYCODE_ENVELOPE:           return Keyboard::Key::Unknown;
        case AKEYCODE_ENTER:              return Keyboard::Key::Enter;
        case AKEYCODE_DEL:                return Keyboard::Key::Backspace;
        case AKEYCODE_FORWARD_DEL:        return Keyboard::Key::Delete;
        case AKEYCODE_GRAVE:              return Keyboard::Key::Grave;
        case AKEYCODE_MINUS:              return Keyboard::Key::Subtract;
        case AKEYCODE_EQUALS:             return Keyboard::Key::Equal;
        case AKEYCODE_LEFT_BRACKET:       return Keyboard::Key::LBracket;
        case AKEYCODE_RIGHT_BRACKET:      return Keyboard::Key::RBracket;
        case AKEYCODE_BACKSLASH:          return Keyboard::Key::Backslash;
        case AKEYCODE_SEMICOLON:          return Keyboard::Key::Semicolon;
        case AKEYCODE_APOSTROPHE:         return Keyboard::Key::Apostrophe;
        case AKEYCODE_SLASH:              return Keyboard::Key::Slash;
        case AKEYCODE_AT:
        case AKEYCODE_NUM:
        case AKEYCODE_HEADSETHOOK:
        case AKEYCODE_FOCUS: // *Camera* focus
        case AKEYCODE_PLUS:
        case AKEYCODE_MENU:
        case AKEYCODE_NOTIFICATION:
        case AKEYCODE_SEARCH:
        case AKEYCODE_MEDIA_PLAY_PAUSE:
        case AKEYCODE_MEDIA_STOP:
        case AKEYCODE_MEDIA_NEXT:
        case AKEYCODE_MEDIA_PREVIOUS:
        case AKEYCODE_MEDIA_REWIND:
        case AKEYCODE_MEDIA_FAST_FORWARD:
        case AKEYCODE_MUTE:               return Keyboard::Key::Unknown;
        case AKEYCODE_PAGE_UP:            return Keyboard::Key::PageUp;
        case AKEYCODE_PAGE_DOWN:          return Keyboard::Key::PageDown;
        case AKEYCODE_PICTSYMBOLS:
        case AKEYCODE_SWITCH_CHARSET:
        case AKEYCODE_BUTTON_A:
        case AKEYCODE_BUTTON_B:
        case AKEYCODE_BUTTON_C:
        case AKEYCODE_BUTTON_X:
        case AKEYCODE_BUTTON_Y:
        case AKEYCODE_BUTTON_Z:
        case AKEYCODE_BUTTON_L1:
        case AKEYCODE_BUTTON_R1:
        case AKEYCODE_BUTTON_L2:
        case AKEYCODE_BUTTON_R2:
        case AKEYCODE_BUTTON_THUMBL:
        case AKEYCODE_BUTTON_THUMBR:
        case AKEYCODE_BUTTON_START:
        case AKEYCODE_BUTTON_SELECT:
        case AKEYCODE_BUTTON_MODE:
        default:                          return Keyboard::Key::Unknown;
    }
    // clang-format on
}


////////////////////////////////////////////////////////////
char32_t WindowImplAndroid::getUnicode(AInputEvent* event)
{
    // Retrieve activity states
    ActivityStates&       states = getActivity();
    const std::lock_guard lock(states.mutex);

    // Initializes JNI
    jint lResult = 0;

    JavaVM* lJavaVM = states.activity->vm;
    JNIEnv* lJNIEnv = states.activity->env;

    JavaVMAttachArgs lJavaVMAttachArgs;
    lJavaVMAttachArgs.version = JNI_VERSION_1_6;
    lJavaVMAttachArgs.name    = "NativeThread";
    lJavaVMAttachArgs.group   = nullptr;

    lResult = lJavaVM->AttachCurrentThread(&lJNIEnv, &lJavaVMAttachArgs);

    if (lResult == JNI_ERR)
        err() << "Failed to initialize JNI, couldn't get the Unicode value" << std::endl;

    // Retrieve key data from the input event
    const jlong downTime  = AKeyEvent_getDownTime(event);
    const jlong eventTime = AKeyEvent_getEventTime(event);
    const jint  action    = AKeyEvent_getAction(event);
    const jint  code      = AKeyEvent_getKeyCode(event);
    const jint  repeat    = AKeyEvent_getRepeatCount(event); // not sure!
    const jint  metaState = AKeyEvent_getMetaState(event);
    const jint  deviceId  = AInputEvent_getDeviceId(event);
    const jint  scancode  = AKeyEvent_getScanCode(event);
    const jint  flags     = AKeyEvent_getFlags(event);
    const jint  source    = AInputEvent_getSource(event);

    // Construct a KeyEvent object from the event data
    jclass    classKeyEvent       = lJNIEnv->FindClass("android/view/KeyEvent");
    jmethodID keyEventConstructor = lJNIEnv->GetMethodID(classKeyEvent, "<init>", "(JJIIIIIIII)V");
    jobject   objectKeyEvent      = lJNIEnv->NewObject(classKeyEvent,
                                                keyEventConstructor,
                                                downTime,
                                                eventTime,
                                                action,
                                                code,
                                                repeat,
                                                metaState,
                                                deviceId,
                                                scancode,
                                                flags,
                                                source);

    // Call its getUnicodeChar() method to get the Unicode value
    jmethodID methodGetUnicode = lJNIEnv->GetMethodID(classKeyEvent, "getUnicodeChar", "(I)I");
    const int unicode          = lJNIEnv->CallIntMethod(objectKeyEvent, methodGetUnicode, metaState);

    lJNIEnv->DeleteLocalRef(classKeyEvent);
    lJNIEnv->DeleteLocalRef(objectKeyEvent);

    // Detach this thread from the JVM
    lJavaVM->DetachCurrentThread();

    return static_cast<char32_t>(unicode);
}

} // namespace sf::priv
