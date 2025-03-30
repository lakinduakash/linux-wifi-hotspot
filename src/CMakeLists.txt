cmake_minimum_required(VERSION 3.12)
project(wihotspot-gui)

include(FindPkgConfig)
pkg_check_modules(GTK gtk+-3.0 REQUIRED)
pkg_check_modules(X11 x11 REQUIRED)
include_directories(${GTK_INCLUDE_DIRS})
include_directories(${X11_INCLUDE_DIRS})

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

add_executable(wihotspot-gui ui/main.c ui/h_prop.c ui/h_prop.h ui/ui.c ui/ui.h ui/read_config.cpp ui/read_config.h ui/util.c ui/util.h)

target_link_libraries(${PROJECT_NAME} ${GTK_LIBRARIES} ${X11_LIBRARIES})

set(source "${CMAKE_SOURCE_DIR}/ui/glade")
set(destination "${CMAKE_CURRENT_BINARY_DIR}/glade")

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${source} ${destination}
    DEPENDS ${destination}
    COMMENT "symbolic link resources folder from ${source} => ${destination}")

install(TARGETS ${PROJECT_NAME}
    DESTINATION share/${PROJECT_NAME})
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/glade
    DESTINATION share/${PROJECT_NAME})
install(CODE "execute_process( \
    COMMAND ${CMAKE_COMMAND} -E create_symlink \
    ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/${PROJECT_NAME} \
    ${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME} \
    )")
install(FILES ${CMAKE_SOURCE_DIR}/desktop/hotspot.png
    DESTINATION share/${PROJECT_NAME})
install(FILES ${CMAKE_SOURCE_DIR}/desktop/wifihotspot.desktop
    DESTINATION share/${PROJECT_NAME}
    RENAME ${PROJECT_NAME}.desktop)
install(FILES ${CMAKE_SOURCE_DIR}/desktop/wifihotspot.desktop
    DESTINATION share/applications
    RENAME ${PROJECT_NAME}.desktop)

add_custom_target(uninstall
    COMMAND rm -rf ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}
    COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}
    COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/share/applications/${PROJECT_NAME}.desktop
    COMMAND rm -rf ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}
    COMMENT Uninstalling files...
    COMMENT Remove old files if exist
    COMMAND rm -rf ${CMAKE_INSTALL_PREFIX}/share/wihotspot
    COMMAND rm -f ${CMAKE_INSTALL_PREFIX}/share/applications/wihotspot.desktop)
