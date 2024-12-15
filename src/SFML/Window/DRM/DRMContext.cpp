////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2023 Andrew Mickelson
//               2013 Jonathan De Wachter (dewachter.jonathan@gmail.com)
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
#include <SFML/Window/DRM/DRMContext.hpp>
#include <SFML/Window/DRM/WindowImplDRM.hpp>
#include <SFML/Window/DRM/CursorImpl.hpp>
#include <SFML/System/Err.hpp>
#include <SFML/System/Sleep.hpp>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <xf86drm.h>

// We check for this definition in order to avoid multiple definitions of GLAD
// entities during unity builds of SFML.
#ifndef SF_GLAD_EGL_IMPLEMENTATION_INCLUDED
#define SF_GLAD_EGL_IMPLEMENTATION_INCLUDED
#define SF_GLAD_EGL_IMPLEMENTATION
#include <glad/egl.h>
#endif

namespace
{
    struct DrmFb
    {
        gbm_bo* bo;
        sf::Uint32 fbId;
    };

    bool initialized = false;
    sf::priv::Drm drmNode;
    drmEventContext drmEventCtx;
    pollfd pollFD;
    gbm_device* gbmDevice = NULL;
    int contextCount = 0;
    EGLDisplay display = EGL_NO_DISPLAY;
    int waitingForFlip = 0;
    sf::Vector2i mousePos; // current mouse position
    sf::Vector2u mouseBounds;
    bool mouseVisible = false;
    int mouseVAdjust = 0;
    sf::priv::CursorImpl *mouseDefaultCursor = NULL;
    const sf::priv::CursorImpl *mouseUserCursor = NULL;

    static void pageFlipHandler(int fd, unsigned int frame,
        unsigned int sec, unsigned int usec, void* data)
    {
        // suppress unused param warning
        (void)fd, (void)frame, (void)sec, (void)usec;

        int* temp = static_cast<int*>(data);
        *temp = 0;
    }

    static bool waitForFlip(int timeout)
    {
        while (waitingForFlip)
        {
            pollFD.revents = 0;

            if (poll(&pollFD, 1, timeout) < 0)
                return false;

            if (pollFD.revents & (POLLHUP | POLLERR))
                return false;

            if (pollFD.revents & POLLIN)
            {
                drmHandleEvent(drmNode.fileDescriptor, &drmEventCtx);
            }
            else
            {
                return false;
            }
        }
        return true;
    }

    void cleanup()
    {
        if (!initialized)
            return;

        drmModeSetCursor(drmNode.fileDescriptor, drmNode.crtcId, 0, 0, 0 );

        drmModeSetCrtc(drmNode.fileDescriptor,
                       drmNode.originalCrtc->crtc_id,
                       drmNode.originalCrtc->buffer_id,
                       drmNode.originalCrtc->x,
                       drmNode.originalCrtc->y,
                       &drmNode.connectorId,
                       1,
                       &drmNode.originalCrtc->mode);

        drmModeFreeConnector(drmNode.savedConnector);
        drmModeFreeEncoder(drmNode.savedEncoder);
        drmModeFreeCrtc(drmNode.originalCrtc);

        eglTerminate(display);
        display = EGL_NO_DISPLAY;

        if ( mouseDefaultCursor )
        {
            delete mouseDefaultCursor;
            mouseDefaultCursor=NULL;
        }

        gbm_device_destroy(gbmDevice);
        gbmDevice = NULL;

        close(drmNode.fileDescriptor);

        drmNode.fileDescriptor = -1;
        drmNode.mode = 0;

        std::memset(&pollFD, 0, sizeof(pollfd));
        std::memset(&drmEventCtx, 0, sizeof(drmEventContext));

        waitingForFlip = 0;
        mouseVisible = false;
        mouseUserCursor = NULL; // user is responsible for destruction of this
        mouseVAdjust = 0;

        initialized = false;
    }

    void drmFbDestroyCallback(gbm_bo* bo, void* data)
    {
        int drmFd = gbm_device_get_fd(gbm_bo_get_device(bo));
        DrmFb* fb = static_cast<DrmFb*>(data);

        if (fb->fbId)
            drmModeRmFB(drmFd, fb->fbId);

        delete fb;
    }

