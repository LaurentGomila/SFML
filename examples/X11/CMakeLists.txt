find_package(X11 REQUIRED)

# all source files
set(SRC X11.cpp)

# define the X11 target
sfml_add_example(X11Example GUI_APP
                 SOURCES ${SRC}
                 DEPENDS SFML::Window X11::X11)

# external dependency headers
target_include_directories(X11Example SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/examples/include")
