# SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
# SPDX-License-Identifier: GPL-3.0-or-later

set(plasma-camera_SRCS
    main.cpp
    plasmacamera.cpp
    resources.qrc
)

add_executable(plasma-camera ${plasma-camera_SRCS})
target_link_libraries(plasma-camera Qt::Core Qt::Qml Qt::Quick Qt::Svg
                                    KF6::CoreAddons KF6::I18n KF6::ConfigGui)

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
    target_link_libraries(plasma-camera Qt::Widgets)
endif()

install(TARGETS plasma-camera ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
