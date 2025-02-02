set(INCROOT ${PROJECT_SOURCE_DIR}/include/SFML/Audio)
set(SRCROOT ${PROJECT_SOURCE_DIR}/src/SFML/Audio)

# all source files
set(SRC
    ${SRCROOT}/AudioResource.cpp
    ${INCROOT}/AudioResource.hpp
    ${SRCROOT}/AudioDevice.cpp
    ${SRCROOT}/AudioDevice.hpp
    ${INCROOT}/Export.hpp
    ${SRCROOT}/Listener.cpp
    ${INCROOT}/Listener.hpp
    ${SRCROOT}/Miniaudio.cpp
    ${SRCROOT}/MiniaudioUtils.hpp
    ${SRCROOT}/MiniaudioUtils.cpp
    ${SRCROOT}/Music.cpp
    ${INCROOT}/Music.hpp
    ${SRCROOT}/PlaybackDevice.cpp
    ${INCROOT}/PlaybackDevice.hpp
    ${SRCROOT}/Sound.cpp
    ${INCROOT}/Sound.hpp
    ${SRCROOT}/SoundBuffer.cpp
    ${INCROOT}/SoundBuffer.hpp
    ${SRCROOT}/SoundBufferRecorder.cpp
    ${INCROOT}/SoundBufferRecorder.hpp
    ${INCROOT}/SoundChannel.hpp
    ${SRCROOT}/InputSoundFile.cpp
    ${INCROOT}/InputSoundFile.hpp
    ${SRCROOT}/OutputSoundFile.cpp
    ${INCROOT}/OutputSoundFile.hpp
    ${SRCROOT}/SoundRecorder.cpp
    ${INCROOT}/SoundRecorder.hpp
    ${SRCROOT}/SoundSource.cpp
    ${INCROOT}/SoundSource.hpp
    ${SRCROOT}/SoundStream.cpp
    ${INCROOT}/SoundStream.hpp
)
source_group("" FILES ${SRC})

set(CODECS_SRC
    ${SRCROOT}/SoundFileFactory.cpp
    ${INCROOT}/SoundFileFactory.hpp
    ${INCROOT}/SoundFileFactory.inl
    ${INCROOT}/SoundFileReader.hpp
    ${SRCROOT}/SoundFileReaderFlac.hpp
    ${SRCROOT}/SoundFileReaderFlac.cpp
    ${SRCROOT}/SoundFileReaderMp3.hpp
    ${SRCROOT}/SoundFileReaderMp3.cpp
    ${SRCROOT}/SoundFileReaderOgg.hpp
    ${SRCROOT}/SoundFileReaderOgg.cpp
    ${SRCROOT}/SoundFileReaderOpus.hpp
    ${SRCROOT}/SoundFileReaderOpus.cpp
    ${SRCROOT}/SoundFileReaderWav.hpp
    ${SRCROOT}/SoundFileReaderWav.cpp
    ${INCROOT}/SoundFileWriter.hpp
    ${SRCROOT}/SoundFileWriterFlac.hpp
    ${SRCROOT}/SoundFileWriterFlac.cpp
    ${SRCROOT}/SoundFileWriterOgg.hpp
    ${SRCROOT}/SoundFileWriterOgg.cpp
    ${SRCROOT}/SoundFileWriterOpus.hpp
    ${SRCROOT}/SoundFileWriterOpus.cpp
    ${SRCROOT}/SoundFileWriterWav.hpp
    ${SRCROOT}/SoundFileWriterWav.cpp
)
source_group("codecs" FILES ${CODECS_SRC})

# Ensure certain files are compiled as Objective-C++
# See: https://miniaud.io/docs/manual/index.html#Building
if(SFML_OS_IOS)
    enable_language(OBJCXX)
    set_source_files_properties(${SRCROOT}/Miniaudio.cpp PROPERTIES LANGUAGE OBJCXX)
endif()

# find external libraries
if(SFML_USE_SYSTEM_DEPS)
    find_package(Vorbis REQUIRED)
    find_package(FLAC REQUIRED)