    DrmFb* drmFbGetFromBo(gbm_bo& bo)
    {
        int drmFd = gbm_device_get_fd(gbm_bo_get_device(&bo));
        DrmFb* fb = static_cast<DrmFb*>(gbm_bo_get_user_data(&bo));
        if (fb)
            return fb;

        fb = new DrmFb();
        fb->bo = &bo;

        const sf::Uint32 width = gbm_bo_get_width(&bo);
        const sf::Uint32 height = gbm_bo_get_height(&bo);
        const sf::Uint32 format = gbm_bo_get_format(&bo);

        sf::Uint32 strides[4] = {0};
        sf::Uint32 handles[4] = {0};
        sf::Uint32 offsets[4] = {0};
        uint64_t modifiers[4] = {0};
        modifiers[0] = gbm_bo_get_modifier(&bo);
        const int num_planes = gbm_bo_get_plane_count(&bo);
        for (int i = 0; i < num_planes; ++i)
        {
            strides[i] = gbm_bo_get_stride_for_plane(&bo, i);
            handles[i] = gbm_bo_get_handle(&bo).u32;
            offsets[i] = gbm_bo_get_offset(&bo, i);
            modifiers[i] = modifiers[0];
        }

        sf::Uint32 flags = 0;
        if (modifiers[0])
        {
            flags = DRM_MODE_FB_MODIFIERS;
        }

        int result = drmModeAddFB2WithModifiers(drmFd, width, height, format, handles, strides, offsets, modifiers, &fb->fbId, flags);

        if (result)
        {
            std::memset(handles, 0, 16);
            handles[0] = gbm_bo_get_handle(&bo).u32;
            std::memset(strides, 0, 16);
            strides[0] = gbm_bo_get_stride(&bo);
            std::memset(offsets, 0, 16);
            result = drmModeAddFB2(drmFd, width, height, format, handles, strides, offsets, &fb->fbId, 0);
        }

        if (result)
        {
            sf::err() << "Failed to create fb: " << std::strerror(errno) << std::endl;
            delete fb;
            return NULL;
        }

        gbm_bo_set_user_data(&bo, fb, drmFbDestroyCallback);

        return fb;
    }

    sf::Uint32 findCrtcForEncoder(const drmModeRes& resources, const drmModeEncoder& encoder)
    {
        for (int i = 0; i < resources.count_crtcs; ++i)
        {
            // Possible_crtcs is a bitmask as described here:
            // https://dvdhrm.wordpress.com/2012/09/13/linux-drm-mode-setting-api
            const sf::Uint32 crtcMask = 1U << i;
            const sf::Uint32 crtcId = resources.crtcs[i];
            if (encoder.possible_crtcs & crtcMask)
            {
                return crtcId;
            }
        }

        // No match found
        return 0;
    }

    sf::Uint32 findCrtcForConnector(const sf::priv::Drm& drm, const drmModeRes& resources, const drmModeConnector& connector)
    {
        for (int i = 0; i < connector.count_encoders; ++i)
        {
            const sf::Uint32 encoderId = connector.encoders[i];
            const drmModeEncoderPtr encoder = drmModeGetEncoder(drm.fileDescriptor, encoderId);

            if (encoder)
            {
                const sf::Uint32 crtcId = findCrtcForEncoder(resources, *encoder);

                drmModeFreeEncoder(encoder);
                if (crtcId != 0)
                {
                    return crtcId;
                }
            }
        }

        // No match found
        return 0;
    }

    int getResources(int fd, drmModeResPtr& resources)
    {
        resources = drmModeGetResources(fd);
        if (resources == NULL)
            return -1;
        return 0;
    }

    int hasMonitorConnected(int fd, drmModeRes& resources)
    {
        drmModeConnectorPtr connector;
        for (int i = 0; i < resources.count_connectors; ++i)
        {
            connector = drmModeGetConnector(fd, resources.connectors[i]);
            if (connector->connection == DRM_MODE_CONNECTED)
            {
                // There is a monitor connected
                drmModeFreeConnector(connector);
                connector = NULL;
                return 1;
            }
            drmModeFreeConnector(connector);
            connector = NULL;
        }
        return 0;
    }

