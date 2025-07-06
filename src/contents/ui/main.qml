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

import "camera"

Kirigami.ApplicationWindow {
    id: root

    minimumWidth: 350
    title: i18n("Camera")

    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None
    pageStack.initialPage: CameraPage {
        id: cameraPage
        camera: mainCamera
    }

    function openSettings() {
        settingsDialog.open();
    }

    PlasmaCamera {
        id: mainCamera

        Component.onCompleted: {
            // Restore selected camera from settings
            const cameraId = findSettingsCameraDevice();
            if (cameraId.length > 0) {
                cameraDevice = cameraId;
            }
            // Start camera
            mainCamera.startCamera();
        }

        // Returns which camera to select from settings, and nothing if the camera can't be found.
        function findSettingsCameraDevice(): string {
            for (const cameraId of mainCamera.cameraDeviceIds) {
                if (cameraId == CameraSettings.cameraDeviceId) {
                    return cameraId;
                }
            }
            return "";
        }

        property Connections cameraDeviceConnection: Connections {
            target: CameraSettings

            // Listen to when the camera settings change, and update
            function onCameraDeviceIdChanged() {
                const cameraId = mainCamera.findSettingsCameraDevice();
                if (cameraId.length > 0) {
                    mainCamera.cameraDevice = cameraId;
                }
            }
        }
        onCameraDeviceChanged: {
            // Bind settings to what the camera device is
            CameraSettings.cameraDeviceId = mainCamera.cameraDevice;
        }

        onErrorOccurred: {
            showPassiveNotification(i18n("An error occurred: \"%1\". Please consider restarting the application if it stopped working.", mainCamera.errorString));
        }
    }

    SettingsDialog {
        id: settingsDialog
        camera: mainCamera
        cameraManager: cameraPage.captureSession
    }
}