else()
    # use an immediately invoked function to scope option variables we have to set
    function(sfml_add_audio_dependencies)
        include(FetchContent)

        # remember whether we are building SFML as a shared library
        if(BUILD_SHARED_LIBS)
            set(SFML_BUILD_SHARED_LIBS ON)
        endif()

        set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
        set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
        set(BUILD_SHARED_LIBS OFF)
        set(BUILD_CXXLIBS OFF)
        set(BUILD_PROGRAMS OFF)
        set(BUILD_EXAMPLES OFF)
        set(BUILD_TESTING OFF)
        set(BUILD_UTILS OFF)
        set(BUILD_DOCS OFF)
        set(INSTALL_MANPAGES OFF)
        set(INSTALL_DOCS OFF)
        set(INSTALL_PKG_CONFIG_MODULE OFF)
        set(INSTALL_PKGCONFIG_MODULES OFF)
        set(OPUS_INSTALL_PKG_CONFIG_MODULE OFF)
        set(WITH_FORTIFY_SOURCE OFF)
        set(WITH_STACK_PROTECTOR OFF)
        set(WITH_AVX OFF) # LLVM/Clang on Windows has issues with AVX2
        SET(OP_DISABLE_HTTP ON)
        SET(OP_DISABLE_EXAMPLES ON)
        SET(OP_DISABLE_DOCS ON)

        FetchContent_Declare(opus
            GIT_REPOSITORY https://github.com/xiph/opus.git
            GIT_TAG v1.5.2
            GIT_SHALLOW ON
            # patch out parts we don't want of the Opus CMake configuration
            # - feature summary
            # - installing headers
            # - add CMAKE_DEBUG_POSTFIX
            PATCH_COMMAND ${CMAKE_COMMAND} -DOPUS_DIR=${FETCHCONTENT_BASE_DIR}/opus-src -P ${PROJECT_SOURCE_DIR}/tools/opus/PatchOpus.cmake)
        FetchContent_Declare(ogg
            GIT_REPOSITORY https://github.com/xiph/ogg.git
            GIT_TAG v1.3.5
            GIT_SHALLOW ON
            # patch out parts we don't want of the Ogg CMake configuration
            # - installing headers & pkgconfig files
            # - add CMAKE_DEBUG_POSTFIX
            PATCH_COMMAND ${CMAKE_COMMAND} -DOGG_DIR=${FETCHCONTENT_BASE_DIR}/ogg-src -P ${PROJECT_SOURCE_DIR}/tools/ogg/PatchOgg.cmake)
        FetchContent_Declare(opusfile
            GIT_REPOSITORY https://github.com/xiph/opusfile.git
            GIT_TAG 9d718345ce03b2fad5d7d28e0bcd1cc69ab2b166
            # patch out parts we don't want of the Opusfile CMake configuration
            # - installing headers
            # - add CMAKE_DEBUG_POSTFIX
            PATCH_COMMAND ${CMAKE_COMMAND} -DOPUSFILE_DIR=${FETCHCONTENT_BASE_DIR}/opusfile-src -P ${PROJECT_SOURCE_DIR}/tools/opus/PatchOpusfile.cmake)
        FetchContent_Declare(flac
            GIT_REPOSITORY https://github.com/xiph/flac.git
            GIT_TAG 1.4.3
            GIT_SHALLOW ON
            # patch out parts we don't want of the FLAC CMake configuration:
            # - adding unnecessary libraries that aren't even used
            # - installing headers & pkgconfig files
            # - add CMAKE_DEBUG_POSTFIX
            PATCH_COMMAND ${CMAKE_COMMAND} -DFLAC_DIR=${FETCHCONTENT_BASE_DIR}/flac-src -P ${PROJECT_SOURCE_DIR}/tools/flac/PatchFLAC.cmake)
        FetchContent_Declare(vorbis
            GIT_REPOSITORY https://github.com/xiph/vorbis.git
            GIT_TAG v1.3.7
            GIT_SHALLOW ON
            # patch out parts we don't want of the Vorbis CMake configuration:
            # - Vorbis doesn't check if the Ogg::ogg target exists before calling find_package
            # - installing headers & pkgconfig files
            # - add CMAKE_DEBUG_POSTFIX
            PATCH_COMMAND ${CMAKE_COMMAND} -DVORBIS_DIR=${FETCHCONTENT_BASE_DIR}/vorbis-src -P ${PROJECT_SOURCE_DIR}/tools/vorbis/PatchVorbis.cmake)
        FetchContent_MakeAvailable(opus ogg opusfile flac vorbis)

        set_target_properties(opus ogg opusfile FLAC vorbis vorbisenc vorbisfile PROPERTIES FOLDER "Dependencies")

        # if building SFML as a shared library and linking our dependencies in
        # as static libraries we need to build them with -fPIC
        if(SFML_BUILD_SHARED_LIBS)
            set_target_properties(opus ogg opusfile FLAC vorbis vorbisenc vorbisfile PROPERTIES POSITION_INDEPENDENT_CODE ON)
        endif()

        # disable building dependencies as part of a unity build, they don't support it
        set_target_properties(opus ogg opusfile FLAC vorbis vorbisenc vorbisfile PROPERTIES UNITY_BUILD OFF)

        sfml_set_stdlib(opus)
        sfml_set_stdlib(ogg)
        sfml_set_stdlib(opusfile)
        sfml_set_stdlib(FLAC)
        sfml_set_stdlib(vorbis)
        sfml_set_stdlib(vorbisenc)
        sfml_set_stdlib(vorbisfile)

        # define NDEBUG when building FLAC to suppress console debug output
        target_compile_definitions(FLAC PRIVATE NDEBUG)

        # _FILE_OFFSET_BITS=64 is not supported with older (<24) Android API levels
        if(SFML_OS_ANDROID)
            target_compile_definitions(FLAC PRIVATE _FILE_OFFSET_BITS=32)
        endif()

        # aliases were introduced only after 1.3.7 was released
        add_library(Vorbis::vorbis ALIAS vorbis)
        add_library(Vorbis::vorbisenc ALIAS vorbisenc)
        add_library(Vorbis::vorbisfile ALIAS vorbisfile)
    endfunction()
    sfml_add_audio_dependencies()
