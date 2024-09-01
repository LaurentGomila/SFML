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

#include <SFML/Window/InputImpl.hpp>
#include <SFML/Window/Unix/ClipboardImpl.hpp>
#include <SFML/Window/Unix/Display.hpp>
#include <SFML/Window/Unix/KeyboardImpl.hpp>
#include <SFML/Window/Unix/Utils.hpp>
#include <SFML/Window/Unix/WindowImplX11.hpp>

#include <SFML/System/Err.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Utf.hpp>

#include <X11/Xlibint.h>
#undef min // Defined by `Xlibint.h`, conflicts with standard headers
#undef max // Defined by `Xlibint.h`, conflicts with standard headers

#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/Xrandr.h>
#include <X11/keysym.h>

#include <algorithm>
#include <array>
#include <bitset>
#include <fcntl.h>
#include <filesystem>
#include <libgen.h>
#include <mutex>
#include <ostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include <cassert>
#include <cstring>

#ifdef SFML_OPENGL_ES
#include <SFML/Window/EglContext.hpp>
using ContextType = sf::priv::EglContext;
#else
#include <SFML/Window/Unix/GlxContext.hpp>
using ContextType = sf::priv::GlxContext;
#endif

////////////////////////////////////////////////////////////
// Private data
////////////////////////////////////////////////////////////
namespace
{
// A nested named namespace is used here to allow unity builds of SFML.
namespace WindowImplX11Impl
{
sf::priv::WindowImplX11*              fullscreenWindow = nullptr;
std::vector<sf::priv::WindowImplX11*> allWindows;
std::bitset<256>                      isKeyFiltered;
std::recursive_mutex                  allWindowsMutex;
sf::String                            windowManagerName;

sf::String wmAbsPosGood[] = {"Enlightenment", "FVWM", "i3"};

constexpr unsigned long eventMask = FocusChangeMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
                                    PointerMotionMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask |
                                    EnterWindowMask | LeaveWindowMask | VisibilityChangeMask | PropertyChangeMask;

constexpr unsigned int maxTrialsCount = 5;

// Filter the events received by windows (only allow those matching a specific window or those needed for the IM to work)
// NOLINTNEXTLINE(readability-non-const-parameter)
Bool checkEvent(::Display*, XEvent* event, XPointer userData)
{
    if (event->xany.window == reinterpret_cast<::Window>(userData) || event->type == GenericEvent)
    {
        // The event matches the current window so pick it up
        return true;
    }
    if (event->type == ClientMessage)
    {
        // The input method sometimes sends ClientMessage with a different window ID.
        // Our event loop has to process them for the IM to work.
        // We assume ClientMessage events not having WM_PROTOCOLS message type are such events.
        // ClientMessage events having WM_PROTOCOLS message type should be handled by their own window,
        // so we ignore them here. They will eventually be picked up with the first condition.
        static const Atom wmProtocols = sf::priv::getAtom("WM_PROTOCOLS");
        return event->xclient.message_type != wmProtocols;
    }
    return false;
}

// Find the name of the current executable
std::filesystem::path findExecutableName()
{
    // We use /proc/self/cmdline to get the command line
    // the user used to invoke this instance of the application
    const int file = ::open("/proc/self/cmdline", O_RDONLY | O_NONBLOCK);

    if (file < 0)
        return "sfml";

    std::vector<char> buffer(256, 0);
    std::size_t       offset = 0;
    ssize_t           result = 0;

    while ((result = read(file, &buffer[offset], 256)) > 0)
    {
        buffer.resize(buffer.size() + static_cast<std::size_t>(result), 0);
        offset += static_cast<std::size_t>(result);
    }

    ::close(file);

    if (offset)
    {
        buffer[offset] = 0;

        // Remove the path to keep the executable name only
        return basename(buffer.data());
    }

    // Default fallback name
    return "sfml";
}

// Check if Extended Window Manager Hints are supported
bool ewmhSupported()
{
    static bool checked       = false;
    static bool ewmhSupported = false;

    if (checked)
        return ewmhSupported;

    checked = true;

    const Atom netSupportingWmCheck = sf::priv::getAtom("_NET_SUPPORTING_WM_CHECK", true);
    const Atom netSupported         = sf::priv::getAtom("_NET_SUPPORTED", true);

    if (!netSupportingWmCheck || !netSupported)
        return false;

    const auto display = sf::priv::openDisplay();

    Atom           actualType   = 0;
    int            actualFormat = 0;
    unsigned long  numItems     = 0;
    unsigned long  numBytes     = 0;
    unsigned char* data         = nullptr;

    int result = XGetWindowProperty(display.get(),
                                    DefaultRootWindow(display.get()),
                                    netSupportingWmCheck,
                                    0,
                                    1,
                                    False,
                                    XA_WINDOW,
                                    &actualType,
                                    &actualFormat,
                                    &numItems,
                                    &numBytes,
                                    &data);

    if (result != Success || actualType != XA_WINDOW || numItems != 1)
    {
        if (result == Success)
            XFree(data);

        return false;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
    const ::Window rootWindow = *reinterpret_cast<::Window*>(data);
#pragma GCC diagnostic pop

    XFree(data);

    if (!rootWindow)
    {
        return false;
    }

    result = XGetWindowProperty(display.get(),
                                rootWindow,
                                netSupportingWmCheck,
                                0,
                                1,
                                False,
                                XA_WINDOW,
                                &actualType,
                                &actualFormat,
                                &numItems,
                                &numBytes,
                                &data);

    if (result != Success || actualType != XA_WINDOW || numItems != 1)
    {
        if (result == Success)
            XFree(data);

        return false;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
    const ::Window childWindow = *reinterpret_cast<::Window*>(data);
#pragma GCC diagnostic pop

    XFree(data);

    if (!childWindow)
        return false;

    // Conforming window managers should return the same window for both queries
    if (rootWindow != childWindow)
        return false;

    ewmhSupported = true;

    // We try to get the name of the window manager
    // for window manager specific workarounds
    const Atom netWmName = sf::priv::getAtom("_NET_WM_NAME", true);

    if (!netWmName)
        return true;

    Atom utf8StringType = sf::priv::getAtom("UTF8_STRING");

    if (!utf8StringType)
        utf8StringType = XA_STRING;

    result = XGetWindowProperty(display.get(),
                                rootWindow,
                                netWmName,
                                0,
                                0x7fffffff,
                                False,
                                utf8StringType,
                                &actualType,
                                &actualFormat,
                                &numItems,
                                &numBytes,
                                &data);

    if (actualType && numItems)
    {
        // It seems the wm name string reply is not necessarily
        // null-terminated. The work around is to get its actual
        // length to build a proper string
        const char* begin = reinterpret_cast<const char*>(data);
        const char* end   = begin + numItems;
        windowManagerName = sf::String::fromUtf8(begin, end);
    }

    if (result == Success)
        XFree(data);

    return true;
}

// Get the parent window.
::Window getParentWindow(::Display* disp, ::Window win)
{
    ::Window     root        = 0;
    ::Window     parent      = 0;
    ::Window*    children    = nullptr;
    unsigned int numChildren = 0;

    XQueryTree(disp, win, &root, &parent, &children, &numChildren);

    // Children information is not used, so must be freed.
    if (children != nullptr)
        XFree(children);

    return parent;
}

// Get the Frame Extents from EWMH WMs that support it.
bool getEWMHFrameExtents(::Display* disp, ::Window win, long& xFrameExtent, long& yFrameExtent)
{
    if (!ewmhSupported())
        return false;

    const Atom frameExtents = sf::priv::getAtom("_NET_FRAME_EXTENTS", true);

    if (frameExtents == None)
        return false;

    bool           gotFrameExtents = false;
    Atom           actualType      = 0;
    int            actualFormat    = 0;
    unsigned long  numItems        = 0;
    unsigned long  numBytesLeft    = 0;
    unsigned char* data            = nullptr;

    const int result = XGetWindowProperty(disp,
                                          win,
                                          frameExtents,
                                          0,
                                          4,
                                          False,
                                          XA_CARDINAL,
                                          &actualType,
                                          &actualFormat,
                                          &numItems,
                                          &numBytesLeft,
                                          &data);

    if ((result == Success) && (actualType == XA_CARDINAL) && (actualFormat == 32) && (numItems == 4) &&
        (numBytesLeft == 0) && (data != nullptr))
    {
        gotFrameExtents = true;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
        long* extents = reinterpret_cast<long*>(data);
#pragma GCC diagnostic pop

        xFrameExtent = extents[0]; // Left.
        yFrameExtent = extents[2]; // Top.
    }

    // Always free data.
    if (data != nullptr)
        XFree(data);

    return gotFrameExtents;
}

// Check if the current WM is in the list of good WMs that provide
// a correct absolute position for the window when queried.
bool isWMAbsolutePositionGood()
{
    // This can only work with EWMH, to get the name.
    if (!ewmhSupported())
        return false;

    return std::any_of(std::begin(wmAbsPosGood),
                       std::end(wmAbsPosGood),
                       [&](const sf::String& name) { return name == windowManagerName; });
}

// Initialize raw mouse input
bool initRawMouse(::Display* disp)
{
    int opcode = 0;
    int event  = 0;
    int error  = 0;

    if (XQueryExtension(disp, "XInputExtension", &opcode, &event, &error))
    {
        int major = 2;
        int minor = 0;

        if (XIQueryVersion(disp, &major, &minor) != BadRequest)
        {
            std::array<unsigned char, XIMaskLen(XI_LASTEVENT)> mask{};
            XISetMask(mask.data(), XI_RawMotion);

            XIEventMask xiEventMask;
            xiEventMask.deviceid = XIAllDevices;
            xiEventMask.mask_len = mask.size();
            xiEventMask.mask     = mask.data();

            if (XISelectEvents(disp, DefaultRootWindow(disp), &xiEventMask, 1) == Success)
                return true;
        }
    }

    return false;
}
} // namespace WindowImplX11Impl
} // namespace


namespace sf::priv
{
////////////////////////////////////////////////////////////
template <>
struct XDeleter<XImage>
{
    void operator()(XImage* image) const
    {
        XDestroyImage(image);
    }
};


////////////////////////////////////////////////////////////
template <>
struct XDeleter<XRRScreenResources>
{
    void operator()(XRRScreenResources* res) const
    {
        XRRFreeScreenResources(res);
    }
};


////////////////////////////////////////////////////////////
template <>
struct XDeleter<XRROutputInfo>
{
    void operator()(XRROutputInfo* outputInfo) const
    {
        XRRFreeOutputInfo(outputInfo);
    }
};


////////////////////////////////////////////////////////////
template <>
struct XDeleter<XRRCrtcInfo>
{
    void operator()(XRRCrtcInfo* crtcInfo) const
    {
        XRRFreeCrtcInfo(crtcInfo);
    }
};


////////////////////////////////////////////////////////////
WindowImplX11::WindowImplX11(WindowHandle handle) : m_isExternal(true)
{
    using namespace WindowImplX11Impl;

    // Open a connection with the X server
    m_display = openDisplay();

    // Make sure to check for EWMH support before we do anything
    ewmhSupported();

    m_screen = DefaultScreen(m_display.get());

    // Save the window handle
    m_window = handle;

    if (m_window)
    {
        // Make sure the window is listening to all the required events
        XSetWindowAttributes attributes;
        attributes.event_mask = eventMask;

        XChangeWindowAttributes(m_display.get(), m_window, CWEventMask, &attributes);

        // Set the WM protocols
        setProtocols();

        // Do some common initializations
        initialize();
    }
}


////////////////////////////////////////////////////////////
WindowImplX11::WindowImplX11(VideoMode mode, const String& title, std::uint32_t style, State state, const ContextSettings& settings) :
m_fullscreen(state == State::Fullscreen),
m_cursorGrabbed(m_fullscreen)
{
    using namespace WindowImplX11Impl;

    // Open a connection with the X server
    m_display = openDisplay();

    // Make sure to check for EWMH support before we do anything
    ewmhSupported();

    m_screen = DefaultScreen(m_display.get());

    // Compute position and size
    Vector2i windowPosition;
    if (m_fullscreen)
    {
        windowPosition = getPrimaryMonitorPosition();
    }
    else
    {
        const Vector2i displaySize(DisplayWidth(m_display.get(), m_screen), DisplayHeight(m_display.get(), m_screen));
        windowPosition = displaySize - Vector2i(mode.size) / 2;
    }

    const unsigned int width  = mode.size.x;
    const unsigned int height = mode.size.y;

    Visual* visual = nullptr;
    int     depth  = 0;

    // Check if the user chose to not create an OpenGL context (settings.attributeFlags will be 0xFFFFFFFF)
    if (settings.attributeFlags == 0xFFFFFFFF)
    {
        // Choose default visual since the user is going to use their own rendering API
        visual = DefaultVisual(m_display.get(), m_screen);
        depth  = DefaultDepth(m_display.get(), m_screen);
    }
    else
    {
        // Choose the visual according to the context settings
        const XVisualInfo visualInfo = ContextType::selectBestVisual(m_display.get(), mode.bitsPerPixel, settings);

        visual = visualInfo.visual;
        depth  = visualInfo.depth;
    }

    // Define the window attributes
    XSetWindowAttributes attributes;
    attributes.colormap   = XCreateColormap(m_display.get(), DefaultRootWindow(m_display.get()), visual, AllocNone);
    attributes.event_mask = eventMask;
    attributes.override_redirect = (m_fullscreen && !ewmhSupported()) ? True : False;

    m_window = XCreateWindow(m_display.get(),
                             DefaultRootWindow(m_display.get()),
                             windowPosition.x,
                             windowPosition.y,
                             width,
                             height,
                             0,
                             depth,
                             InputOutput,
                             visual,
                             CWEventMask | CWOverrideRedirect | CWColormap,
                             &attributes);

    if (!m_window)
    {
        err() << "Failed to create window" << std::endl;
        return;
    }

    // Set the WM protocols
    setProtocols();

    // Set the WM initial state to the normal state
    XWMHints xHints{};
    xHints.flags         = StateHint;
    xHints.initial_state = NormalState;
    XSetWMHints(m_display.get(), m_window, &xHints);

    // If not in fullscreen, set the window's style (tell the window manager to
    // change our window's decorations and functions according to the requested style)
    if (!m_fullscreen)
    {
        const Atom wmHintsAtom = getAtom("_MOTIF_WM_HINTS", false);
        if (wmHintsAtom)
        {
            // NOLINTBEGIN(readability-identifier-naming)
            // Disable naming check so these better match the contents of the Motif library
            // https://sourceforge.net/p/motif/code/ci/master/tree/lib/Xm/MwmUtil.h
            constexpr unsigned long MWM_HINTS_FUNCTIONS   = 1 << 0;
            constexpr unsigned long MWM_HINTS_DECORATIONS = 1 << 1;

            //constexpr unsigned long MWM_DECOR_ALL         = 1 << 0;
            constexpr unsigned long MWM_DECOR_BORDER   = 1 << 1;
            constexpr unsigned long MWM_DECOR_RESIZEH  = 1 << 2;
            constexpr unsigned long MWM_DECOR_TITLE    = 1 << 3;
            constexpr unsigned long MWM_DECOR_MENU     = 1 << 4;
            constexpr unsigned long MWM_DECOR_MINIMIZE = 1 << 5;
            constexpr unsigned long MWM_DECOR_MAXIMIZE = 1 << 6;

            //constexpr unsigned long MWM_FUNC_ALL          = 1 << 0;
            constexpr unsigned long MWM_FUNC_RESIZE   = 1 << 1;
            constexpr unsigned long MWM_FUNC_MOVE     = 1 << 2;
            constexpr unsigned long MWM_FUNC_MINIMIZE = 1 << 3;
            constexpr unsigned long MWM_FUNC_MAXIMIZE = 1 << 4;
            constexpr unsigned long MWM_FUNC_CLOSE    = 1 << 5;
            // NOLINTEND(readability-identifier-naming)

            struct WMHints
            {
                unsigned long flags{MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS};
                unsigned long functions{};
                unsigned long decorations{};
                long          inputMode{};
                unsigned long state{};
            } hints;

            if (style & Style::Titlebar)
            {
                hints.decorations |= MWM_DECOR_BORDER | MWM_DECOR_TITLE | MWM_DECOR_MINIMIZE | MWM_DECOR_MENU;
                hints.functions |= MWM_FUNC_MOVE | MWM_FUNC_MINIMIZE;
            }
            if (style & Style::Resize)
            {
                hints.decorations |= MWM_DECOR_MAXIMIZE | MWM_DECOR_RESIZEH;
                hints.functions |= MWM_FUNC_MAXIMIZE | MWM_FUNC_RESIZE;
            }
            if (style & Style::Close)
            {
                hints.decorations |= 0;
                hints.functions |= MWM_FUNC_CLOSE;
            }

            XChangeProperty(m_display.get(),
                            m_window,
                            wmHintsAtom,
                            wmHintsAtom,
                            32,
                            PropModeReplace,
                            reinterpret_cast<const unsigned char*>(&hints),
                            5);
        }
    }

    // This is a hack to force some windows managers to disable resizing
    if (!(style & Style::Resize))
    {
        m_useSizeHints = true;
        XSizeHints sizeHints{};
        sizeHints.flags     = PMinSize | PMaxSize | USPosition;
        sizeHints.min_width = sizeHints.max_width = static_cast<int>(width);
        sizeHints.min_height = sizeHints.max_height = static_cast<int>(height);
        sizeHints.x                                 = windowPosition.x;
        sizeHints.y                                 = windowPosition.y;
        XSetWMNormalHints(m_display.get(), m_window, &sizeHints);
    }

    // Set the window's WM class (this can be used by window managers)
    XClassHint hint{};

    // The instance name should be something unique to this invocation
    // of the application but is rarely if ever used these days.
    // For simplicity, we retrieve it via the base executable name.
    std::string       executableName = findExecutableName().string();
    std::vector<char> windowInstance(executableName.size() + 1, 0);
    std::copy(executableName.begin(), executableName.end(), windowInstance.begin());
    hint.res_name = windowInstance.data();

    // The class name identifies a class of windows that
    // "are of the same type". We simply use the initial window name as
    // the class name.
    std::string       ansiTitle = title.toAnsiString();
    std::vector<char> windowClass(ansiTitle.size() + 1, 0);
    std::copy(ansiTitle.begin(), ansiTitle.end(), windowClass.begin());
    hint.res_class = windowClass.data();

    XSetClassHint(m_display.get(), m_window, &hint);

    // Set the window's name
    setTitle(title);

    // Do some common initializations
    initialize();

    // Set fullscreen video mode and switch to fullscreen if necessary
    if (m_fullscreen)
    {
        // Disable hint for min and max size,
        // otherwise some windows managers will not remove window decorations
        XSizeHints sizeHints{};
        long       flags = 0;
        XGetWMNormalHints(m_display.get(), m_window, &sizeHints, &flags);
        sizeHints.flags &= ~(PMinSize | PMaxSize);
        XSetWMNormalHints(m_display.get(), m_window, &sizeHints);

        setVideoMode(mode);
        switchToFullscreen();
    }
}


////////////////////////////////////////////////////////////
WindowImplX11::~WindowImplX11()
{
    using namespace WindowImplX11Impl;

    // Cleanup graphical resources
    cleanup();

    // Destroy icon pixmap
    if (m_iconPixmap)
        XFreePixmap(m_display.get(), m_iconPixmap);

    // Destroy icon mask pixmap
    if (m_iconMaskPixmap)
        XFreePixmap(m_display.get(), m_iconMaskPixmap);

    // Destroy the cursor
    if (m_hiddenCursor)
        XFreeCursor(m_display.get(), m_hiddenCursor);

    // Destroy the input context
    if (m_inputContext)
        XDestroyIC(m_inputContext);

    // Destroy the window
    if (m_window && !m_isExternal)
    {
        XDestroyWindow(m_display.get(), m_window);
        XFlush(m_display.get());
    }

    // Remove this window from the global list of windows (required for focus request)
    const std::lock_guard lock(allWindowsMutex);
    allWindows.erase(std::find(allWindows.begin(), allWindows.end(), this));
}


////////////////////////////////////////////////////////////
WindowHandle WindowImplX11::getNativeHandle() const
{
    return m_window;
}


////////////////////////////////////////////////////////////
void WindowImplX11::processEvents()
{
    using namespace WindowImplX11Impl;

    XEvent event;

    // Pick out the events that are interesting for this window
    while (XCheckIfEvent(m_display.get(), &event, &checkEvent, reinterpret_cast<XPointer>(m_window)))
    {
        // This function implements a workaround to properly discard
        // repeated key events when necessary. The problem is that the
        // system's key events policy doesn't match SFML's one: X server will generate
        // both repeated KeyPress and KeyRelease events when maintaining a key down, while
        // SFML only wants repeated KeyPress events. Thus, we have to:
        // - Discard duplicated KeyRelease events when m_keyRepeat is true
        // - Discard both duplicated KeyPress and KeyRelease events when m_keyRepeat is false

        bool processThisEvent = true;

        // Detect repeated key events
        while (event.type == KeyRelease)
        {
            XEvent nextEvent;
            if (XCheckIfEvent(m_display.get(), &nextEvent, checkEvent, reinterpret_cast<XPointer>(m_window)))
            {
                if ((nextEvent.type == KeyPress) && (nextEvent.xkey.keycode == event.xkey.keycode) &&
                    (event.xkey.time <= nextEvent.xkey.time) && (nextEvent.xkey.time <= event.xkey.time + 1))
                {
                    // This sequence of events comes from maintaining a key down
                    if (m_keyRepeat)
                    {
                        // Ignore the KeyRelease event and process the KeyPress event
                        event = nextEvent;
                        break;
                    }

                    // Ignore both events
                    processThisEvent = false;
                    break;
                }

                // This sequence of events does not come from maintaining a key down,
                // so process the KeyRelease event normally,
                processEvent(event);
                // but loop because the next event can be the first half
                // of a sequence coming from maintaining a key down.
                event = nextEvent;
            }
            else
            {
                // No event after this KeyRelease event so assume it can be processed.
                break;
            }
        }

        if (processThisEvent)
        {
            processEvent(event);
        }
    }

    // Process clipboard window events
    ClipboardImpl::processEvents();
}


////////////////////////////////////////////////////////////
Vector2i WindowImplX11::getPosition() const
{
    using namespace WindowImplX11Impl;

    // Get absolute position of our window relative to root window. This
    // takes into account all information that X11 has, including X11
    // border widths and any decorations. It corresponds to where the
    // window actually is, but not necessarily to where we told it to
    // go using setPosition() and XMoveWindow(). To have the two match
    // as expected, we may have to subtract decorations and borders.
    ::Window child         = 0;
    int      xAbsRelToRoot = 0;
    int      yAbsRelToRoot = 0;

    XTranslateCoordinates(m_display.get(), m_window, DefaultRootWindow(m_display.get()), 0, 0, &xAbsRelToRoot, &yAbsRelToRoot, &child);

    // CASE 1: some rare WMs actually put the window exactly where we tell
    // it to, even with decorations and such, which get shifted back.
    // In these rare cases, we can use the absolute value directly.
    if (isWMAbsolutePositionGood())
        return {xAbsRelToRoot, yAbsRelToRoot};

    // CASE 2: most modern WMs support EWMH and can define _NET_FRAME_EXTENTS
    // with the exact frame size to subtract, so if present, we prefer it and
    // query it first. According to spec, this already includes any borders.
    long xFrameExtent = 0;
    long yFrameExtent = 0;

    if (getEWMHFrameExtents(m_display.get(), m_window, xFrameExtent, yFrameExtent))
    {
        // Get final X/Y coordinates: subtract EWMH frame extents from
        // absolute window position.
        return {(xAbsRelToRoot - static_cast<int>(xFrameExtent)), (yAbsRelToRoot - static_cast<int>(yFrameExtent))};
    }

    // CASE 3: EWMH frame extents were not available, use geometry.
    // We climb back up to the window before the root and use its
    // geometry information to extract X/Y position. This because
    // re-parenting WMs may re-parent the window multiple times, so
    // we'd have to climb up to the furthest ancestor and sum the
    // relative differences and borders anyway; and doing that to
    // subtract those values from the absolute coordinates of the
    // window is equivalent to going up the tree and asking the
    // furthest ancestor what it's relative distance to the root is.
    // So we use that approach because it's simpler.
    // This approach assumes that any window between the root and
    // our window is part of decorations/borders in some way. This
    // seems to hold true for most reasonable WM implementations.
    ::Window ancestor = m_window;
    ::Window root     = DefaultRootWindow(m_display.get());

    while (getParentWindow(m_display.get(), ancestor) != root)
    {
        // Next window up (parent window).
        ancestor = getParentWindow(m_display.get(), ancestor);
    }

    // Get final X/Y coordinates: take the relative position to
    // the root of the furthest ancestor window.
    int          xRelToRoot  = 0;
    int          yRelToRoot  = 0;
    unsigned int width       = 0;
    unsigned int height      = 0;
    unsigned int borderWidth = 0;
    unsigned int depth       = 0;

    XGetGeometry(m_display.get(), ancestor, &root, &xRelToRoot, &yRelToRoot, &width, &height, &borderWidth, &depth);

    return {xRelToRoot, yRelToRoot};
}


////////////////////////////////////////////////////////////
void WindowImplX11::setPosition(Vector2i position)
{
    XMoveWindow(m_display.get(), m_window, position.x, position.y);
    XFlush(m_display.get());
}


////////////////////////////////////////////////////////////
Vector2u WindowImplX11::getSize() const
{
    XWindowAttributes attributes;
    XGetWindowAttributes(m_display.get(), m_window, &attributes);
    return Vector2u(Vector2i(attributes.width, attributes.height));
}


////////////////////////////////////////////////////////////
void WindowImplX11::setSize(Vector2u size)
{
    // If resizing is disable for the window we have to update the size hints (required by some window managers).
    if (m_useSizeHints)
    {
        XSizeHints sizeHints{};
        sizeHints.flags     = PMinSize | PMaxSize;
        sizeHints.min_width = sizeHints.max_width = static_cast<int>(size.x);
        sizeHints.min_height = sizeHints.max_height = static_cast<int>(size.y);
        XSetWMNormalHints(m_display.get(), m_window, &sizeHints);
    }

    XResizeWindow(m_display.get(), m_window, size.x, size.y);
    XFlush(m_display.get());
}


////////////////////////////////////////////////////////////
void WindowImplX11::setMinimumSize(const std::optional<Vector2u>& minimumSize)
{
    WindowImpl::setMinimumSize(minimumSize);
    setWindowSizeConstraints();
}


////////////////////////////////////////////////////////////
void WindowImplX11::setMaximumSize(const std::optional<Vector2u>& maximumSize)
{
    WindowImpl::setMaximumSize(maximumSize);
    setWindowSizeConstraints();
}


////////////////////////////////////////////////////////////
void WindowImplX11::setTitle(const String& title)
{
    // Bare X11 has no Unicode window title support.
    // There is however an option to tell the window manager your Unicode title via hints.

    // Convert to UTF-8 encoding.
    const auto utf8Title = title.toUtf8();

    const Atom useUtf8 = getAtom("UTF8_STRING", false);

    // Set the _NET_WM_NAME atom, which specifies a UTF-8 encoded window title.
    const Atom wmName = getAtom("_NET_WM_NAME", false);
    XChangeProperty(m_display.get(),
                    m_window,
                    wmName,
                    useUtf8,
                    8,
                    PropModeReplace,
                    utf8Title.c_str(),
                    static_cast<int>(utf8Title.size()));

    // Set the _NET_WM_ICON_NAME atom, which specifies a UTF-8 encoded window title.
    const Atom wmIconName = getAtom("_NET_WM_ICON_NAME", false);
    XChangeProperty(m_display.get(),
                    m_window,
                    wmIconName,
                    useUtf8,
                    8,
                    PropModeReplace,
                    utf8Title.c_str(),
                    static_cast<int>(utf8Title.size()));

    // Set the non-Unicode title as a fallback for window managers who don't support _NET_WM_NAME.
    Xutf8SetWMProperties(m_display.get(),
                         m_window,
                         title.toAnsiString().c_str(),
                         title.toAnsiString().c_str(),
                         nullptr,
                         0,
                         nullptr,
                         nullptr,
                         nullptr);
}


////////////////////////////////////////////////////////////
void WindowImplX11::setIcon(Vector2u size, const std::uint8_t* pixels)
{
    // X11 wants BGRA pixels: swap red and blue channels
    // Note: this memory will be freed by X11Ptr<XImage> deleter
    // NOLINTBEGIN(cppcoreguidelines-no-malloc)
    auto* iconPixels = static_cast<std::uint8_t*>(
        std::malloc(static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y) * 4));
    // NOLINTEND(cppcoreguidelines-no-malloc)
    for (std::size_t i = 0; i < static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y); ++i)
    {
        iconPixels[i * 4 + 0] = pixels[i * 4 + 2];
        iconPixels[i * 4 + 1] = pixels[i * 4 + 1];
        iconPixels[i * 4 + 2] = pixels[i * 4 + 0];
        iconPixels[i * 4 + 3] = pixels[i * 4 + 3];
    }

