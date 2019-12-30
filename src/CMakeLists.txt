set(plasma-camera_SRCS
    main.cpp
    )

qt5_add_resources(RESOURCES resources.qrc)
add_executable(plasma-camera ${plasma-camera_SRCS} ${RESOURCES})
target_link_libraries(plasma-camera Qt5::Core Qt5::Qml Qt5::Quick Qt5::Svg KF5::CoreAddons KF5::I18n)

if(ANDROID)
    target_link_libraries(plasma-camera Qt5::AndroidExtras Qt5::Svg KF5::Kirigami2)

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

endif()

install(TARGETS plasma-camera ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
