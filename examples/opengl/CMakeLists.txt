# all source files
set(SRC OpenGL.cpp)

if(SFML_OS_IOS)
    set(RESOURCES
        resources/background.jpg
        resources/tuffy.ttf)
    set_source_files_properties(${RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

# define the opengl target
sfml_add_example(opengl GUI_APP
                 SOURCES ${SRC}
                 BUNDLE_RESOURCES ${RESOURCES}
                 DEPENDS SFML::Graphics)

# external dependency headers
target_include_directories(opengl SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/examples/include")