    // Create the icon pixmap
    Visual*    defVisual = DefaultVisual(m_display.get(), m_screen);
    const auto defDepth  = static_cast<unsigned int>(DefaultDepth(m_display.get(), m_screen));
    const auto iconImage = X11Ptr<XImage>(
        XCreateImage(m_display.get(), defVisual, defDepth, ZPixmap, 0, reinterpret_cast<char*>(iconPixels), size.x, size.y, 32, 0));
    if (!iconImage)
    {
        err() << "Failed to set the window's icon" << std::endl;
        return;
    }

    if (m_iconPixmap)
        XFreePixmap(m_display.get(), m_iconPixmap);

    if (m_iconMaskPixmap)
        XFreePixmap(m_display.get(), m_iconMaskPixmap);

    m_iconPixmap = XCreatePixmap(m_display.get(), RootWindow(m_display.get(), m_screen), size.x, size.y, defDepth);
    XGCValues values;
    GC        iconGC = XCreateGC(m_display.get(), m_iconPixmap, 0, &values);
    XPutImage(m_display.get(), m_iconPixmap, iconGC, iconImage.get(), 0, 0, 0, 0, size.x, size.y);
    XFreeGC(m_display.get(), iconGC);

    // Create the mask pixmap (must have 1 bit depth)
    const std::size_t         pitch = (size.x + 7) / 8;
    std::vector<std::uint8_t> maskPixels(pitch * size.y, 0);
    for (std::size_t j = 0; j < size.y; ++j)
    {
        for (std::size_t i = 0; i < pitch; ++i)
        {
            for (std::size_t k = 0; k < 8; ++k)
            {
                if (i * 8 + k < size.x)
                {
                    const std::uint8_t opacity = (pixels[(i * 8 + k + j * size.x) * 4 + 3] > 0) ? 1 : 0;
                    maskPixels[i + j * pitch] |= static_cast<std::uint8_t>(opacity << k);
                }
            }
        }
    }
    m_iconMaskPixmap = XCreatePixmapFromBitmapData(m_display.get(),
                                                   m_window,
                                                   reinterpret_cast<char*>(maskPixels.data()),
                                                   size.x,
                                                   size.y,
                                                   1,
                                                   0,
                                                   1);

