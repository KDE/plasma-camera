# SPDX-FileCopyrightText: 2025 Andrew Wang
# SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
# SPDX-License-Identifier: GPL-3.0-or-later

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(plasma-camera_SRCS
    main.cpp
    plasmacamera.cpp
    plasmacameramanager.cpp
    plasmacamera/converter.cpp
    plasmacamera/image.cpp
    plasmacamera/path.cpp
    plasmacamera/settings.cpp
    plasmacamera/worker.cpp
    resources.qrc
)

find_package(Qt6 REQUIRED COMPONENTS Multimedia)

add_executable(plasma-camera ${plasma-camera_SRCS})
target_compile_options(plasma-camera PUBLIC -fexceptions)
target_link_libraries(
        plasma-camera
        Qt::Core
        Qt::Qml
        Qt::Quick
        Qt::Svg
        Qt::Sensors
        Qt6::Multimedia
        KF6::CoreAddons
        KF6::I18n
        KF6::ConfigGui
)

find_package(PkgConfig)
pkg_check_modules(LIBCAMERA REQUIRED IMPORTED_TARGET libcamera)
include_directories(${CMAKE_SOURCE_DIR} ${LIBCAMERA_INCLUDE_DIRS})

kconfig_add_kcfg_files(plasma-camera GENERATE_MOC camerasettings.kcfgc)

if(ANDROID)
    target_link_libraries(plasma-camera Qt::Svg KF6::Kirigami)

    kirigami_package_breeze_icons(ICONS
        emblem-videos-symbolic
        camera-photo-symbolic
        dialog-error-symbolic
        media-playback-start
        media-playback-stop
        alarm-symbolic
        camera-photo
        ratiocrop
        whitebalance
        clock
        help-about
    )

    ecm_add_android_apk(plasma-camera ANDROID_DIR ${CMAKE_SOURCE_DIR})
else()
    target_link_libraries(plasma-camera Qt::Widgets PkgConfig::LIBCAMERA)
endif()

target_include_directories(plasma-camera PRIVATE ${CMAKE_BINARY_DIR})
install(TARGETS plasma-camera ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
