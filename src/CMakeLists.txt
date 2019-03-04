cmake_minimum_required(VERSION 3.12)
project(wihotspot C)

include(FindPkgConfig)
pkg_check_modules(GTK gtk+-3.0 REQUIRED)
include_directories(${GTK_INCLUDE_DIRS})

set(CMAKE_C_STANDARD 99)

add_executable(wihotspot ui/main.c)

target_link_libraries(${PROJECT_NAME} ${GTK_LIBRARIES})

include_directories(/usr/include)

file(COPY ui/glade DESTINATION ${CMAKE_BINARY_DIR})