    // Send our new icon to the window through the WMHints
    XWMHints hints{};
    hints.flags       = IconPixmapHint | IconMaskHint;
    hints.icon_pixmap = m_iconPixmap;
    hints.icon_mask   = m_iconMaskPixmap;
    XSetWMHints(m_display.get(), m_window, &hints);

    // ICCCM wants BGRA pixels: swap red and blue channels
    // ICCCM also wants the first 2 unsigned 32-bit values to be width and height
    std::vector<unsigned long> icccmIconPixels(2 + size.x * size.y, 0);
    unsigned long*             ptr = icccmIconPixels.data();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference" // False positive.
    *ptr++ = size.x;
    *ptr++ = size.y;
#pragma GCC diagnostic pop

    for (std::size_t i = 0; i < static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y); ++i)
    {
        *ptr++ = static_cast<unsigned long>(
            (pixels[i * 4 + 2] << 0) | (pixels[i * 4 + 1] << 8) | (pixels[i * 4 + 0] << 16) | (pixels[i * 4 + 3] << 24));
    }

    const Atom netWmIcon = getAtom("_NET_WM_ICON");

    XChangeProperty(m_display.get(),
                    m_window,
                    netWmIcon,
                    XA_CARDINAL,
                    32,
                    PropModeReplace,
                    reinterpret_cast<const unsigned char*>(icccmIconPixels.data()),
                    static_cast<int>(2 + size.x * size.y));

