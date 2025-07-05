// SPDX-FileCopyrightText: 2013 Digia Plc and/or its subsidiary(-ies)
// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtMultimedia

import org.kde.kirigami as Kirigami

import org.kde.plasmacamera

Kirigami.Page {
    id: root

    title: i18n("Camera")
    leftPadding: 0
    rightPadding: 0
    bottomPadding: 0
    topPadding: 0

    globalToolBarStyle: Kirigami.ApplicationHeaderStyle.None

    required property PlasmaCamera camera

    enum CaptureMode {
        Photo,
        Video
    }

    // The current mode of capturing the camera is going to do
    property int captureMode: CameraPage.CaptureMode.Photo

    property PlasmaCameraManager captureSession: PlasmaCameraManager {
        id: captureSession
        plasmaCamera: root.camera
        videoSink: viewfinder.videoSink

        recorder: MediaRecorder {}
    }

    property CaptureController captureController: CaptureController {
        id: captureController

        camera: root.camera
        captureSession: root.captureSession
        captureMode: root.captureMode
    }

    // These actions are used for functional purposes, but are not actually visible
    actions: [
        Kirigami.Action {
            id: switchModeAction

            visible: false
            text: i18n("Switch mode")
            icon.color: "transparent"
            icon.name: {
                if (root.captureMode === CameraPage.CaptureMode.Photo)
                    return "emblem-videos-symbolic";
                else if (root.captureMode === CameraPage.CaptureMode.Video)
                    return "camera-photo-symbolic";
            }
            enabled: true
            onTriggered: {
                if (root.captureMode === CameraPage.CaptureMode.Photo)
                    root.captureMode = CameraPage.CaptureMode.Video;
                else
                    root.captureMode = CameraPage.CaptureMode.Photo;
            }
        },
        Kirigami.Action {
            id: captureAction

            visible: false
            text: root.captureController.intendedText
            icon.color: "transparent"
            icon.name: root.captureController.intendedIcon
            onTriggered: root.captureController.requestCapture()
            enabled: !camera.busy && !root.captureController.captureTimerRunning
        },
        Kirigami.Action {
            id: switchCameraAction

            visible: false
            text: i18n("Switch Camera")
            icon.color: "transparent"
            icon.name: "camera-photo-symbolic"
            enabled: !root.camera.busy && root.camera.cameraDeviceIds.length > 1

            onTriggered: root.camera.switchToNextCamera();
        }
    ]

    Viewfinder {
        id: viewfinder
        anchors.fill: parent

        camera: root.camera
        bottomMargin: controlContainer.height

        // Header controls
        CameraExtraControls {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            exposureValueEnabled: root.camera.exposureValueAvailable
            onExposureValueRequested: (value) => root.camera.exposureValue = value;
        }

        // Footer controls
        CameraControls {
            id: controlContainer
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right

            captureSession: root.captureSession
            captureController: root.captureController

            captureMode: root.captureMode
            captureEnabled: captureAction.enabled
            switchCameraEnabled: switchCameraAction.enabled
            timerButtonEnabled: !root.camera.busy

            onCaptureButtonClicked: captureAction.triggered()
            onSwitchCameraRequested: switchCameraAction.triggered()
            onTimerDurationRequested: (duration) => {
                root.captureController.captureTimerDuration = duration;
            }
            onSwitchCaptureModeRequested: (captureMode) => {
                root.captureMode = captureMode;
            }
        }

        // Indicator for timer
        ViewfinderTimerOverlay {
            id: timerOverlay
            anchors.fill: parent

            captureMode: root.captureMode
            captureController: root.captureController
        }
    }
}
