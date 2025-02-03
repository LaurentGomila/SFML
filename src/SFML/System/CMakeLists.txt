set(INCROOT ${PROJECT_SOURCE_DIR}/include/SFML/System)
set(SRCROOT ${PROJECT_SOURCE_DIR}/src/SFML/System)

# all source files
set(SRC
    ${INCROOT}/Angle.hpp
    ${INCROOT}/Angle.inl
    ${SRCROOT}/Clock.cpp
    ${INCROOT}/Clock.hpp
    ${SRCROOT}/EnumArray.hpp
    ${SRCROOT}/Err.cpp
    ${INCROOT}/Err.hpp
    ${INCROOT}/Exception.hpp
    ${INCROOT}/Export.hpp
    ${INCROOT}/InputStream.hpp
    ${INCROOT}/NativeActivity.hpp
    ${SRCROOT}/Sleep.cpp
    ${INCROOT}/Sleep.hpp
    ${SRCROOT}/String.cpp
    ${INCROOT}/String.hpp
    ${INCROOT}/String.inl
    ${INCROOT}/Time.hpp
    ${INCROOT}/Time.inl
    ${INCROOT}/Utf.hpp
    ${INCROOT}/Utf.inl
    ${SRCROOT}/Utils.hpp
    ${SRCROOT}/Utils.cpp
    ${SRCROOT}/Vector2.cpp
    ${INCROOT}/Vector2.hpp
    ${INCROOT}/Vector2.inl
    ${SRCROOT}/Vector3.cpp
    ${INCROOT}/Vector3.hpp
    ${INCROOT}/Vector3.inl
    ${SRCROOT}/FileInputStream.cpp
    ${INCROOT}/FileInputStream.hpp
    ${SRCROOT}/MemoryInputStream.cpp
    ${INCROOT}/MemoryInputStream.hpp
    ${INCROOT}/SuspendAwareClock.hpp
)
source_group("" FILES ${SRC})

# add platform specific sources
if(SFML_OS_WINDOWS)
    set(PLATFORM_SRC
        ${SRCROOT}/Win32/SleepImpl.cpp
        ${SRCROOT}/Win32/SleepImpl.hpp
    )
    source_group("windows" FILES ${PLATFORM_SRC})
else()
    set(PLATFORM_SRC
        ${SRCROOT}/Unix/SleepImpl.cpp
        ${SRCROOT}/Unix/SleepImpl.hpp
    )

    if(SFML_OS_ANDROID)
        list(APPEND PLATFORM_SRC
            ${SRCROOT}/Android/Activity.hpp
            ${SRCROOT}/Android/Activity.cpp
            ${SRCROOT}/Android/NativeActivity.cpp
            ${SRCROOT}/Android/ResourceStream.cpp
            ${SRCROOT}/Android/ResourceStream.cpp
            ${SRCROOT}/Android/SuspendAwareClock.cpp
        )
    endif()

    source_group("unix" FILES ${PLATFORM_SRC})
endif()

find_package(Threads REQUIRED)

# define the sfml-system target
sfml_add_library(System
                 SOURCES ${SRC} ${PLATFORM_SRC}
                 DEPENDENCIES "Dependencies.cmake.in")

# enable precompiled headers
if (SFML_ENABLE_PCH)
    message(VERBOSE "enabling PCH for SFML library 'sfml-system' (reused as the PCH for other SFML libraries)")
    target_precompile_headers(sfml-system PRIVATE "${PROJECT_SOURCE_DIR}/src/SFML/PCH.hpp")
endif()

if(SFML_OS_ANDROID)
    # glad sources
    target_include_directories(sfml-system SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/extlibs/headers/glad/include")
endif()

target_link_libraries(sfml-system PRIVATE Threads::Threads)

if(SFML_OS_LINUX)
    target_link_libraries(sfml-system PRIVATE rt)
elseif(SFML_OS_WINDOWS)
    target_link_libraries(sfml-system PRIVATE winmm)
elseif(SFML_OS_ANDROID)
    target_link_libraries(sfml-system PRIVATE android log)
endif()