    XFlush(m_display.get());
}


////////////////////////////////////////////////////////////
void WindowImplX11::setVisible(bool visible)
{
    if (visible)
    {
        XMapWindow(m_display.get(), m_window);

        if (m_fullscreen)
            switchToFullscreen();

        XFlush(m_display.get());

        // Before continuing, make sure the WM has
        // internally marked the window as viewable
        while (!m_windowMapped && !m_isExternal)
            processEvents();
    }
    else
    {
        XUnmapWindow(m_display.get(), m_window);

        XFlush(m_display.get());

        // Before continuing, make sure the WM has
        // internally marked the window as unviewable
        while (m_windowMapped && !m_isExternal)
            processEvents();
    }
}


////////////////////////////////////////////////////////////
void WindowImplX11::setMouseCursorVisible(bool visible)
{
    XDefineCursor(m_display.get(), m_window, visible ? m_lastCursor : m_hiddenCursor);
    XFlush(m_display.get());
}


////////////////////////////////////////////////////////////
void WindowImplX11::setMouseCursor(const CursorImpl& cursor)
{
    m_lastCursor = cursor.m_cursor;
    XDefineCursor(m_display.get(), m_window, m_lastCursor);
    XFlush(m_display.get());
}


////////////////////////////////////////////////////////////
void WindowImplX11::setMouseCursorGrabbed(bool grabbed)
{
    using namespace WindowImplX11Impl;

    // This has no effect in fullscreen mode
    if (m_fullscreen || (m_cursorGrabbed == grabbed))
        return;

    if (grabbed)
    {
        // Try multiple times to grab the cursor
        for (unsigned int trial = 0; trial < maxTrialsCount; ++trial)
        {
            const int result = XGrabPointer(m_display.get(), m_window, True, None, GrabModeAsync, GrabModeAsync, m_window, None, CurrentTime);

            if (result == GrabSuccess)
            {
                m_cursorGrabbed = true;
                break;
            }

            // The cursor grab failed, trying again after a small sleep
            sf::sleep(sf::milliseconds(50));
        }

        if (!m_cursorGrabbed)
            err() << "Failed to grab mouse cursor" << std::endl;
    }
    else
    {
        // Release the cursor from the window and disable cursor grabbing
        XUngrabPointer(m_display.get(), CurrentTime);
        m_cursorGrabbed = false;
    }
}


////////////////////////////////////////////////////////////
void WindowImplX11::setKeyRepeatEnabled(bool enabled)
{
    m_keyRepeat = enabled;
}


////////////////////////////////////////////////////////////
void WindowImplX11::requestFocus()
{
    using namespace WindowImplX11Impl;

    // Focus is only stolen among SFML windows, not between applications
    // Check the global list of windows to find out whether an SFML window has the focus
    // Note: can't handle console and other non-SFML windows belonging to the application.
    bool sfmlWindowFocused = false;

    {
        const std::lock_guard lock(allWindowsMutex);
        for (sf::priv::WindowImplX11* windowPtr : allWindows)
        {
            if (windowPtr->hasFocus())
            {
                sfmlWindowFocused = true;
                break;
            }
        }
    }

    // Check if window is viewable (not on other desktop, ...)
    // TODO: Check also if minimized
    XWindowAttributes attributes;
    if (XGetWindowAttributes(m_display.get(), m_window, &attributes) == 0)
    {
        sf::err() << "Failed to check if window is viewable while requesting focus" << std::endl;
        return; // error getting attribute
    }

    const bool windowViewable = (attributes.map_state == IsViewable);

    if (sfmlWindowFocused && windowViewable)
    {
        // Another SFML window of this application has the focus and the current window is viewable:
        // steal focus (i.e. bring window to the front and give it input focus)
        grabFocus();
    }
    else
    {
        // Otherwise: display urgency hint (flashing application logo)
        // Ensure WM hints exist, allocate if necessary
        auto hints = X11Ptr<XWMHints>(XGetWMHints(m_display.get(), m_window));
        if (hints == nullptr)
            hints.reset(XAllocWMHints());

        // Add urgency (notification) flag to hints
        hints->flags |= XUrgencyHint;
        XSetWMHints(m_display.get(), m_window, hints.get());
    }
}


////////////////////////////////////////////////////////////
bool WindowImplX11::hasFocus() const
{
    ::Window focusedWindow  = 0;
    int      revertToReturn = 0;
    XGetInputFocus(m_display.get(), &focusedWindow, &revertToReturn);

    return m_window == focusedWindow;
}


