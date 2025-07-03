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
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None

    function openSettings() {
        settingsDialog.open();
    }

    PlasmaCamera {
        id: mainCamera

        readonly property int captureStillImage: 1
        readonly property int captureVideo: 2
        property int captureMode: camera.captureStillImage
        property int selfTimerDuration: 0  // seconds
        property bool selfTimerRunning: false

        active: true

        function findCameraDevice() {
            for (const cameraId of mainCamera.getCameraDevicesId()) {
                if (cameraId == CameraSettings.cameraDeviceId) {
                    return cameraId;
                }
            }
        }

        cameraDevice: findCameraDevice()

        property Connections cameraDeviceConnection: Connections {
            target: CameraSettings

            function onCameraDeviceIdChanged() {
                mainCamera.cameraDevice = mainCamera.findCameraDevice();
            }
        }
        onCameraDeviceChanged: {
            CameraSettings.cameraDeviceId = mainCamera.cameraDevice;
        }

        // one-way binding between CameraSettings and PlasmaCamera for white balance
        // whiteBalanceMode: CameraSettings.whiteBalanceMode

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

    SettingsDialog {
        id: settingsDialog
        camera: mainCamera
    }
}

