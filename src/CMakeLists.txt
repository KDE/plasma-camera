set(plasma-camera_SRCS
    main.cpp
    )

qt5_add_resources(RESOURCES resources.qrc)
add_executable(plasma-camera ${plasma-camera_SRCS} ${RESOURCES})
target_link_libraries(plasma-camera Qt5::Core  Qt5::Qml Qt5::Quick Qt5::Svg)
install(TARGETS plasma-camera ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