////////////////////////////////////////////////////////////
void WindowImplX11::grabFocus()
{
    using namespace WindowImplX11Impl;

    Atom netActiveWindow = None;

    if (ewmhSupported())
        netActiveWindow = getAtom("_NET_ACTIVE_WINDOW");

    // Only try to grab focus if the window is mapped
    XWindowAttributes attr;

    XGetWindowAttributes(m_display.get(), m_window, &attr);

    if (attr.map_state == IsUnmapped)
        return;

    if (netActiveWindow)
    {
        auto event                 = XEvent();
        event.type                 = ClientMessage;
        event.xclient.window       = m_window;
        event.xclient.format       = 32;
        event.xclient.message_type = netActiveWindow;
        event.xclient.data.l[0]    = 1; // Normal application
        event.xclient.data.l[1]    = static_cast<long>(m_lastInputTime);
        event.xclient.data.l[2]    = 0; // We don't know the currently active window

        const int result = XSendEvent(m_display.get(),
                                      DefaultRootWindow(m_display.get()),
                                      False,
                                      SubstructureNotifyMask | SubstructureRedirectMask,
                                      &event);

        XFlush(m_display.get());

        if (!result)
            err() << "Setting fullscreen failed, could not send \"_NET_ACTIVE_WINDOW\" event" << std::endl;
    }
    else
    {
        XRaiseWindow(m_display.get(), m_window);
        XSetInputFocus(m_display.get(), m_window, RevertToPointerRoot, CurrentTime);
        XFlush(m_display.get());
    }
}


////////////////////////////////////////////////////////////
void WindowImplX11::setVideoMode(const VideoMode& mode)
{
    using namespace WindowImplX11Impl;

    // Skip mode switching if the new mode is equal to the desktop mode
    if (mode == VideoMode::getDesktopMode())
        return;

    // Check if the XRandR extension is present
    if (!checkXRandR())
    {
        // XRandR extension is not supported: we cannot use fullscreen mode
        err() << "Fullscreen is not supported, switching to window mode" << std::endl;
        return;
    }

    // Get root window
    ::Window rootWindow = RootWindow(m_display.get(), m_screen);

    // Get the screen resources
    const auto res = X11Ptr<XRRScreenResources>(XRRGetScreenResources(m_display.get(), rootWindow));
    if (!res)
    {
        err() << "Failed to get the current screen resources for fullscreen mode, switching to window mode" << std::endl;
        return;
    }

    RROutput output = getOutputPrimary(rootWindow, res.get());

    // Get output info from output
    const auto outputInfo = X11Ptr<XRROutputInfo>(XRRGetOutputInfo(m_display.get(), res.get(), output));
    if (!outputInfo || outputInfo->connection == RR_Disconnected)
    {
        err() << "Failed to get output info for fullscreen mode, switching to window mode" << std::endl;
        return;
    }

    // Retrieve current RRMode, screen position and rotation
    const auto crtcInfo = X11Ptr<XRRCrtcInfo>(XRRGetCrtcInfo(m_display.get(), res.get(), outputInfo->crtc));
    if (!crtcInfo)
    {
        err() << "Failed to get crtc info for fullscreen mode, switching to window mode" << std::endl;
        return;
    }

    // Find RRMode to set
    bool   modeFound = false;
    RRMode xRandMode = 0;

    for (int i = 0; (i < res->nmode) && !modeFound; ++i)
    {
        if (crtcInfo->rotation == RR_Rotate_90 || crtcInfo->rotation == RR_Rotate_270)
            std::swap(res->modes[i].height, res->modes[i].width);

        // Check if screen size match
        if ((res->modes[i].width == mode.size.x) && (res->modes[i].height == mode.size.y))
        {
            xRandMode = res->modes[i].id;
            modeFound = true;
        }
    }

    if (!modeFound)
    {
        err() << "Failed to find a matching RRMode for fullscreen mode, switching to window mode" << std::endl;
        return;
    }

    // Save the current video mode before we switch to fullscreen
    m_oldVideoMode = crtcInfo->mode;
    m_oldRRCrtc    = outputInfo->crtc;

    // Switch to fullscreen mode
    XRRSetCrtcConfig(m_display.get(),
                     res.get(),
                     outputInfo->crtc,
                     CurrentTime,
                     crtcInfo->x,
                     crtcInfo->y,
                     xRandMode,
                     crtcInfo->rotation,
                     &output,
                     1);

    // Set "this" as the current fullscreen window
    fullscreenWindow = this;
}


////////////////////////////////////////////////////////////
void WindowImplX11::resetVideoMode()
{
    using namespace WindowImplX11Impl;

    if (fullscreenWindow == this)
    {
        // Try to set old configuration
        // Check if the XRandR extension
        if (checkXRandR())
        {
            const auto res = X11Ptr<XRRScreenResources>(
                XRRGetScreenResources(m_display.get(), DefaultRootWindow(m_display.get())));
            if (!res)
            {
                err() << "Failed to get the current screen resources to reset the video mode" << std::endl;
                return;
            }

            // Retrieve current screen position and rotation
            const auto crtcInfo = X11Ptr<XRRCrtcInfo>(XRRGetCrtcInfo(m_display.get(), res.get(), m_oldRRCrtc));
            if (!crtcInfo)
            {
                err() << "Failed to get crtc info to reset the video mode" << std::endl;
                return;
            }

            // Get the primary screen
            RROutput output = XRRGetOutputPrimary(m_display.get(), DefaultRootWindow(m_display.get()));

            // Check if returned output is valid, otherwise use the first screen
            if (output == None)
                output = res->outputs[0];

            XRRSetCrtcConfig(m_display.get(),
                             res.get(),
                             m_oldRRCrtc,
                             CurrentTime,
                             crtcInfo->x,
                             crtcInfo->y,
                             m_oldVideoMode,
                             crtcInfo->rotation,
                             &output,
                             1);
        }

        // Reset the fullscreen window
        fullscreenWindow = nullptr;
    }
}


////////////////////////////////////////////////////////////
void WindowImplX11::switchToFullscreen()
{
    using namespace WindowImplX11Impl;

    grabFocus();

    if (ewmhSupported())
    {
        const Atom netWmBypassCompositor = getAtom("_NET_WM_BYPASS_COMPOSITOR");

        if (netWmBypassCompositor)
        {
            constexpr unsigned long bypassCompositor = 1;

            XChangeProperty(m_display.get(),
                            m_window,
                            netWmBypassCompositor,
                            XA_CARDINAL,
                            32,
                            PropModeReplace,
                            reinterpret_cast<const unsigned char*>(&bypassCompositor),
                            1);
        }

        const Atom netWmState           = getAtom("_NET_WM_STATE", true);
        const Atom netWmStateFullscreen = getAtom("_NET_WM_STATE_FULLSCREEN", true);

        if (!netWmState || !netWmStateFullscreen)
        {
            err() << "Setting fullscreen failed. Could not get required atoms" << std::endl;
            return;
        }

        auto event                 = XEvent();
        event.type                 = ClientMessage;
        event.xclient.window       = m_window;
        event.xclient.format       = 32;
        event.xclient.message_type = netWmState;
        event.xclient.data.l[0]    = 1; // _NET_WM_STATE_ADD
        event.xclient.data.l[1]    = static_cast<long>(netWmStateFullscreen);
        event.xclient.data.l[2]    = 0; // No second property
        event.xclient.data.l[3]    = 1; // Normal window

        const int result = XSendEvent(m_display.get(),
                                      DefaultRootWindow(m_display.get()),
                                      False,
                                      SubstructureNotifyMask | SubstructureRedirectMask,
                                      &event);

        if (!result)
            err() << "Setting fullscreen failed, could not send \"_NET_WM_STATE\" event" << std::endl;
    }
}


////////////////////////////////////////////////////////////
void WindowImplX11::setProtocols()
{
    using namespace WindowImplX11Impl;

    const Atom wmProtocols    = getAtom("WM_PROTOCOLS");
    const Atom wmDeleteWindow = getAtom("WM_DELETE_WINDOW");

    if (!wmProtocols)
    {
        err() << "Failed to request WM_PROTOCOLS atom." << std::endl;
        return;
    }

    std::vector<Atom> atoms;

    if (wmDeleteWindow)
    {
        atoms.push_back(wmDeleteWindow);
    }
    else
    {
        err() << "Failed to request WM_DELETE_WINDOW atom." << std::endl;
    }

    Atom netWmPing = None;
    Atom netWmPid  = None;

    if (ewmhSupported())
    {
        netWmPing = getAtom("_NET_WM_PING", true);
        netWmPid  = getAtom("_NET_WM_PID", true);
    }

    if (netWmPing && netWmPid)
    {
        const long pid = getpid();

        XChangeProperty(m_display.get(),
                        m_window,
                        netWmPid,
                        XA_CARDINAL,
                        32,
                        PropModeReplace,
                        reinterpret_cast<const unsigned char*>(&pid),
                        1);

        atoms.push_back(netWmPing);
    }

    if (!atoms.empty())
    {
        XChangeProperty(m_display.get(),
                        m_window,
                        wmProtocols,
                        XA_ATOM,
                        32,
                        PropModeReplace,
                        reinterpret_cast<const unsigned char*>(atoms.data()),
                        static_cast<int>(atoms.size()));
    }
    else
    {
        err() << "Didn't set any window protocols" << std::endl;
    }
}