    int findDrmDevice(drmModeResPtr& resources)
    {
        static const int maxDrmDevices = 64;

        drmDevicePtr devices[maxDrmDevices] = { NULL };

        const int numDevices = drmGetDevices2(0, devices, maxDrmDevices);
        if (numDevices < 0)
        {
            sf::err() << "drmGetDevices2 failed: " << std::strerror(-numDevices) << std::endl;
            return -1;
        }

        int fileDescriptor = -1;
        for (int i = 0; i < numDevices; ++i)
        {
            drmDevicePtr device = devices[i];
            int result = 0;

            if (!(device->available_nodes & (1 << DRM_NODE_PRIMARY)))
                continue;
            // OK, it's a primary device. If we can get the drmModeResources, it means it's also a KMS-capable device.
            fileDescriptor = open(device->nodes[DRM_NODE_PRIMARY], O_RDWR);
            if (fileDescriptor < 0)
                continue;
            result = getResources(fileDescriptor, resources);
            if (!result && hasMonitorConnected(fileDescriptor, *resources) != 0)
            {
#ifdef SFML_DEBUG
                sf::err() << "DRM device used: " << device->nodes[DRM_NODE_PRIMARY] << std::endl;
#endif
                break;
            }
            close(fileDescriptor);
            fileDescriptor = -1;
        }
        drmFreeDevices(devices, numDevices);

        if (fileDescriptor < 0)
            sf::err() << "No drm device found!" << std::endl;
        return fileDescriptor;
    }

    int initDrm()
    {
        if (initialized)
            return 0;

        drmModeResPtr resources;

        // Use environment variable "SFML_DRM_DEVICE" (or NULL if not set)
        char* deviceStr = std::getenv("SFML_DRM_DEVICE");
        if (deviceStr && *deviceStr)
        {
            drmNode.fileDescriptor = open(deviceStr, O_RDWR);
            const int ret = getResources(drmNode.fileDescriptor, resources);
            if (ret < 0 && errno == EOPNOTSUPP)
                sf::err() << deviceStr << " does not look like a modeset device" << std::endl;
        }
        else
        {
            drmNode.fileDescriptor = findDrmDevice(resources);
        }

        if (drmNode.fileDescriptor < 0)
        {
            sf::err() << "Could not open drm device" << std::endl;
            return -1;
        }

        if (!resources)
        {
            sf::err() << "drmModeGetResources failed: " << std::strerror(errno) << std::endl;
            return -1;
        }

        // Find a connected connector:
        drmModeConnectorPtr connector = NULL;
        for (int i = 0; i < resources->count_connectors; ++i)
        {
            connector = drmModeGetConnector(drmNode.fileDescriptor, resources->connectors[i]);
            if (connector->connection == DRM_MODE_CONNECTED)
            {
                // It's connected, let's use this!
                break;
            }
            drmModeFreeConnector(connector);
            connector = NULL;
        }

        if (!connector)
        {
            // We could be fancy and listen for hotplug events and wait for a connector..
            sf::err() << "No connected connector!" << std::endl;
            return -1;
        }

        // Find encoder:
        drmModeEncoderPtr encoder = NULL;
        for (int i = 0; i < resources->count_encoders; ++i)
        {
            encoder = drmModeGetEncoder(drmNode.fileDescriptor, resources->encoders[i]);
            if (encoder->encoder_id == connector->encoder_id)
                break;
            drmModeFreeEncoder(encoder);
            encoder = NULL;
        }

        if (encoder)
        {
            drmNode.crtcId = encoder->crtc_id;
        }
        else
        {
            const sf::Uint32 crtcId = findCrtcForConnector(drmNode, *resources, *connector);
            if (crtcId == 0)
            {
                sf::err() << "No crtc found!" << std::endl;
                return -1;
            }

            drmNode.crtcId = crtcId;
        }

        drmModeFreeResources(resources);

        drmNode.connectorId = connector->connector_id;

        drmNode.savedConnector = connector;
        drmNode.savedEncoder = encoder;

        // Get original display mode so we can restore display mode after program exits
        drmNode.originalCrtc = drmModeGetCrtc(drmNode.fileDescriptor, drmNode.crtcId);

        gbmDevice = gbm_create_device(drmNode.fileDescriptor);

        std::atexit(cleanup);
        initialized = true;

        pollFD.fd = drmNode.fileDescriptor;
        pollFD.events = POLLIN;
        drmEventCtx.version = 2;
        drmEventCtx.page_flip_handler = pageFlipHandler;

        drmNode.mode = 0;

        return 1;
    }

