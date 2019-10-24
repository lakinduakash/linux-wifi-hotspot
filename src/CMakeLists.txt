cmake_minimum_required(VERSION 3.10)
project(wihotspot)

include(FindPkgConfig)
pkg_check_modules(GTK gtk+-3.0 REQUIRED)
pkg_check_modules(X11 x11 REQUIRED)
include_directories(${GTK_INCLUDE_DIRS})
include_directories(${X11_INCLUDE_DIRS})

set(CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

add_executable(wihotspot ui/main.c ui/h_prop.c ui/h_prop.h ui/ui.c ui/ui.h ui/read_config.cpp ui/read_config.h ui/util.c ui/util.h)

target_link_libraries(${PROJECT_NAME} ${GTK_LIBRARIES} ${X11_LIBRARIES})

include_directories(/usr/include)

set (source "${CMAKE_SOURCE_DIR}/ui/glade")
set (destination "${CMAKE_CURRENT_BINARY_DIR}/glade")

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${source} ${destination}
        DEPENDS ${destination}
        COMMENT "symbolic link resources folder from ${source} => ${destination}"
)

set (appdir "/usr/share")
set (appbin "/usr/bin")

add_custom_target(install_build
        COMMAND mkdir -p $ENV{DESTDIR}${appdir}/${PROJECT_NAME}
        COMMAND mkdir -p $ENV{DESTDIR}${appdir}/${PROJECT_NAME}/glade
        COMMAND install -Dm755 ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME} $ENV{DESTDIR}${appdir}/${PROJECT_NAME}/${PROJECT_NAME}
        COMMAND install -Dm755 ${CMAKE_CURRENT_BINARY_DIR}/glade/* $ENV{DESTDIR}${appdir}/${PROJECT_NAME}/glade/
        COMMAND ln -rs $ENV{DESTDIR}${appdir}/${PROJECT_NAME}/${PROJECT_NAME} $ENV{DESTDIR}${appbin}/${PROJECT_NAME}
        #COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{DESTDIR}${appdir}/${PROJECT_NAME}/${PROJECT_NAME} $ENV{DESTDIR}${appbin}/${PROJECT_NAME}
        DEPENDS ${PROJECT_NAME}
        COMMENT Installing files...
        )

add_custom_target(uninstall_build
        COMMAND rm -r $ENV{DESTDIR}${appdir}/${PROJECT_NAME}
        COMMAND rm -f $ENV{DESTDIR}${appbin}/${PROJECT_NAME}
        COMMENT Unstalling files...
        )