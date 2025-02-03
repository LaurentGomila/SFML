# all source files
set(SRC Vulkan.cpp)

# define the window target
sfml_add_example(vulkan GUI_APP
                 SOURCES ${SRC}
                 DEPENDS SFML::Graphics)

# external dependency headers
target_include_directories(vulkan SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/examples/vulkan")