////////////////////////////////////////////////////////////
void WindowImplX11::initialize()
{
    using namespace WindowImplX11Impl;

    // Create the input context
    m_inputMethod = openXim();

    if (m_inputMethod)
    {
        m_inputContext = XCreateIC(m_inputMethod.get(),
                                   XNClientWindow,
                                   m_window,
                                   XNFocusWindow,
                                   m_window,
                                   XNInputStyle,
                                   XIMPreeditNothing | XIMStatusNothing,
                                   nullptr);
    }
    else
    {
        m_inputContext = nullptr;
    }

    if (!m_inputContext)
        err() << "Failed to create input context for window -- TextEntered event won't be able to return unicode"
              << std::endl;

    const Atom wmWindowType       = getAtom("_NET_WM_WINDOW_TYPE", false);
    Atom       wmWindowTypeNormal = getAtom("_NET_WM_WINDOW_TYPE_NORMAL", false);

    if (wmWindowType && wmWindowTypeNormal)
    {
        XChangeProperty(m_display.get(),
                        m_window,
                        wmWindowType,
                        XA_ATOM,
                        32,
                        PropModeReplace,
                        reinterpret_cast<const unsigned char*>(&wmWindowTypeNormal),
                        1);
    }

    // Enable raw input in first window
    if (allWindows.empty())
    {
        if (!initRawMouse(m_display.get()))
            sf::err() << "Failed to initialize raw mouse input" << std::endl;
    }

    // Show the window
    setVisible(true);

    // Raise the window and grab input focus
    grabFocus();

    // Create the hidden cursor
    createHiddenCursor();

    // Flush the commands queue
    XFlush(m_display.get());

    // Make sure that file dropping is disabled
    setFileDroppingEnabled(false);

    // Add this window to the global list of windows (required for focus request)
    const std::lock_guard lock(allWindowsMutex);
    allWindows.push_back(this);
}


////////////////////////////////////////////////////////////
void WindowImplX11::updateLastInputTime(::Time time)
{
    if (time && (time != m_lastInputTime))
    {
        const Atom netWmUserTime = getAtom("_NET_WM_USER_TIME", true);

        if (netWmUserTime)
        {
            XChangeProperty(m_display.get(),
                            m_window,
                            netWmUserTime,
                            XA_CARDINAL,
                            32,
                            PropModeReplace,
                            reinterpret_cast<const unsigned char*>(&time),
                            1);
        }

        m_lastInputTime = time;
    }
}


////////////////////////////////////////////////////////////
void WindowImplX11::createHiddenCursor()
{
    // Create the cursor's pixmap (1x1 pixels)
    const Pixmap cursorPixmap    = XCreatePixmap(m_display.get(), m_window, 1, 1, 1);
    GC           graphicsContext = XCreateGC(m_display.get(), cursorPixmap, 0, nullptr);
    XDrawPoint(m_display.get(), cursorPixmap, graphicsContext, 0, 0);
    XFreeGC(m_display.get(), graphicsContext);

    // Create the cursor, using the pixmap as both the shape and the mask of the cursor
    XColor color;
    color.flags = DoRed | DoGreen | DoBlue;
    color.red = color.blue = color.green = 0;
    m_hiddenCursor = XCreatePixmapCursor(m_display.get(), cursorPixmap, cursorPixmap, &color, &color, 0, 0);

    // We don't need the pixmap any longer, free it
    XFreePixmap(m_display.get(), cursorPixmap);
}


////////////////////////////////////////////////////////////
void WindowImplX11::cleanup()
{
    // Restore the previous video mode (in case we were running in fullscreen)
    resetVideoMode();

    // Unhide the mouse cursor (in case it was hidden)
    setMouseCursorVisible(true);
}