    void setDrmMode( unsigned int width=0, unsigned int height=0 )
    {
        // don't do anything if supplied width and height are 0 and we already have a drm mode
        if (( width == 0 ) && drmNode.mode )
            return;

        drmModeConnectorPtr connector = drmNode.savedConnector;
        if (!connector)
            return;

        // Find user requested mode:
        // Use environment variable "SFML_DRM_MODE" (or NULL if not set)
        char* modeString = std::getenv("SFML_DRM_MODE");

        // Use environment variable "SFML_DRM_REFRESH" (or 0 if not set)
        // Use in combination with mode to request specific refresh rate for the mode
        // if multiple refresh rates for same mode might be supported
        unsigned int refreshRate = 0;
        char* refreshString = std::getenv("SFML_DRM_REFRESH");

        if (refreshString)
            refreshRate = static_cast<unsigned int>(atoi(refreshString));

        bool matched = false;
        for (int i = 0; i < connector->count_modes; ++i)
        {
            drmModeModeInfoPtr currentMode = &connector->modes[i];

            if (refreshRate == 0 || currentMode->vrefresh == refreshRate)
            {
                // prefer SFML_DRM_MODE setting if matched
                if ((modeString && *modeString) && (std::strcmp(currentMode->name, modeString) == 0))
                {
                    drmNode.mode = currentMode;
                    break;
                }
                // otherwise match to program supplied width and height
                if (!matched && ( currentMode->hdisplay == width ) && ( currentMode->vdisplay == height ))
                {
                    drmNode.mode = currentMode;
                    matched=true;
                }
            }
        }
        if ((modeString && *modeString) && !drmNode.mode)
            sf::err() << "SFML_DRM_MODE (" << modeString << ") not found, using default mode!" << std::endl;

        // Let's use the current mode rather than the preferred one if the user didn't specify a mode with env vars
        if (!drmNode.mode)
            drmNode.mode = &(drmNode.originalCrtc->mode);

#ifdef SFML_DEBUG
        sf::err() << "DRM Mode used: " << drmNode.mode->name << "@" << drmNode.mode->vrefresh << std::endl;
#endif
    }

    EGLDisplay getInitializedDisplay()
    {
        if (display == EGL_NO_DISPLAY)
        {
            gladLoaderLoadEGL(EGL_NO_DISPLAY);

            eglCheck(display = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(gbmDevice)));

            EGLint major, minor;
            eglCheck(eglInitialize(display, &major, &minor));

            gladLoaderLoadEGL(display);

#if defined(SFML_OPENGL_ES)
            if (!eglBindAPI(EGL_OPENGL_ES_API))
            {
                sf::err() << "failed to bind api EGL_OPENGL_ES_API" << std::endl;
            }
#else
            if (!eglBindAPI(EGL_OPENGL_API))
            {
                sf::err() << "failed to bind api EGL_OPENGL_API" << std::endl;
            }
#endif
        }

        return display;
    }
}


