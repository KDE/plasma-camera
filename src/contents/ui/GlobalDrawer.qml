// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2013 Digia Plc and/or its subsidiary(-ies)
// SPDX-License-Identifier: BSD-3-Clause

import org.kde.kirigami 2.0 as Kirigami
import QtQuick 2.7
import QtMultimedia 5.8

import org.kde.plasmacamera 1.0

Kirigami.GlobalDrawer {
    id: drawer
    property var camera
    handleVisible: false
    Component {
        id: devicesSubAction

        Kirigami.Action {
            property string value
            checked: value === CameraSettings.cameraDeviceId

            onTriggered: {
                CameraSettings.cameraDeviceId = value
            }
        }
    }

    Component {
        id: resolutionSubAction

        Kirigami.Action {
            property size value
            checked: value === CameraSettings.resolution

            onTriggered: {
                CameraSettings.resolution = value
            }
        }
    }

    actions: [
        Kirigami.Action {
            id: devicesAction
            text: i18n("Camera")
            iconName: "camera-photo-symbolic"
            Component.onCompleted: {
                var cameras = QtMultimedia.availableCameras
                var childrenList = []

                for (var i in cameras) {
                    childrenList[i] = devicesSubAction.createObject(devicesAction, {
                        value: cameras[i].deviceId,
                        text: "%1".arg(cameras[i].displayName)
                    })
                }
                devicesAction.children = childrenList
            }
        },
        Kirigami.Action {
            id: resolutionAction
            text: i18n("Resolution")
            iconName: "ratiocrop"
            Component.onCompleted: {
                var resolutions = drawer.camera.imageCapture.supportedResolutions
                var childrenList = []

                for (var i in resolutions) {
                    var pixels = resolutions[i].width * resolutions[i].height
                    var megapixels = Math.round(pixels / 10000) / 100

                    childrenList[i] = resolutionSubAction.createObject(resolutionAction, {
                        value: resolutions[i],
                        text: "%1 x %2 (%3 MP)".arg(resolutions[i].width).arg(resolutions[i].height).arg(megapixels)
                    })
                }
                resolutionAction.children = childrenList
            }
        },
        Kirigami.Action {
            id: wbaction
            text: i18n("White balance")
            iconName: "whitebalance"
            Kirigami.Action {
                iconName: "qrc:///camera_auto_mode.png"
                onTriggered: CameraSettings.whiteBalanceMode = CameraImageProcessing.WhiteBalanceAuto
                text: i18n("Auto")
                checked: CameraSettings.whiteBalanceMode === CameraImageProcessing.WhiteBalanceAuto
            }
            Kirigami.Action {
                iconName: "qrc:///camera_white_balance_sunny.png"
                onTriggered: CameraSettings.whiteBalanceMode = CameraImageProcessing.WhiteBalanceSunlight
                text: i18n("Sunlight")
                checked: CameraSettings.whiteBalanceMode === CameraImageProcessing.WhiteBalanceSunlight
            }
            Kirigami.Action {
                iconName: "qrc:///camera_white_balance_cloudy.png"
                onTriggered: CameraSettings.whiteBalanceMode = CameraImageProcessing.WhiteBalanceCloudy
                text: i18n("Cloudy")
                checked: CameraSettings.whiteBalanceMode === CameraImageProcessing.WhiteBalanceCloudy
            }
            Kirigami.Action {
                iconName: "qrc:///camera_white_balance_incandescent.png"
                onTriggered: CameraSettings.whiteBalanceMode = CameraImageProcessing.WhiteBalanceTungsten
                text: i18n("Tungsten")
                checked: CameraSettings.whiteBalanceMode === CameraImageProcessing.WhiteBalanceTungsten
            }
            Kirigami.Action {
                iconName: "qrc:///camera_white_balance_flourescent.png"
                onTriggered: CameraSettings.whiteBalanceMode = CameraImageProcessing.WhiteBalanceFluorescent
                text: i18n("Fluorescent")
                checked: CameraSettings.whiteBalanceMode === CameraImageProcessing.WhiteBalanceFluorescent
            }
        },
        Kirigami.Action {
            text: i18n("About")
            iconName: "help-about"
            onTriggered: pageStack.pushDialogLayer("qrc:/AboutPage.qml")
        }
    ]
}