////////////////////////////////////////////////////////////
bool WindowImplX11::processEvent(XEvent& windowEvent)
{
    using namespace WindowImplX11Impl;

    // Convert the X11 event to a sf::Event
    switch (windowEvent.type)
    {
        // Destroy event
        case DestroyNotify:
        {
            // The window is about to be destroyed: we must cleanup resources
            cleanup();
            break;
        }

        // Gain focus event
        case FocusIn:
        {
            // Update the input context
            if (m_inputContext)
                XSetICFocus(m_inputContext);

            // Grab cursor
            if (m_cursorGrabbed)
            {
                // Try multiple times to grab the cursor
                for (unsigned int trial = 0; trial < maxTrialsCount; ++trial)
                {
                    const int result = XGrabPointer(m_display.get(),
                                                    m_window,
                                                    True,
                                                    None,
                                                    GrabModeAsync,
                                                    GrabModeAsync,
                                                    m_window,
                                                    None,
                                                    CurrentTime);

                    if (result == GrabSuccess)
                    {
                        m_cursorGrabbed = true;
                        break;
                    }

                    // The cursor grab failed, trying again after a small sleep
                    sf::sleep(sf::milliseconds(50));
                }

                if (!m_cursorGrabbed)
                    err() << "Failed to grab mouse cursor" << std::endl;
            }

            pushEvent(Event::FocusGained{});

            // If the window has been previously marked urgent (notification) as a result of a focus request, undo that
            const auto hints = X11Ptr<XWMHints>(XGetWMHints(m_display.get(), m_window));
            if (hints != nullptr)
            {
                // Remove urgency (notification) flag from hints
                hints->flags &= ~XUrgencyHint;
                XSetWMHints(m_display.get(), m_window, hints.get());
            }

            break;
        }

        // Lost focus event
        case FocusOut:
        {
            // Update the input context
            if (m_inputContext)
                XUnsetICFocus(m_inputContext);

            // Release cursor
            if (m_cursorGrabbed)
                XUngrabPointer(m_display.get(), CurrentTime);

            pushEvent(Event::FocusLost{});
            break;
        }

        // Resize event
        case ConfigureNotify:
        {
            // ConfigureNotify can be triggered for other reasons, check if the size has actually changed
            if ((windowEvent.xconfigure.width != m_previousSize.x) || (windowEvent.xconfigure.height != m_previousSize.y))
            {
                pushEvent(Event::Resized{Vector2u(Vector2(windowEvent.xconfigure.width, windowEvent.xconfigure.height))});

                m_previousSize.x = windowEvent.xconfigure.width;
                m_previousSize.y = windowEvent.xconfigure.height;
            }
            break;
        }

        // Close event
        case ClientMessage:
        {
            // Input methods might want random ClientMessage events
            if (!XFilterEvent(&windowEvent, None))
            {
                static const Atom wmProtocols = getAtom("WM_PROTOCOLS");

                // Handle window manager protocol messages we support
                if (windowEvent.xclient.message_type == wmProtocols)
                {
                    static const Atom wmDeleteWindow = getAtom("WM_DELETE_WINDOW");
                    static const Atom netWmPing      = ewmhSupported() ? getAtom("_NET_WM_PING", true) : None;

                    if ((windowEvent.xclient.format == 32) &&
                        (windowEvent.xclient.data.l[0]) == static_cast<long>(wmDeleteWindow))
                    {
                        // Handle the WM_DELETE_WINDOW message
                        pushEvent(Event::Closed{});
                    }
                    else if (netWmPing && (windowEvent.xclient.format == 32) &&
                             (windowEvent.xclient.data.l[0]) == static_cast<long>(netWmPing))
                    {
                        // Handle the _NET_WM_PING message, send pong back to WM to show that we are responsive
                        windowEvent.xclient.window = DefaultRootWindow(m_display.get());

                        XSendEvent(m_display.get(),
                                   DefaultRootWindow(m_display.get()),
                                   False,
                                   SubstructureNotifyMask | SubstructureRedirectMask,
                                   &windowEvent);
                    }
                }
            }

            // Specifications for Xdnd: https://wiki.freedesktop.org/www/Specifications/XDND/

            // Drag and drop position update
            if (windowEvent.xclient.message_type == getAtom("XdndPosition"))
            {
                const Atom xdndStatus = XInternAtom(m_display.get(), "XdndStatus", false);

                XEvent message;
                message.xclient.type         = ClientMessage;
                message.xclient.display      = windowEvent.xclient.display;
                message.xclient.window       = m_dropSource;
                message.xclient.message_type = xdndStatus;
                message.xclient.format       = 32;

                message.xclient.data.l[0] = static_cast<long>(m_window); // The current window

                // Specify if we want the drop or not, and if we want XdndPosition events whenever the mouse moves out of the rectangle
                message.xclient.data.l[1] = (m_acceptedFileType != None);

                // Send back window rectangle coordinates and width
                message.xclient.data.l[2] = 0;
                message.xclient.data.l[3] = 0;

                // Specify action we accept
                message.xclient.data.l[4] = static_cast<long>(getAtom("XdndActionCopy"));

                XSendEvent(m_display.get(), m_dropSource, false, 0, &message);
            }

            if (windowEvent.xclient.message_type == getAtom("XdndEnter"))
            {
                // Store the source window
                m_dropSource = static_cast<::Window>(windowEvent.xclient.data.l[0]);

                m_acceptedFileType = None;

                if (windowEvent.xclient.data.l[1] & 0x1)
                {
                    // There are more than 3 types supported by the source, so we must get the XdndTypeList
                    Atom           actualType       = None;
                    int            actualFormat     = 0;
                    unsigned long  numOfItems       = 0;
                    unsigned long  bytesAfterReturn = 0;
                    unsigned char* data             = nullptr;
                    // Get the list of types that the source supports
                    if (XGetWindowProperty(m_display.get(),
                                           m_dropSource,
                                           getAtom("XdndTypeList"),
                                           0,
                                           1024,
                                           false,
                                           AnyPropertyType,
                                           &actualType,
                                           &actualFormat,
                                           &numOfItems,
                                           &bytesAfterReturn,
                                           &data) == Success)
                    {
                        if (actualType != None)
                        {
                            Atom* supportedAtoms = reinterpret_cast<Atom*>(data);

                            // Go through all of them and check if we support any of them
                            for (int i = 0; i < static_cast<int>(numOfItems); i++)
                            {
                                if (canAcceptFileType(supportedAtoms[i]))
                                {
                                    m_acceptedFileType = supportedAtoms[i];
                                    break;
                                }
                            }
                        }
                    }
                }
                else
                {
                    // Go through the 3 types that the source supports and check if we support any of them
                    for (int i = 2; i < 5; i++)
                    {
                        if (canAcceptFileType(static_cast<Atom>(windowEvent.xclient.data.l[i])))
                        {
                            m_acceptedFileType = static_cast<Atom>(windowEvent.xclient.data.l[i]);
                            break;
                        }
                    }
                }
            }

            // An item has been dropped
            if (windowEvent.xclient.message_type == getAtom("XdndDrop"))
            {
                // Make sure that an acceptable file type was found
                if (m_acceptedFileType != None)
                {
                    // Get the timestamp
                    const auto dropTimestamp = static_cast<::Time>(windowEvent.xclient.data.l[2]);

                    // Get the selection using the given timestamp
                    XConvertSelection(m_display.get(),
                                      getAtom("XdndSelection"),
                                      m_acceptedFileType,
                                      getAtom("XDND_DATA"),
                                      m_window,
                                      dropTimestamp);
                }

                XEvent message;

                message.xclient.type         = ClientMessage;
                message.xclient.display      = m_display.get();
                message.xclient.window       = m_dropSource;
                message.xclient.message_type = getAtom("XdndFinished");
                message.xclient.format       = 32;
                message.xclient.data.l[0]    = static_cast<long>(m_window);
                if (m_acceptedFileType != None)
                {
                    // Tell the application we copied the data
                    message.xclient.data.l[1] = 1;
                    message.xclient.data.l[2] = static_cast<long>(getAtom("XdndActionCopy"));
                }
                else
                {
                    // Tell the application we did nothing
                    message.xclient.data.l[1] = 0;
                    message.xclient.data.l[2] = None;
                }

                XSendEvent(m_display.get(), m_dropSource, false, NoEventMask, &message);

                m_acceptedFileType = None;
                m_dropSource       = 0;
            }

            // The cursor left the window, so make sure we clean up
            if (windowEvent.xclient.message_type == getAtom("XdndLeave"))
            {
                m_acceptedFileType = None;
                m_dropSource       = 0;
            }
            break;
        }

        // Key down event
        case KeyPress:
        {
            // Fill the event parameters
            // TODO: if modifiers are wrong, use XGetModifierMapping to retrieve the actual modifiers mapping
            Event::KeyPressed event;
            event.code     = KeyboardImpl::getKeyFromEvent(windowEvent.xkey);
            event.scancode = KeyboardImpl::getScancodeFromEvent(windowEvent.xkey);
            event.alt      = windowEvent.xkey.state & Mod1Mask;
            event.control  = windowEvent.xkey.state & ControlMask;
            event.shift    = windowEvent.xkey.state & ShiftMask;
            event.system   = windowEvent.xkey.state & Mod4Mask;

            const bool filtered = XFilterEvent(&windowEvent, None);

            // Generate a KeyPressed event if needed
            if (filtered)
            {
                pushEvent(event);
                isKeyFiltered.set(windowEvent.xkey.keycode);
            }
            else
            {
                // Push a KeyPressed event if the key has never been filtered before
                // (a KeyPressed event would have already been pushed if it had been filtered).
                //
                // Some dummy IMs (like the built-in one you get by setting XMODIFIERS=@im=none)
                // never filter events away, and we have to take care of that.
                //
                // In addition, ignore text-only KeyPress events generated by IMs (with keycode set to 0).
                if (!isKeyFiltered.test(windowEvent.xkey.keycode) && windowEvent.xkey.keycode != 0)
                    pushEvent(event);
            }

            // Generate TextEntered events if needed
            if (!filtered)
            {
                if (m_inputContext)
                {
                    Status       status = 0;
                    std::uint8_t keyBuffer[64];

                    const int length = Xutf8LookupString(m_inputContext,
                                                         &windowEvent.xkey,
                                                         reinterpret_cast<char*>(keyBuffer),
                                                         sizeof(keyBuffer),
                                                         nullptr,
                                                         &status);

                    if (status == XBufferOverflow)
                        err() << "A TextEntered event has more than 64 bytes of UTF-8 input, and "
                                 "has been discarded\nThis means either you have typed a very long string "
                                 "(more than 20 chars), or your input method is broken in obscure ways."
                              << std::endl;
                    else if (status == XLookupChars)
                    {
                        // There might be more than 1 characters in this event,
                        // so we must iterate it
                        std::uint32_t unicode = 0;
                        std::uint8_t* iter    = keyBuffer;
                        while (iter < keyBuffer + length)
                        {
                            iter = Utf8::decode(iter, keyBuffer + length, unicode, 0);
                            if (unicode != 0)
                                pushEvent(Event::TextEntered{unicode});
                        }
                    }
                }
                else
                {
                    static XComposeStatus status;
                    char                  keyBuffer[16];
                    if (XLookupString(&windowEvent.xkey, keyBuffer, sizeof(keyBuffer), nullptr, &status))
                        pushEvent(Event::TextEntered{static_cast<std::uint32_t>(keyBuffer[0])});
                }
            }

            updateLastInputTime(windowEvent.xkey.time);

            break;
        }

        // Key up event
        case KeyRelease:
        {
            // Fill the event parameters
            Event::KeyReleased event;
            event.code     = KeyboardImpl::getKeyFromEvent(windowEvent.xkey);
            event.scancode = KeyboardImpl::getScancodeFromEvent(windowEvent.xkey);
            event.alt      = windowEvent.xkey.state & Mod1Mask;
            event.control  = windowEvent.xkey.state & ControlMask;
            event.shift    = windowEvent.xkey.state & ShiftMask;
            event.system   = windowEvent.xkey.state & Mod4Mask;
            pushEvent(event);

            break;
        }

        // Mouse button pressed
        case ButtonPress:
        {
            // Buttons 4 and 5 are the vertical wheel and 6 and 7 the horizontal wheel.
            const unsigned int button = windowEvent.xbutton.button;
            if ((button == Button1) || (button == Button2) || (button == Button3) || (button == 8) || (button == 9))
            {
                Event::MouseButtonPressed event;
                event.position = {windowEvent.xbutton.x, windowEvent.xbutton.y};

                // clang-format off
                switch(button)
                {
                    case Button1: event.button = Mouse::Button::Left;     break;
                    case Button2: event.button = Mouse::Button::Middle;   break;
                    case Button3: event.button = Mouse::Button::Right;    break;
                    case 8:       event.button = Mouse::Button::Extra1;   break;
                    case 9:       event.button = Mouse::Button::Extra2;   break;
                }
                // clang-format on

                pushEvent(event);
            }

            updateLastInputTime(windowEvent.xbutton.time);

            break;
        }

        // Mouse button released
        case ButtonRelease:
        {
            const unsigned int button = windowEvent.xbutton.button;
            if ((button == Button1) || (button == Button2) || (button == Button3) || (button == 8) || (button == 9))
            {
                Event::MouseButtonReleased event;
                event.position = {windowEvent.xbutton.x, windowEvent.xbutton.y};
                switch (button)
                {
                    case Button1:
                        event.button = Mouse::Button::Left;
                        break;
                    case Button2:
                        event.button = Mouse::Button::Middle;
                        break;
                    case Button3:
                        event.button = Mouse::Button::Right;
                        break;
                    case 8:
                        event.button = Mouse::Button::Extra1;
                        break;
                    case 9:
                        event.button = Mouse::Button::Extra2;
                        break;
                }
                pushEvent(event);
            }
            else if ((button == Button4) || (button == Button5))
            {
                Event::MouseWheelScrolled event;
                event.wheel    = Mouse::Wheel::Vertical;
                event.delta    = (button == Button4) ? 1 : -1;
                event.position = {windowEvent.xbutton.x, windowEvent.xbutton.y};
                pushEvent(event);
            }
            else if ((button == 6) || (button == 7))
            {
                Event::MouseWheelScrolled event;
                event.wheel    = Mouse::Wheel::Horizontal;
                event.delta    = (button == 6) ? 1 : -1;
                event.position = {windowEvent.xbutton.x, windowEvent.xbutton.y};
                pushEvent(event);
            }
            break;
        }

        // Mouse moved
        case MotionNotify:
        {
            pushEvent(Event::MouseMoved{{windowEvent.xmotion.x, windowEvent.xmotion.y}});
            break;
        }

        // Mouse entered
        case EnterNotify:
        {
            if (windowEvent.xcrossing.mode == NotifyNormal)
                pushEvent(Event::MouseEntered{});
            break;
        }

        // Mouse left
        case LeaveNotify:
        {
            if (windowEvent.xcrossing.mode == NotifyNormal)
                pushEvent(Event::MouseLeft{});
            break;
        }

        // Keyboard mapping changed
        case MappingNotify:
        {
            if (windowEvent.xmapping.request == MappingKeyboard)
                XRefreshKeyboardMapping(&windowEvent.xmapping);

            break;
        }

        // Window unmapped
        case UnmapNotify:
        {
            if (windowEvent.xunmap.window == m_window)
                m_windowMapped = false;

            break;
        }

        // Window visibility change
        case VisibilityNotify:
        {
            // We prefer using VisibilityNotify over MapNotify because
            // some window managers like awesome don't internally flag a
            // window as viewable even after it is mapped but before it
            // is visible leading to certain function calls failing with
            // an unviewable error if called before VisibilityNotify arrives

            // Empirical testing on most widely used window managers shows
            // that mapping a window will always lead to a VisibilityNotify
            // event that is not VisibilityFullyObscured
            if (windowEvent.xvisibility.window == m_window)
            {
                if (windowEvent.xvisibility.state != VisibilityFullyObscured)
                    m_windowMapped = true;
            }

            break;
        }

        // Window property change
        case PropertyNotify:
        {
            if (!m_lastInputTime)
                m_lastInputTime = windowEvent.xproperty.time;

            break;
        }

        // Raw input
        case GenericEvent:
        {
            if (XGetEventData(m_display.get(), &windowEvent.xcookie))
            {
                if (windowEvent.xcookie.evtype == XI_RawMotion)
                {
                    const auto* rawEvent       = static_cast<const XIRawEvent*>(windowEvent.xcookie.data);
                    int         relativeValueX = 0;
                    int         relativeValueY = 0;

                    // Get relative input values
                    if ((rawEvent->valuators.mask_len > 0) && XIMaskIsSet(rawEvent->valuators.mask, 0))
                        relativeValueX = static_cast<int>(rawEvent->raw_values[0]);

                    if ((rawEvent->valuators.mask_len > 1) && XIMaskIsSet(rawEvent->valuators.mask, 1))
                        relativeValueY = static_cast<int>(rawEvent->raw_values[1]);

                    pushEvent(Event::MouseMovedRaw{{relativeValueX, relativeValueY}});
                }

                XFreeEventData(m_display.get(), &windowEvent.xcookie);
            }

            break;
        }

        // XConvertSelection response
        case SelectionNotify:
        {
            if (windowEvent.xclient.message_type == getAtom("XdndSelection"))
            {
                // Notification that the current selection owner
                // has responded to our request

                Atom           type           = 0;
                int            format         = 0;
                unsigned long  items          = 0;
                unsigned long  remainingBytes = 0;
                unsigned char* data           = nullptr;

                // The selection owner should have written the selection
                // data to the specified window property
                const int result = XGetWindowProperty(m_display.get(),
                                                      m_window,
                                                      windowEvent.xselection.property,
                                                      0,
                                                      0x7fffffff,
                                                      False,
                                                      AnyPropertyType,
                                                      &type,
                                                      &format,
                                                      &items,
                                                      &remainingBytes,
                                                      &data);

                String filenames;

                if (result == Success)
                {
                    // We don't support INCR for now
                    // It is very unlikely that this will be returned
                    // for purely text data transfer anyway
                    if (type != getAtom("INCR", false))
                    {
                        filenames = reinterpret_cast<char*>(data);
                    }

                    XFree(data);

                    // The selection requestor must always delete the property themselves
                    XDeleteProperty(m_display.get(), m_window, windowEvent.xselection.property);
                }

                // Split sf::String into std::vector<sf::String> by the new lines

                std::vector<String> filenamesVector;
                size_t              lastPosition = 0;

                while (filenames.find("\n", lastPosition) != std::string::npos)
                {
                    filenamesVector.push_back(
                        filenames.substring(lastPosition, filenames.find("\n", lastPosition) - lastPosition + 1));

                    lastPosition = filenames.find("\n", lastPosition) + 1;
                }

                if (lastPosition < filenames.getSize())
                {
                    filenamesVector.push_back(filenames.substring(lastPosition, filenames.getSize() - lastPosition));
                }

                for (String& filename : filenamesVector)
                {
                    // To signify that it is giving a file, a program may put file:// at the start, so remove it
                    if (filename.find("file://") == 0)
                    {
                        filename = filename.substring(7, filename.getSize() - 7);
                    }

                    // The last character can be a newline for file lists, so remove it if it is there
                    while (filename[filename.getSize() - 1] == '\n' || filename[filename.getSize() - 1] == '\r')
                    {
                        filename = filename.substring(0, filename.getSize() - 1);
                    }
                }

                pushEvent(Event::FilesDropped{filenamesVector, Mouse::getPosition()});
            }

            break;
        }
    }

    return true;
}


