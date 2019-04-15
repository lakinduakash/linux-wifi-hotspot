cmake_minimum_required(VERSION 3.12)
project(wihotspot)

include(FindPkgConfig)
pkg_check_modules(GTK gtk+-3.0 REQUIRED)
pkg_check_modules(X11 x11 REQUIRED)
include_directories(${GTK_INCLUDE_DIRS})
include_directories(${X11_INCLUDE_DIRS})

set(CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

add_executable(wihotspot ui/main.c ui/h_prop.c ui/h_prop.h ui/ui.c ui/ui.h ui/read_config.cpp ui/read_config.h ui/util.c ui/util.h)

target_link_libraries(${PROJECT_NAME} ${GTK_LIBRARIES} ${X11_LIBRARIES} config)

include_directories(/usr/include)

set (source "${CMAKE_SOURCE_DIR}/ui/glade")
set (destination "${CMAKE_CURRENT_BINARY_DIR}/glade")
add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${source} ${destination}
        DEPENDS ${destination}
        COMMENT "symbolic link resources folder from ${source} => ${destination}"
)