namespace sf
{
namespace priv
{
////////////////////////////////////////////////////////////
DRMContext::DRMContext(DRMContext* shared) :
m_display    (EGL_NO_DISPLAY),
m_context    (EGL_NO_CONTEXT),
m_surface    (EGL_NO_SURFACE),
m_config     (NULL),
m_currentBO  (NULL),
m_nextBO     (NULL),
m_gbmSurface (NULL),
m_shown      (false),
m_scanOut    (false)
{
    contextCount++;
    if (initDrm() < 0)
        return;

    setDrmMode();

    // Get the initialized EGL display
    m_display = getInitializedDisplay();

    // Get the best EGL config matching the default video settings
    m_config = getBestConfig(m_display, ContextSettings());
    updateSettings();

    // Create EGL context
    createContext(shared);

    if (shared)
        createSurface(drmNode.mode->hdisplay, drmNode.mode->vdisplay, false);
    else // create a surface to force the GL to initialize (seems to be required for glGetString() etc )
        createSurface(1, 1, false);
}


////////////////////////////////////////////////////////////
DRMContext::DRMContext(DRMContext* shared, const ContextSettings& settings, const WindowImpl* owner, unsigned int /*bitsPerPixel*/) :
m_display    (EGL_NO_DISPLAY),
m_context    (EGL_NO_CONTEXT),
m_surface    (EGL_NO_SURFACE),
m_config     (NULL),
m_currentBO  (NULL),
m_nextBO     (NULL),
m_gbmSurface (NULL),
m_shown      (false),
m_scanOut    (false)
{
    contextCount++;

    if (initDrm() < 0)
        return;

    Vector2u size;
    if (owner)
        size = owner->getSize();

    setDrmMode(size.x, size.y);

    // Get the initialized EGL display
    m_display = getInitializedDisplay();

    // Get the best EGL config matching the requested video settings
    m_config = getBestConfig(m_display, settings);
    updateSettings();

    // Create EGL context
    createContext(shared);

    if (owner)
        createSurface(drmNode.mode->hdisplay, drmNode.mode->vdisplay, true);
}


////////////////////////////////////////////////////////////
DRMContext::DRMContext(DRMContext* shared, const ContextSettings& settings, unsigned int /*width*/, unsigned int /*height*/) :
m_display    (EGL_NO_DISPLAY),
m_context    (EGL_NO_CONTEXT),
m_surface    (EGL_NO_SURFACE),
m_config     (NULL),
m_currentBO  (NULL),
m_nextBO     (NULL),
m_gbmSurface (NULL),
m_shown      (false),
m_scanOut    (false)
{
    contextCount++;
    if (initDrm() < 0)
        return;

    setDrmMode();

    // Get the initialized EGL display
    m_display = getInitializedDisplay();

    // Get the best EGL config matching the requested video settings
    m_config = getBestConfig(m_display, settings);
    updateSettings();

    // Create EGL context
    createContext(shared);
    createSurface(drmNode.mode->hdisplay, drmNode.mode->vdisplay, false);
}


////////////////////////////////////////////////////////////
DRMContext::~DRMContext()
{
    // Deactivate the current context
    EGLContext currentContext;
    eglCheck(currentContext = eglGetCurrentContext());

    if (currentContext == m_context)
    {
        eglCheck(eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
    }

    // Destroy context
    if (m_context != EGL_NO_CONTEXT)
    {
        eglCheck(eglDestroyContext(m_display, m_context));
        m_context = EGL_NO_CONTEXT;
    }

    // Destroy surface
    if (m_surface != EGL_NO_SURFACE)
    {
        eglCheck(eglDestroySurface(m_display, m_surface));
        m_surface = EGL_NO_SURFACE;
    }

    if (m_currentBO)
        gbm_surface_release_buffer(m_gbmSurface, m_currentBO);

    if (m_nextBO)
        gbm_surface_release_buffer(m_gbmSurface, m_nextBO);

    if (m_gbmSurface)
        gbm_surface_destroy(m_gbmSurface);

    contextCount--;
    if (contextCount == 0)
        cleanup();
}


////////////////////////////////////////////////////////////
bool DRMContext::makeCurrent(bool current)
{
    const EGLSurface surface = current ? m_surface : EGL_NO_SURFACE;
    const EGLContext context = current ? m_context : EGL_NO_CONTEXT;
    return m_surface != EGL_NO_SURFACE && eglMakeCurrent(m_display, surface, surface, context);
}


////////////////////////////////////////////////////////////
void DRMContext::display()
{
    if (m_surface == EGL_NO_SURFACE)
        return;

    if (!m_scanOut)
    {
        eglCheck(eglSwapBuffers(m_display, m_surface));
        return;
    }

    // Handle display of buffer to the screen
    DrmFb* fb = NULL;

    if (!waitForFlip(-1))
        return;

    if (m_currentBO)
    {
        gbm_surface_release_buffer(m_gbmSurface, m_currentBO);
        m_currentBO = NULL;
    }

    eglCheck(eglSwapBuffers(m_display, m_surface));

    m_currentBO = m_nextBO;

    // This call must be preceeded by a single call to eglSwapBuffers()
    m_nextBO = gbm_surface_lock_front_buffer(m_gbmSurface);

    if (!m_nextBO)
        return;

    fb = drmFbGetFromBo(*m_nextBO);
    if (!fb)
    {
        err() << "Failed to get FB from buffer object" << std::endl;
        return;
    }

    // If first time, need to first call drmModeSetCrtc()
    if (!m_shown)
    {
        if (drmModeSetCrtc(drmNode.fileDescriptor, drmNode.crtcId, fb->fbId, 0, 0, &drmNode.connectorId, 1, drmNode.mode))
        {
            err() << "Failed to set mode: " << std::strerror(errno) << std::endl;
            std::abort();
        }
        m_shown = true;
    }

    // Do page flip
    if (!drmModePageFlip(drmNode.fileDescriptor, drmNode.crtcId, fb->fbId, DRM_MODE_PAGE_FLIP_EVENT, &waitingForFlip))
        waitingForFlip = 1;
}


////////////////////////////////////////////////////////////
void DRMContext::setVerticalSyncEnabled(bool enabled)
{
    eglCheck(eglSwapInterval(m_display, enabled ? 1 : 0));
}


////////////////////////////////////////////////////////////
void DRMContext::createContext(DRMContext* shared)
{
    const EGLint contextVersion[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 1,
        EGL_NONE
    };

    EGLContext toShared;

    if (shared)
        toShared = shared->m_context;
    else
        toShared = EGL_NO_CONTEXT;

    if (toShared != EGL_NO_CONTEXT)
        eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    // Create EGL context
    eglCheck(m_context = eglCreateContext(m_display, m_config, toShared, contextVersion));
    if (m_context == EGL_NO_CONTEXT)
        err() << "Failed to create EGL context" << std::endl;
}


////////////////////////////////////////////////////////////
void DRMContext::createSurface(unsigned int width, unsigned int height, bool scanout)
{
    sf::Uint32 flags = GBM_BO_USE_RENDERING;
    uint32_t fmt = GBM_FORMAT_ARGB8888;

    m_scanOut = scanout;
    if (m_scanOut)
        flags |= GBM_BO_USE_SCANOUT;

    if (!gbm_device_is_format_supported(gbmDevice, fmt, flags))
        err() << "Warning: GBM surface format not supported." << std::endl;

    m_gbmSurface = gbm_surface_create(
        gbmDevice,
        width,
        height,
        fmt,
        flags);

    if (!m_gbmSurface)
    {
        err() << "Failed to create gbm surface." << std::endl;
        return;
    }

    eglCheck(m_surface = eglCreateWindowSurface(m_display, m_config, reinterpret_cast<EGLNativeWindowType>(m_gbmSurface), NULL));

    if (m_surface == EGL_NO_SURFACE)
    {
        err() << "Failed to create EGL Surface" << std::endl;
    }
}


////////////////////////////////////////////////////////////
void DRMContext::destroySurface()
{
    eglCheck(eglDestroySurface(m_display, m_surface));
    m_surface = EGL_NO_SURFACE;

    gbm_surface_destroy(m_gbmSurface);
    m_gbmSurface = NULL;

    // Ensure that this context is no longer active since our surface is now destroyed
    setActive(false);
}


////////////////////////////////////////////////////////////
EGLConfig DRMContext::getBestConfig(EGLDisplay display, const ContextSettings& settings)
{
    // Set our video settings constraint
    const EGLint attributes[] =
    {
        EGL_DEPTH_SIZE, static_cast<EGLint>(settings.depthBits),
        EGL_STENCIL_SIZE, static_cast<EGLint>(settings.stencilBits),
        EGL_SAMPLE_BUFFERS, static_cast<EGLint>(settings.antialiasingLevel),
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,

        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
#if defined(SFML_OPENGL_ES)
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
#else
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
#endif
        EGL_NONE
    };

    EGLint configCount;
    EGLConfig configs[1];

    // Ask EGL for the best config matching our video settings
    eglCheck(eglChooseConfig(display, attributes, configs, 1, &configCount));

    return configs[0];
}


////////////////////////////////////////////////////////////
void DRMContext::updateSettings()
{
    EGLint tmp;

    // Update the internal context settings with the current config
    eglCheck(eglGetConfigAttrib(m_display, m_config, EGL_DEPTH_SIZE, &tmp));
    m_settings.depthBits = static_cast<unsigned int>(tmp);

    eglCheck(eglGetConfigAttrib(m_display, m_config, EGL_STENCIL_SIZE, &tmp));
    m_settings.stencilBits = static_cast<unsigned int>(tmp);

    eglCheck(eglGetConfigAttrib(m_display, m_config, EGL_SAMPLES, &tmp));
    m_settings.antialiasingLevel = static_cast<unsigned int>(tmp);

    m_settings.majorVersion = 1;
    m_settings.minorVersion = 1;
    m_settings.attributeFlags = ContextSettings::Default;
}


////////////////////////////////////////////////////////////
GlFunctionPointer DRMContext::getFunction(const char* name)
{
    return reinterpret_cast<GlFunctionPointer>(eglGetProcAddress(name));
}


////////////////////////////////////////////////////////////
Drm& DRMContext::getDRM()
{
    initDrm();
    return drmNode;
}

gbm_device *DRMContext::getGbmDevice()
{
    initDrm();
    return gbmDevice;
}

sf::Vector2i DRMContext::getCursorPos()
{
    sf::Vector2i temp=mousePos;
    temp.y -= mouseVAdjust;
    return temp;
}

void DRMContext::setCursorPos( const sf::Vector2i &pos )
{
    sf::Vector2i temp=pos;

    if (temp.x > static_cast<int>(mouseBounds.x))
       temp.x = static_cast<int>(mouseBounds.x);
    else if (temp.x < 0)
        temp.x=0;

    temp.y += mouseVAdjust;

    if (temp.y > static_cast<int>(mouseBounds.y) + mouseVAdjust)
       temp.y = static_cast<int>(mouseBounds.y) + mouseVAdjust;
    else if (temp.y < mouseVAdjust)
        temp.y=mouseVAdjust;

    mousePos=temp;
    drmModeMoveCursor( drmNode.fileDescriptor, drmNode.crtcId, mousePos.x, mousePos.y );
}

void DRMContext::setCursorBounds( const sf::Vector2u &bound )
{
    sf::Vector2u temp=bound;
    mouseVAdjust=0;
    if (drmNode.mode)
    {
        if (temp.x > drmNode.mode->hdisplay)
           temp.x = drmNode.mode->hdisplay;
        if (temp.y > drmNode.mode->vdisplay)
           temp.y = drmNode.mode->vdisplay;

        mouseVAdjust=static_cast<int>(drmNode.mode->vdisplay - temp.y);
    }

    mouseBounds=temp;
    setCursorPos( getCursorPos() );
}

void DRMContext::setCursorVisible( bool vis )
{
    bool update = ( vis != mouseVisible );
    mouseVisible = vis;

    if (update)
        updateCursorDisplay();
}

void DRMContext::setUserCursor( const CursorImpl &cur )
{
    bool update = mouseVisible && ( mouseUserCursor != &cur );
    mouseUserCursor = &cur;

    if (update)
        updateCursorDisplay();
}

void DRMContext::updateCursorDisplay()
{
    const CursorImpl *p = mouseUserCursor;
    if (!p || !p->m_bo)
    {
        if (!mouseDefaultCursor)
        {
            mouseDefaultCursor = new CursorImpl();
            if (!mouseDefaultCursor->loadFromSystem(sf::Cursor::Type::Arrow))
            {
                sf::err() << "Error loading default cursor." << std::endl;
            }
        }
        p=mouseDefaultCursor;
    }

    int ret;

    if (!(p->m_bo) || !mouseVisible)
    {
        ret = drmModeSetCursor(drmNode.fileDescriptor, drmNode.crtcId, 0, 0, 0 );
    }
    else
    {
        setCursorPos(getCursorPos());
        ret = drmModeSetCursor2(drmNode.fileDescriptor, drmNode.crtcId, gbm_bo_get_handle(p->m_bo).u32,
                        gbm_bo_get_width(p->m_bo), gbm_bo_get_height(p->m_bo), static_cast<int32_t>(p->m_hotspot.x),
                        static_cast<int32_t>(p->m_hotspot.y));
    }

    if (ret)
    {
        sf::err() << "drmModeSetCursor failed to set cursor: " << strerror(errno) << std::endl;
    }
}


} // namespace priv

} // namespace sf
