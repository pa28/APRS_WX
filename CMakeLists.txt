cmake_minimum_required(VERSION 3.16)

project(APRS_WX VERSION 0.1.0.1)

set(CMAKE_CXX_STANDARD 17)

add_executable(
        APRS_WX
        main.cpp
        src/APRS_Packet.cpp
        src/APRS_IS.cpp
        src/WeatherAggregator.cpp
        )

install(TARGETS APRS_WX
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

set(CPACK_DEB_COMPONENT_INSTALL 1)
set(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS 1)
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_RELEASE ${PROJECT_VERSION_TWEAK})
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_DEBIAN_HAMCLOCK_SYSTEMD_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_DEBIAN_PACKAGE_SECTION "hamradio")
set(CPACK_RPM_PACKAGE_URL "https://github.com/pa28/")
set(CPACK_RPM_PACKAGE_SUMMARY "CWOP Weather packet aggregator.")
set(CPACK_RPM_PACKAGE_DESCRIPTION "CWOP Weather packet aggregator.")
set(CPACK_RPM_PACKAGE_VENDOR "www.hrbuckley.net")
set(CPACK_PACKAGE_CONTACT "Richard Buckley")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Productivity")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_HAMCLOCK_SYSTEMD_EXECUTABLES SourceCode/ESPHamClock/hamclock-fb0-800x480)
set(CPACK_PACKAGE_HAMCLOCK_EXECUTABLES SourceCode/ESPHamClock/hamclock-800x480)
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
