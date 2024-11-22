// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2013 Digia Plc and/or its subsidiary(-ies)
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import QtMultimedia

import org.kde.kirigami as Kirigami
import org.kde.plasmacamera


Kirigami.ApplicationWindow {
    id: root

    minimumWidth: 350
    title: i18n("Camera")

    pageStack.initialPage: cameraPage
    // TODO: Unable to assign [undefined] to int
    // pageStack.globalToolBar.style: applicationWindow().headerStyle

    PlasmaCamera {
        id: mainCamera

        readonly property int captureStillImage: 1
        readonly property int captureVideo: 2
        property int captureMode: camera.captureStillImage
        property int selfTimerDuration: 0  // seconds
        property bool selfTimerRunning: false

        // ismobile: Kirigami.Settings.isMobile
        // aboutData: aboutData

        active: true

        // two-way binding between CameraSettings and PlasmaCamera for camera device
        // TODO: unsure if this is proper or hacky
        cameraDevice: {
            for (const cameraId of mainCamera.getCameraDevicesId()) {
                if (cameraId == CameraSettings.cameraDeviceId)
                    return cameraId;
            }
        }
        onCameraDeviceChanged: {
            CameraSettings.cameraDeviceId = cameraDevice;
        }

        // one-way binding between CameraSettings and PlasmaCamera for white balance
        // whiteBalanceMode: CameraSettings.whiteBalanceMode

        // TODO: https://forum.qt.io/topic/130485/injection-of-parameters-into-signal-handlers-is-deprecated-use-javascript-functions-with-formal-parameters-instead
        //   - this doesn't work anymore
        onErrorOccurred: {
            showPassiveNotification(i18n("An error occurred: \"%1\". Please consider restarting the application if it stopped working.", errorString));
        }

    }

    Component {
        id: cameraPage

        CameraPage {
            camera: mainCamera
        }

    }

    globalDrawer: GlobalDrawer {
        camera: mainCamera
    }
}