endif()

find_package(Threads REQUIRED)

# define the sfml-audio target
sfml_add_library(Audio
                 SOURCES ${SRC} ${CODECS_SRC}
                 DEPENDENCIES "Dependencies.cmake.in")

# avoids warnings in vorbisfile.h
target_compile_definitions(sfml-audio PRIVATE OV_EXCLUDE_STATIC_CALLBACKS FLAC__NO_DLL)

# disable miniaudio features we do not use
target_compile_definitions(sfml-audio PRIVATE MA_NO_MP3 MA_NO_FLAC MA_NO_ENCODING MA_NO_RESOURCE_MANAGER MA_NO_GENERATION)

# use standard fixed-width integer types
target_compile_definitions(sfml-audio PRIVATE MA_USE_STDINT)

# detect the endianness as required by Ogg
target_compile_definitions(sfml-audio PRIVATE SFML_IS_BIG_ENDIAN=$<STREQUAL:${CMAKE_CXX_BYTE_ORDER},BIG_ENDIAN>)

# setup dependencies
target_link_libraries(sfml-audio
                      PUBLIC SFML::System
                      PRIVATE Vorbis::vorbis Vorbis::vorbisfile Vorbis::vorbisenc FLAC::FLAC OpusFile::opusfile Threads::Threads)
if(SFML_OS_IOS)
    target_link_libraries(sfml-audio PRIVATE "-framework Foundation" "-framework CoreFoundation" "-framework CoreAudio" "-framework AudioToolbox" "-framework AVFoundation")
endif()

# miniaudio sources
target_include_directories(sfml-audio SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/extlibs/headers/miniaudio")

# minimp3 sources
target_include_directories(sfml-audio SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/extlibs/headers/minimp3")

if(SFML_OS_ANDROID)
    target_link_libraries(sfml-audio PRIVATE android OpenSLES)
endif()

if(SFML_OS_LINUX)
    target_link_libraries(sfml-audio PRIVATE dl)
endif()
