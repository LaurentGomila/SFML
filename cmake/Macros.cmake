include(CMakeParseArguments)

# include the compiler warnings helpers
include(${CMAKE_CURRENT_LIST_DIR}/CompilerWarnings.cmake)

# helper function to tweak visibility of public symbols
function(set_public_symbols_hidden target)
    # ensure public symbols are hidden by default (exported ones are explicitly marked)
    set_target_properties(${target} PROPERTIES
                          CXX_VISIBILITY_PRESET hidden
                          VISIBILITY_INLINES_HIDDEN YES)
endfunction()

# This little macro lets you set any Xcode specific property
macro(sfml_set_xcode_property TARGET XCODE_PROPERTY XCODE_VALUE)
    set_property(TARGET ${TARGET} PROPERTY XCODE_ATTRIBUTE_${XCODE_PROPERTY} ${XCODE_VALUE})
endmacro()

# set the appropriate standard library on each platform for the given target
# example: sfml_set_stdlib(sfml-system)
function(sfml_set_stdlib target)
    # for gcc on Windows, apply the SFML_USE_STATIC_STD_LIBS option if it is enabled
    if(SFML_OS_WINDOWS)
        if(SFML_COMPILER_GCC)
            if(SFML_USE_STATIC_STD_LIBS AND NOT SFML_COMPILER_GCC_TDM)
                target_link_libraries(${target} PRIVATE "-static-libgcc" "-static-libstdc++")
            elseif(NOT SFML_USE_STATIC_STD_LIBS AND SFML_COMPILER_GCC_TDM)
                target_link_libraries(${target} PRIVATE "-shared-libgcc" "-shared-libstdc++")
            endif()
        elseif(SFML_COMPILER_MSVC)
            if(SFML_USE_STATIC_STD_LIBS)
                set_property(TARGET ${target} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
            endif()
        endif()
    endif()
endfunction()

function(sfml_set_common_ios_properties target)
    # enable automatic reference counting on iOS
    sfml_set_xcode_property(${target} CLANG_ENABLE_OBJC_ARC YES)
    sfml_set_xcode_property(${target} IPHONEOS_DEPLOYMENT_TARGET "${SFML_IOS_DEPLOYMENT_TARGET}")
    sfml_set_xcode_property(${target} CODE_SIGN_IDENTITY "${SFML_CODE_SIGN_IDENTITY}")

    get_target_property(target_type ${target} TYPE)
    if(target_type STREQUAL "EXECUTABLE")
        set_target_properties(${target} PROPERTIES
            MACOSX_BUNDLE ON # Bare executables are not usable on iOS, only bundle applications
            MACOSX_BUNDLE_GUI_IDENTIFIER "org.sfml-dev.${target}" # If missing, trying to launch an example in simulator will make Xcode < 9.3 crash
            MACOSX_BUNDLE_BUNDLE_NAME "${target}"
            MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
        )
    endif()
endfunction()

# add a new target which is a SFML library
# example: sfml_add_library(Graphics
#                           SOURCES sprite.cpp image.cpp ...
#                           [STATIC]) # Always create a static library and ignore BUILD_SHARED_LIBS
macro(sfml_add_library module)

    # parse the arguments
    cmake_parse_arguments(THIS "STATIC" "DEPENDENCIES" "SOURCES" ${ARGN})
    if(NOT "${THIS_UNPARSED_ARGUMENTS}" STREQUAL "")
        message(FATAL_ERROR "Extra unparsed arguments when calling sfml_add_library: ${THIS_UNPARSED_ARGUMENTS}")
    endif()

    # create the target
    string(TOLOWER sfml-${module} target)
    if(THIS_STATIC)
        add_library(${target} STATIC ${THIS_SOURCES})
    else()
        add_library(${target} ${THIS_SOURCES})
    endif()
    add_library(SFML::${module} ALIAS ${target})

    # enable C++17 support
    target_compile_features(${target} PUBLIC cxx_std_17)

    # Add required flags for GCC if coverage reporting is enabled
    if(SFML_ENABLE_COVERAGE AND (SFML_COMPILER_GCC OR SFML_COMPILER_CLANG))
        target_compile_options(${target} PUBLIC $<$<CONFIG:DEBUG>:-O0> $<$<CONFIG:DEBUG>:-g> $<$<CONFIG:DEBUG>:-fprofile-arcs> $<$<CONFIG:DEBUG>:-ftest-coverage>)
        target_link_options(${target} PUBLIC $<$<CONFIG:DEBUG>:--coverage>)
    endif()

    set_target_warnings(${target})
    set_public_symbols_hidden(${target})

    # enable precompiled headers
    if (SFML_ENABLE_PCH AND (NOT ${target} STREQUAL "sfml-system"))
        message(VERBOSE "enabling PCH for SFML library '${target}'")
        target_precompile_headers(${target} REUSE_FROM sfml-system)
    endif()

    # define the export symbol of the module
    string(REPLACE "-" "_" NAME_UPPER "${target}")
    string(TOUPPER "${NAME_UPPER}" NAME_UPPER)
    set_target_properties(${target} PROPERTIES DEFINE_SYMBOL ${NAME_UPPER}_EXPORTS)

    # define the export name of the module
    set_target_properties(${target} PROPERTIES EXPORT_NAME SFML::${module})

    # adjust the output file prefix/suffix to match our conventions
    if(BUILD_SHARED_LIBS AND NOT THIS_STATIC)
        if(SFML_OS_WINDOWS)
            # include the major version number in Windows shared library names (but not import library names)
            set_target_properties(${target} PROPERTIES DEBUG_POSTFIX -d)
            set_target_properties(${target} PROPERTIES SUFFIX "-${PROJECT_VERSION_MAJOR}${CMAKE_SHARED_LIBRARY_SUFFIX}")

            # fill out all variables we use to generate the .rc file
            string(TIMESTAMP RC_CURRENT_YEAR "%Y")
            string(REGEX REPLACE "sfml-([a-z])([a-z]*)" "\\1" RC_MODULE_NAME_HEAD "${target}")
            string(REGEX REPLACE "sfml-([a-z])([a-z]*)" "\\2" RC_MODULE_NAME_TAIL "${target}")
            string(TOUPPER "${RC_MODULE_NAME_HEAD}" RC_MODULE_NAME_HEAD)
            set(RC_MODULE_NAME "${RC_MODULE_NAME_HEAD}${RC_MODULE_NAME_TAIL}")
            set(RC_VERSION_SUFFIX "") # Add something like the git revision short SHA-1 in the future
            set(RC_PRERELEASE "0") # Set to 1 to mark the DLL as a pre-release DLL
            set(RC_TARGET_NAME "${target}")
            set(RC_TARGET_FILE_NAME_SUFFIX "-${PROJECT_VERSION_MAJOR}${CMAKE_SHARED_LIBRARY_SUFFIX}")

            # generate the .rc file
            configure_file(
                "${PROJECT_SOURCE_DIR}/tools/windows/resource.rc.in"
                "${CMAKE_CURRENT_BINARY_DIR}/${target}.rc"
                @ONLY
            )
            target_sources(${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/${target}.rc")
            source_group("" FILES "${CMAKE_CURRENT_BINARY_DIR}/${target}.rc")

            if(SFML_COMPILER_GCC OR SFML_COMPILER_CLANG)
                # on Windows + gcc/clang get rid of "lib" prefix for shared libraries,
                # and transform the ".dll.a" suffix into ".a" for import libraries
                set_target_properties(${target} PROPERTIES PREFIX "")
                set_target_properties(${target} PROPERTIES IMPORT_SUFFIX ".a")
            endif()
        else()
            set_target_properties(${target} PROPERTIES DEBUG_POSTFIX -d)
        endif()
    else()
        set_target_properties(${target} PROPERTIES DEBUG_POSTFIX -s-d)
        set_target_properties(${target} PROPERTIES RELEASE_POSTFIX -s)
        set_target_properties(${target} PROPERTIES MINSIZEREL_POSTFIX -s)
        set_target_properties(${target} PROPERTIES RELWITHDEBINFO_POSTFIX -s)
    endif()

    # set the version and soversion of the target (for compatible systems -- mostly Linuxes)
    # except for Android which strips soversion suffixes
    if(NOT SFML_OS_ANDROID)
        set_target_properties(${target} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
        set_target_properties(${target} PROPERTIES VERSION ${PROJECT_VERSION})
    endif()

    # set the target's folder (for IDEs that support it, e.g. Visual Studio)
    set_target_properties(${target} PROPERTIES FOLDER "SFML")

    # set the target flags to use the appropriate C++ standard library
    sfml_set_stdlib(${target})

    # For Visual Studio on Windows, export debug symbols (PDB files) to lib directory
    if(SFML_GENERATE_PDB)
        # PDB files are only generated in Debug and RelWithDebInfo configurations, find out which one
        if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
            set(SFML_PDB_POSTFIX "-d")
        else()
            set(SFML_PDB_POSTFIX "")
        endif()

        if(BUILD_SHARED_LIBS AND NOT THIS_STATIC)
            # DLLs export debug symbols in the linker PDB (the compiler PDB is an intermediate file)
            set_target_properties(${target} PROPERTIES
                                  PDB_NAME "${target}${SFML_PDB_POSTFIX}"
                                  PDB_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
        else()
            if(NOT ${target} STREQUAL "sfml-main")
                string(PREPEND SFML_PDB_POSTFIX "-s")
            endif()

            if(SFML_ENABLE_PCH)
                message(VERBOSE "overriding PDB name for '${target}' with \"sfml-system\" due to PCH being enabled")

                # For PCH builds with PCH reuse, the PDB name must be the same as the target that's being reused
                set_target_properties(${target} PROPERTIES
                                      COMPILE_PDB_NAME "sfml-system"
                                      COMPILE_PDB_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
            else()
                # Static libraries have no linker PDBs, thus the compiler PDBs are relevant
                set_target_properties(${target} PROPERTIES
                                      COMPILE_PDB_NAME "${target}${SFML_PDB_POSTFIX}"
                                      COMPILE_PDB_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
            endif()
        endif()
    endif()

    # build frameworks or dylibs
    if((SFML_OS_MACOS OR SFML_OS_IOS) AND BUILD_SHARED_LIBS AND NOT THIS_STATIC)
        if(SFML_BUILD_FRAMEWORKS)
            # adapt target to build frameworks instead of dylibs
            set_target_properties(${target} PROPERTIES
                                  FRAMEWORK ON
                                  FRAMEWORK_VERSION ${PROJECT_VERSION}
                                  MACOSX_FRAMEWORK_IDENTIFIER org.sfml-dev.${target}
                                  MACOSX_FRAMEWORK_SHORT_VERSION_STRING ${PROJECT_VERSION}
                                  MACOSX_FRAMEWORK_BUNDLE_VERSION ${PROJECT_VERSION})
        endif()

        # adapt install directory to allow distributing dylibs/frameworks in user's frameworks/application bundle
        # but only if cmake rpath options aren't set
        if(NOT CMAKE_SKIP_RPATH AND NOT CMAKE_SKIP_INSTALL_RPATH AND NOT CMAKE_INSTALL_RPATH AND NOT CMAKE_INSTALL_RPATH_USE_LINK_PATH AND NOT CMAKE_INSTALL_NAME_DIR)
            set_target_properties(${target} PROPERTIES INSTALL_NAME_DIR "@rpath")
            if(NOT CMAKE_SKIP_BUILD_RPATH)
                set_target_properties(${target} PROPERTIES BUILD_WITH_INSTALL_NAME_DIR ON)
            endif()
        endif()
    endif()

    if(SFML_OS_IOS)
        sfml_set_common_ios_properties(${target})
    endif()

    if(SFML_OS_ANDROID)
        # Always use position-independent code on Android, even when linking statically.
        # This is needed because all c++ code is placed in a shared library on Android.
        set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()

    if(BUILD_SHARED_LIBS)
        set(config_name "Shared")
    else()
        set(config_name "Static")
    endif()

    # install the target and create export-set
    install(TARGETS ${target} EXPORT SFML${module}${config_name}Targets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT bin
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT bin
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT devel
            FRAMEWORK DESTINATION "." COMPONENT bin)

    # because the frameworks directory hierarchy has to be set up before any target files
    # are installed we can't call install(EXPORT ...Targets) here
    # this is because frameworks are only set up after all modules directories have already been added
    # sfml_export_targets() is called after the frameworks are set up so we will have to
    # save all modules to a global property and read it out to call install(EXPORT ...Targets)
    # for each module in sfml_export_targets(), see below
    get_property(SFML_ADD_LIBRARY_MODULES GLOBAL PROPERTY SFML_ADD_LIBRARY_MODULES_PROPERTY)
    list(APPEND SFML_ADD_LIBRARY_MODULES ${module})
    set_property(GLOBAL PROPERTY SFML_ADD_LIBRARY_MODULES_PROPERTY "${SFML_ADD_LIBRARY_MODULES}")

    # when static linking, generate and install dependency configuration
    if(NOT BUILD_SHARED_LIBS AND THIS_DEPENDENCIES)
        # if we are building static libraries, generate and install dependencies config file
        include(CMakePackageConfigHelpers)

        configure_package_config_file("${THIS_DEPENDENCIES}" "${CMAKE_CURRENT_BINARY_DIR}/SFML${module}Dependencies.cmake"
            INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SFML")

        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/SFML${module}Dependencies.cmake"
                DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SFML
                COMPONENT devel)
    endif()

    # add <project>/include as public include directory
    target_include_directories(${target}
                               PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
                               PRIVATE "${PROJECT_SOURCE_DIR}/src")

    if(SFML_BUILD_FRAMEWORKS)
        target_include_directories(${target} INTERFACE "$<INSTALL_INTERFACE:SFML.framework>")
    else()
        target_include_directories(${target} INTERFACE "$<INSTALL_INTERFACE:include>")
    endif()

    # define SFML_STATIC if the build type is not set to 'shared'
    if(NOT BUILD_SHARED_LIBS)
        target_compile_definitions(${target} PUBLIC "SFML_STATIC")
    endif()

endmacro()

# add a new target which is a SFML example
# example: sfml_add_example(ftp
#                           SOURCES ftp.cpp ...
#                           BUNDLE_RESOURCES MainMenu.nib ...    # Files to be added in target but not installed next to the executable
#                           DEPENDS SFML::Network)
macro(sfml_add_example target)

    # parse the arguments
    cmake_parse_arguments(THIS "GUI_APP" "" "SOURCES;BUNDLE_RESOURCES;DEPENDS" ${ARGN})

    # set a source group for the source files
    source_group("" FILES ${THIS_SOURCES})

    # check whether resources must be added in target
    set(target_input ${THIS_SOURCES})
    if(THIS_BUNDLE_RESOURCES)
        set(target_input ${target_input} ${THIS_BUNDLE_RESOURCES})
    endif()

    # create the target
    if(THIS_GUI_APP AND SFML_OS_WINDOWS AND NOT DEFINED CMAKE_CONFIGURATION_TYPES AND ${CMAKE_BUILD_TYPE} STREQUAL "Release")
        add_executable(${target} WIN32 ${target_input})
        target_link_libraries(${target} PRIVATE SFML::Main)
    elseif(THIS_GUI_APP AND SFML_OS_IOS)

        # For iOS apps we need the launch screen storyboard,
        # and a custom info.plist to use it
        set(LAUNCH_SCREEN "${PROJECT_SOURCE_DIR}/examples/assets/LaunchScreen.storyboard")
        set(LOGO "${PROJECT_SOURCE_DIR}/examples/assets/logo.png")
        set(INFO_PLIST "${PROJECT_SOURCE_DIR}/examples/assets/info.plist")
        set(ICONS "${PROJECT_SOURCE_DIR}/examples/assets/icon.icns")
        add_executable(${target} MACOSX_BUNDLE ${target_input} ${LAUNCH_SCREEN} ${LOGO} ${ICONS})
        set(RESOURCES ${LAUNCH_SCREEN} ${LOGO} ${ICONS})
        set_target_properties(${target} PROPERTIES RESOURCE "${RESOURCES}"
                                                   MACOSX_BUNDLE_INFO_PLIST ${INFO_PLIST}
                                                   MACOSX_BUNDLE_ICON_FILE icon.icns)
        target_link_libraries(${target} PRIVATE SFML::Main)
    else()
        add_executable(${target} ${target_input})
    endif()

    # enable precompiled headers
    if (SFML_ENABLE_PCH)
        message(VERBOSE "enabling PCH for SFML example '${target}'")
        target_precompile_headers(${target} REUSE_FROM sfml-system)
    endif()

    set_target_warnings(${target})
    set_public_symbols_hidden(${target})

    # set the debug suffix
    set_target_properties(${target} PROPERTIES DEBUG_POSTFIX -d)

    # set the target's folder (for IDEs that support it, e.g. Visual Studio)
    set_target_properties(${target} PROPERTIES FOLDER "Examples")

    # set the target flags to use the appropriate C++ standard library
    sfml_set_stdlib(${target})

    # set the Visual Studio startup path for debugging
    set_target_properties(${target} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    # link the target to its SFML dependencies
    if(THIS_DEPENDS)
        target_link_libraries(${target} PRIVATE ${THIS_DEPENDS})
    endif()

    if(SFML_OS_IOS)
        sfml_set_common_ios_properties(${target})
    endif()

    if(SFML_OS_WINDOWS AND SFML_USE_MESA3D)
        add_dependencies(${target} "install-mesa3d")
    endif()
endmacro()

# add a new target which is a SFML test
# example: sfml_add_test(sfml-test
#                           ftp.cpp ...
#                           SFML::Network)
function(sfml_add_test target SOURCES DEPENDS)

    # set a source group for the source files
    source_group("" FILES ${SOURCES})

    # create the target
    add_executable(${target} ${SOURCES})

    # enable precompiled headers
    if (SFML_ENABLE_PCH)
        message(VERBOSE "enabling PCH for SFML test '${target}'")
        target_precompile_headers(${target} REUSE_FROM sfml-system)
    endif()

    # set the target's folder (for IDEs that support it, e.g. Visual Studio)
    set_target_properties(${target} PROPERTIES FOLDER "Tests")

    # set the target flags to use the appropriate C++ standard library
    sfml_set_stdlib(${target})

    # set the Visual Studio startup path for debugging
    set_target_properties(${target} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    # link the target to its SFML dependencies
    target_link_libraries(${target} PRIVATE ${DEPENDS} sfml-test-main)

    set_target_warnings(${target})
    set_public_symbols_hidden(${target})

    # If coverage is enabled for MSVC and we are linking statically, use /WHOLEARCHIVE
    # to make sure the linker doesn't discard unused code sections before coverage can be measured
    if(SFML_ENABLE_COVERAGE AND SFML_COMPILER_MSVC AND NOT BUILD_SHARED_LIBS)
        foreach(DEPENDENCY ${DEPENDS})
            target_link_options(${target} PRIVATE $<$<CONFIG:DEBUG>:/WHOLEARCHIVE:$<TARGET_LINKER_FILE:${DEPENDENCY}>>)
        endforeach()
    endif()

    if(SFML_OS_WINDOWS AND SFML_USE_MESA3D)
        add_dependencies(${target} "install-mesa3d")
    endif()

    # Delay test registration when cross compiling to avoid running crosscompiled app on host OS
    if(CMAKE_CROSSCOMPILING)
        set(CMAKE_CATCH_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)

        # When running tests on Android, use a custom shell script to invoke commands using adb shell
        if(SFML_OS_ANDROID)
            set_target_properties(${target} PROPERTIES CROSSCOMPILING_EMULATOR "${PROJECT_BINARY_DIR}/run-in-adb-shell.sh")
        endif()
    endif()

    # Add the test
    catch_discover_tests(${target} WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
endfunction()

# Generate a SFMLConfig.cmake file (and associated files) from the targets registered
# in SFML_ADD_LIBRARY_MODULES_PROPERTY (EXPORT parameter of install(TARGETS))
function(sfml_export_targets)
    # CMAKE_CURRENT_LIST_DIR or CMAKE_CURRENT_SOURCE_DIR not usable for files that are to be included like this one
    set(CURRENT_DIR "${PROJECT_SOURCE_DIR}/cmake")

    include(CMakePackageConfigHelpers)
    write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/SFMLConfigVersion.cmake"
                                     VERSION ${PROJECT_VERSION}
                                     COMPATIBILITY SameMajorVersion)

    if(SFML_BUILD_FRAMEWORKS)
        set(config_package_location "SFML.framework/Resources/CMake")
    else()
        set(config_package_location ${CMAKE_INSTALL_LIBDIR}/cmake/SFML)
    endif()
    configure_package_config_file("${CURRENT_DIR}/SFMLConfig.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/SFMLConfig.cmake"
        INSTALL_DESTINATION "${config_package_location}")

    # generate and install export files for all modules that were added to the list in sfml_add_library()
    if(BUILD_SHARED_LIBS)
        set(config_name "Shared")
    else()
        set(config_name "Static")
    endif()
    get_property(SFML_ADD_LIBRARY_MODULES GLOBAL PROPERTY SFML_ADD_LIBRARY_MODULES_PROPERTY)
    foreach(module ${SFML_ADD_LIBRARY_MODULES})
        install(EXPORT SFML${module}${config_name}Targets
                FILE SFML${module}${config_name}Targets.cmake
                DESTINATION ${config_package_location})
    endforeach()

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/SFMLConfig.cmake"
                  "${CMAKE_CURRENT_BINARY_DIR}/SFMLConfigVersion.cmake"
            DESTINATION ${config_package_location}
            COMPONENT devel)
endfunction()
