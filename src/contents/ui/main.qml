// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2013 Digia Plc and/or its subsidiary(-ies)
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.7
import org.kde.kirigami 2.2 as Kirigami
import QtMultimedia
import org.kde.plasmacamera 1.0

Kirigami.ApplicationWindow {
    id: root

    minimumWidth: 350
    title: i18n("Camera")
    pageStack.initialPage: cameraPage
    pageStack.globalToolBar.style: applicationWindow().headerStyle

    Component {
        id: cameraPage

        CameraPage {
            camera: mainCamera
        }

    }

    MediaDevices {
        id: devices
    }

    Camera {
        id: mainCamera

        readonly property int captureStillImage: 1
        readonly property int captureVideo: 2
        property int captureMode: mainCamera.captureStillImage
        property int selfTimerDuration: 0 // in seconds
        property bool selfTimerRunning: false

        active: true
        cameraDevice: {
            for (const device of devices.videoInputs) {
                if (device.id == CameraSettings.cameraDeviceId)
                    return device;

            }
            return devices.defaultVideoInput;
        }
        onCameraDeviceChanged: mainCamera.start()
        whiteBalanceMode: CameraSettings.whiteBalanceMode
        onErrorOccurred: {
            showPassiveNotification(i18n("An error occurred: \"%1\". Please consider restarting the application if it stopped working.", errorString));
        }
    }

    globalDrawer: GlobalDrawer {
        camera: mainCamera
    }

}
