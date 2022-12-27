cmake_minimum_required(VERSION 3.16)

project(APRS_WX VERSION 0.3.1.0)
set(PACKAGE_CONTACT "Richard Buckley <richard.buckley@ieee.org>")
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules" "${CMAKE_MODULE_PATH}")
find_package(CURLPP REQUIRED)
include_directories(${CURLPP_INCLUDE_DIR})

set(DAEMON_USER "daemon")
set(DAEMON_GROUP "daemon")
set(MONITOR_NAME "SYS_MONITOR")
set(APRS_WX_CONFIG_DIR "${CMAKE_INSTALL_DATA}")

add_compile_options(-Wall -Wextra -pedantic -Werror -Wconversion)

add_executable(APRS_WX
        src/aprs_wx.cpp
        src/APRS_Packet.cpp
        src/APRS_IS.cpp
        src/WeatherAggregator.cpp
        src/ConfigFile.cpp
        src/XDGFilePaths.cpp src/InputParser.h)

target_link_libraries(APRS_WX
        stdc++fs
        ${CURLPP_LIBRARIES}
)

add_executable(SYS_MONITOR
        src/sys_monitor.cpp
        src/XDGFilePaths.cpp
        src/InputParser.h
        src/ConfigFile.cpp
        src/unixstd.h)

target_link_libraries(SYS_MONITOR
        stdc++fs
        ${CURLPP_LIBRARIES})

# APRS_WX
# conffiles
configure_file("resources/aprs_wx/conffiles.in" "resources/aprs_wx/conffiles" NEWLINE_STYLE UNIX)

# Configure the systemctl service file.
configure_file("resources/aprs_wx/aprs_wx.service.in" "resources/aprs_wx/aprs_wx.service" NEWLINE_STYLE UNIX)

# Configure postinst
configure_file("resources/aprs_wx/postinst.in" "resources/aprs_wx/postinst" NEWLINE_STYLE UNIX)

# Configure config
configure_file("resources/aprs_wx/config.in" "resources/aprs_wx/config.txt" NEWLINE_STYLE UNIX)
configure_file("resources/aprs_wx/config.in" "resources/aprs_wx/config.pkg" NEWLINE_STYLE UNIX)

# SYS_MONITOR
# conffiles
configure_file("resources/computers/conffiles.in" "resources/computers/conffiles" NEWLINE_STYLE UNIX)

# Configure the systemctl service file.
configure_file("resources/computers/sys_monitor.service.in" "resources/computers/sys_monitor.service" NEWLINE_STYLE UNIX)

# Configure postinst
configure_file("resources/computers/postinst.in" "resources/computers/postinst" NEWLINE_STYLE UNIX)

# Configure config
configure_file("resources/computers/config.in" "resources/computers/config.txt" NEWLINE_STYLE UNIX)
configure_file("resources/computers/config.in" "resources/computers/config.pkg" NEWLINE_STYLE UNIX)

install(TARGETS APRS_WX
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT APRS_WX_SYSTEMD EXCLUDE_FROM_ALL
        )

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/resources/aprs_wx/config.txt"
        "${CMAKE_CURRENT_BINARY_DIR}/resources/aprs_wx/config.pkg"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}"
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ GROUP_WRITE
        COMPONENT APRS_WX_SYSTEMD EXCLUDE_FROM_ALL
        )

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/resources/aprs_wx/aprs_wx.service"
        DESTINATION /etc/systemd/system
        COMPONENT APRS_WX_SYSTEMD EXCLUDE_FROM_ALL
        )

install(TARGETS ${MONITOR_NAME}
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT SYS_MONITOR EXCLUDE_FROM_ALL)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/resources/computers/config.txt"
        "${CMAKE_CURRENT_BINARY_DIR}/resources/computers/config.pkg"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/${MONITOR_NAME}"
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ GROUP_WRITE
        COMPONENT SYS_MONITOR EXCLUDE_FROM_ALL
        )

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/resources/computers/sys_monitor.service"
        DESTINATION /etc/systemd/system
        COMPONENT SYS_MONITOR EXCLUDE_FROM_ALL
        )

set(CPACK_DEB_COMPONENT_INSTALL 1)
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS 1)
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_RELEASE ${PROJECT_VERSION_TWEAK})
set(CPACK_GENERATOR "DEB")
#
set(CPACK_PACKAGE_SYS_MONITOR_EXECUTABLES SYS_MONITOR)
set(CPACK_DEBIAN_SYS_MONITOR_PACKAGE_NAME "${MONITOR_NAME}")
set(CPACK_DEBIAN_SYS_MONITOR_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_BINARY_DIR}/resources/computers/postinst;${CMAKE_CURRENT_BINARY_DIR}/resources/computers/conffiles")
#
set(CPACK_PACKAGE_APRS_WX_SYSTEMD_EXECUTABLES APRS_WX)
set(CPACK_DEBIAN_APRS_WX_SYSTEMD_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_DEBIAN_APRS_WX_SYSTEMD_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_BINARY_DIR}/resources/aprs_wx/postinst;${CMAKE_CURRENT_BINARY_DIR}/resources/aprs_wx/conffiles")
#
set(CPACK_DEBIAN_PACKAGE_SECTION "hamradio")
set(CPACK_RPM_PACKAGE_URL "https://github.com/pa28/aprs_wx")
set(CPACK_RPM_PACKAGE_VENDOR "www.hrbuckley.net")
set(CPACK_PACKAGE_CONTACT "Richard Buckley")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Productivity")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
        /usr/local
        /usr/local/bin
        /usr/local/share
        /usr/local/include
        /usr/lib/systemd
        /usr/lib/systemd/system
        )


include(CPack)

cpack_add_component(APRS_WX_SYSTEMD
        DISPLAY_NAME "aprs_wx"
        DESCRIPTION "CWOP Weather packet aggregator."
        )

cpack_add_component(SYS_MONITOR
        DISPLAY_NAME "sys_monitor"
        DESCRIPTION "Push computer operating statistics to an influxDB."
        )
