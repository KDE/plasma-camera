// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2013 Digia Plc and/or its subsidiary(-ies)
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.7
import org.kde.kirigami 2.2 as Kirigami
import QtMultimedia 5.8

import org.kde.plasmacamera 1.0

Kirigami.ApplicationWindow {
    id: root

    Component {
        id: cameraPage

        CameraPage {
            camera: mainCamera
        }
    }

    Camera {
        id: mainCamera
        captureMode: Camera.CaptureStillImage
        deviceId: CameraSettings.cameraDeviceId
        imageProcessing.whiteBalanceMode: CameraSettings.whiteBalanceMode

        property int selfTimerDuration: 0 // in seconds
        property bool selfTimerRunning: false

        imageCapture {
            id: imageCapture
            resolution: CameraSettings.resolution
        }

        videoRecorder {
            id: videoRecorder
            resolution: CameraSettings.resolution
            // frameRate: 30 // a fixed frame rate is not set for now as it does not always get enforced anyway and can cause errors
        }

        onError: {
            showPassiveNotification(i18n("An error occurred: \"%1\". Please consider restarting the application if it stopped working.", errorString))
        }
    }

    title: i18n("Camera")
    globalDrawer: GlobalDrawer {
        camera: mainCamera
    }

    pageStack.initialPage: cameraPage
    pageStack.globalToolBar.style: applicationWindow().headerStyle
}
