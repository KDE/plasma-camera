// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2013 Digia Plc and/or its subsidiary(-ies)
// SPDX-License-Identifier: BSD-3-Clause

import org.kde.kirigami 2.0 as Kirigami
import QtQuick 2.7
import QtMultimedia
import org.kde.plasmacamera 1.0

Kirigami.GlobalDrawer {
    id: drawer

    property var camera

    handleVisible: false
    actions: [
        Kirigami.Action {
            id: devicesAction

            text: i18n("Camera")
            icon.name: "camera-photo-symbolic"
            Component.onCompleted: {
                var cameras = devices.videoInputs;
                var childrenList = [];
                for (var i in cameras) {
                    childrenList[i] = devicesSubAction.createObject(devicesAction, {
                        "value": cameras[i].id,
                        "text": "%1".arg(cameras[i].description)
                    });
                }
                devicesAction.children = childrenList;
            }
        },
        Kirigami.Action {
            id: resolutionAction

            text: i18n("Resolution")
            icon.name: "ratiocrop"
            Component.onCompleted: {
                var resolutions = drawer.camera.cameraDevice.videoFormats;
                var childrenList = [];
                for (var i in resolutions) {
                    var pixels = resolutions[i].resolution.width * resolutions[i].resolution.height;
                    var megapixels = Math.round(pixels / 10000) / 100;
                    childrenList[i] = resolutionSubAction.createObject(resolutionAction, {
                        "value": resolutions[i].resolution,
                        "text": "%1 x %2 (%3 MP)".arg(resolutions[i].resolution.width).arg(resolutions[i].resolution.height).arg(megapixels)
                    });
                }
                resolutionAction.children = childrenList;
            }
        },
        Kirigami.Action {
            id: wbaction

            text: i18n("White balance")
            icon.name: "whitebalance"

            Kirigami.Action {
                icon.name: "qrc:///camera_auto_mode.png"
                onTriggered: CameraSettings.whiteBalanceMode = Camera.WhiteBalanceAuto
                text: i18n("Auto")
                checked: CameraSettings.whiteBalanceMode === Camera.WhiteBalanceAuto
            }

            Kirigami.Action {
                icon.name: "qrc:///camera_white_balance_sunny.png"
                onTriggered: CameraSettings.whiteBalanceMode = Camera.WhiteBalanceSunlight
                text: i18n("Sunlight")
                checked: CameraSettings.whiteBalanceMode === Camera.WhiteBalanceSunlight
            }

            Kirigami.Action {
                icon.name: "qrc:///camera_white_balance_cloudy.png"
                onTriggered: CameraSettings.whiteBalanceMode = Camera.WhiteBalanceCloudy
                text: i18n("Cloudy")
                checked: CameraSettings.whiteBalanceMode === Camera.WhiteBalanceCloudy
            }

            Kirigami.Action {
                icon.name: "qrc:///camera_white_balance_incandescent.png"
                onTriggered: CameraSettings.whiteBalanceMode = Camera.WhiteBalanceTungsten
                text: i18n("Tungsten")
                checked: CameraSettings.whiteBalanceMode === Camera.WhiteBalanceTungsten
            }

            Kirigami.Action {
                icon.name: "qrc:///camera_white_balance_flourescent.png"
                onTriggered: CameraSettings.whiteBalanceMode = Camera.WhiteBalanceFluorescent
                text: i18n("Fluorescent")
                checked: CameraSettings.whiteBalanceMode === Camera.WhiteBalanceFluorescent
            }

        },
        Kirigami.Action {
            text: i18n("About")
            icon.name: "help-about"
            onTriggered: pageStack.pushDialogLayer("qrc:/AboutPage.qml")
        }
    ]

    Component {
        id: devicesSubAction

        Kirigami.Action {
            property string value

            checked: value === CameraSettings.cameraDeviceId
            onTriggered: {
                CameraSettings.cameraDeviceId = value;
            }
        }

    }

    Component {
        id: resolutionSubAction

        Kirigami.Action {
            property size value

            checked: value === CameraSettings.resolution
            onTriggered: {
                CameraSettings.resolution = value;
            }
        }

    }

    MediaDevices {
        id: devices
    }

}