////////////////////////////////////////////////////////////
bool WindowImplX11::checkXRandR()
{
    // Check if the XRandR extension is present
    int version = 0;
    if (!XQueryExtension(m_display.get(), "RANDR", &version, &version, &version))
    {
        err() << "XRandR extension is not supported" << std::endl;
        return false;
    }

    return true;
}


////////////////////////////////////////////////////////////
RROutput WindowImplX11::getOutputPrimary(::Window& rootWindow, XRRScreenResources* res)
{
    const RROutput output = XRRGetOutputPrimary(m_display.get(), rootWindow);

    // Check if returned output is valid, otherwise use the first screen
    if (output == None)
        return res->outputs[0];

    return output;
}


////////////////////////////////////////////////////////////
Vector2i WindowImplX11::getPrimaryMonitorPosition()
{
    Vector2i monitorPosition;

    // Get root window
    ::Window rootWindow = RootWindow(m_display.get(), m_screen);

    // Get the screen resources
    const auto res = X11Ptr<XRRScreenResources>(XRRGetScreenResources(m_display.get(), rootWindow));
    if (!res)
    {
        err() << "Failed to get the current screen resources for primary monitor position" << std::endl;
        return monitorPosition;
    }

    const RROutput output = getOutputPrimary(rootWindow, res.get());

    // Get output info from output
    const auto outputInfo = X11Ptr<XRROutputInfo>(XRRGetOutputInfo(m_display.get(), res.get(), output));
    if (!outputInfo || outputInfo->connection == RR_Disconnected)
    {
        err() << "Failed to get output info for primary monitor position" << std::endl;
        return monitorPosition;
    }

    // Retrieve current RRMode, screen position and rotation
    const auto crtcInfo = X11Ptr<XRRCrtcInfo>(XRRGetCrtcInfo(m_display.get(), res.get(), outputInfo->crtc));
    if (!crtcInfo)
    {
        err() << "Failed to get crtc info for primary monitor position" << std::endl;
        return monitorPosition;
    }

    monitorPosition.x = crtcInfo->x;
    monitorPosition.y = crtcInfo->y;

    return monitorPosition;
}


////////////////////////////////////////////////////////////
void WindowImplX11::setWindowSizeConstraints() const
{
    // Do nothing if resizing is disabled
    if (m_useSizeHints)
        return;

    XSizeHints sizeHints{};
    if (const auto minimumSize = getMinimumSize())
    {
        sizeHints.flags |= PMinSize;
        sizeHints.min_width  = static_cast<int>(minimumSize->x);
        sizeHints.min_height = static_cast<int>(minimumSize->y);
    }
    if (const auto maximumSize = getMaximumSize())
    {
        sizeHints.flags |= PMaxSize;
        sizeHints.max_width  = static_cast<int>(maximumSize->x);
        sizeHints.max_height = static_cast<int>(maximumSize->y);
    }
    XSetWMNormalHints(m_display.get(), m_window, &sizeHints);
}

////////////////////////////////////////////////////////////
void WindowImplX11::setFileDroppingEnabled(bool enabled)
{
    // Xdnd does not work on Wayland, so we check if Wayland is currently active before we enable Xdnd
    // Checking if this exists isn't a perfect solution, as a user could set this
    // in their environment variables, but it's better than crashing

    const char* value = getenv("WAYLAND_DISPLAY");

    // If this variable exists, then that (usually) means that wayland is being used instead of X11, so don't turn on file dropping
    if (value != nullptr)
    {
        // If we are enabling it give it an error, but don't give an error if we are disabling it
        if (enabled)
        {
            sf::err() << "Drag and drop is not supported on Xwayland!" << std::endl;
        }

        return;
    }

    // In order for item dropping to be enabled, the XdndAware property must be set.
    if (enabled)
    {
        Atom xdndVersion = 5;
        XChangeProperty(m_display.get(),
                        m_window,
                        getAtom("XdndAware"),
                        XA_ATOM,
                        32,
                        PropModeReplace,
                        reinterpret_cast<unsigned char*>(&xdndVersion),
                        true);
    }
    else
    {
        XDeleteProperty(m_display.get(), m_window, getAtom("XdndAware"));
    }
}

bool sf::priv::WindowImplX11::canAcceptFileType(const Atom& fileType)
{
    // We currently only accept uri-lists, but this can be changed if you want to add more types to be supported

    // Array of acceptable file types, this is static so we don't get the Atoms every time
    static const std::array<Atom, 1> acceptableFileTypes({
        getAtom("text/uri-list"),
    });

    return std::any_of(acceptableFileTypes.begin(),
                       acceptableFileTypes.end(),
                       [fileType](const Atom& atom) { return atom == fileType; });
}

} // namespace sf::priv
