cmake_minimum_required(VERSION 3.16)

project(APRS_WX VERSION 0.1.6.4)
set(PACKAGE_CONTACT "Richard Buckley <richard.buckley@ieee.org>")
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules" "${CMAKE_MODULE_PATH}")
find_package(CURLPP REQUIRED)
include_directories(${CURLPP_INCLUDE_DIR})

add_executable(
        APRS_WX
        src/aprs_wx.cpp
        src/APRS_Packet.cpp
        src/APRS_IS.cpp
        src/WeatherAggregator.cpp
        src/ConfigFile.cpp
        src/XDGFilePaths.cpp)

target_link_libraries(
        APRS_WX
        stdc++fs
        ${CURLPP_LIBRARIES}
)

install(TARGETS APRS_WX
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT APRS_WX_SYSTEMD EXCLUDE_FROM_ALL
        )

install(FILES "resources/aprs_wx/config.txt"
        DESTINATION "/etc/${PROJECT_NAME}"
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ GROUP_WRITE
        COMPONENT APRS_WX_SYSTEMD EXCLUDE_FROM_ALL
        )

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/aprs_wx.service"
        DESTINATION /etc/systemd/system
        COMPONENT APRS_WX_SYSTEMD EXCLUDE_FROM_ALL
        )

set(CPACK_DEB_COMPONENT_INSTALL 1)
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS 1)
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_RELEASE ${PROJECT_VERSION_TWEAK})
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_DEBIAN_APRS_WX_SYSTEMD_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_DEBIAN_PACKAGE_SECTION "hamradio")
set(CPACK_RPM_PACKAGE_URL "https://github.com/pa28/aprs_wx")
set(CPACK_RPM_PACKAGE_SUMMARY "CWOP Weather packet aggregator.")
set(CPACK_RPM_PACKAGE_DESCRIPTION "CWOP Weather packet aggregator.")
set(CPACK_RPM_PACKAGE_VENDOR "www.hrbuckley.net")
set(CPACK_PACKAGE_CONTACT "Richard Buckley")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Productivity")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_APRS_WX_SYSTEMD_EXECUTABLES APRS_WX)
#set(CPACK_PACKAGE_HAMCLOCK_EXECUTABLES SourceCode/ESPHamClock/hamclock-800x480)
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
        /usr/local
        /usr/local/bin
        /usr/local/share
        /usr/local/include
        /usr/lib/systemd
        /usr/lib/systemd/system
        )
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/postinst")

include(CPack)

cpack_add_component(APRS_WX_SYSTEMD
        DISPLAY_NAME "aprs_wx"
        DESCRIPTION "CWOP Weather packet aggregator."
        )
