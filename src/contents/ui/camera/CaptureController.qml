// SPDX-FileCopyrightText: 2013 Digia Plc and/or its subsidiary(-ies)
// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick
import QtMultimedia

import org.kde.kirigami as Kirigami
import org.kde.plasmacamera

QtObject {
    id: root

    required property PlasmaCamera camera
    required property PlasmaCameraManager captureSession
    required property int captureMode

    // The duration that the capture timer is set for (in ms), can be changed
    property real captureTimerDuration: 0

    // The remaining seconds of the countdown
    readonly property int countdownRemainingSeconds: countdownTimer.countdownRemainingSeconds

    // Whether the capture session is currently recording a video
    readonly property bool isRecording: root.captureSession.isRecordingVideo

    // Whether the capture session is currently saving a video
    readonly property bool isSaving: root.captureSession.isSavingVideo

    // Whether the capture timer is running
    readonly property bool captureTimerRunning: captureTimer.running

    // The intended text for the current capture state
    readonly property string intendedText: {
        if (captureTimerRunning)
            return i18n("Cancel self-timer");
        else if (root.captureMode === CameraPage.CaptureMode.Photo)
            return i18n("Capture photo");
        else if (root.isRecording)
            return i18n("Stop recording video");
        else if (root.captureMode === CameraPage.CaptureMode.Video)
            return i18n("Start recording video");
    }

    // The intended text for the current capture state
    readonly property string intendedIcon: {
        if (captureTimerRunning)
            return "dialog-error-symbolic";
        else if (root.captureMode === CameraPage.CaptureMode.Photo)
            return "camera-photo-symbolic";
        else if (root.isRecording)
            return "media-playback-stop";
        else if (root.captureMode === CameraPage.CaptureMode.Video)
            return "emblem-videos-symbolic";
    }

    // Call when the capture button is clicked
    function requestCapture() {
        if (captureTimer.running) {
            // Cancel photo/video when countdown timer is running
            captureTimer.stop();
        } else if ((root.captureTimerDuration === 0) || root.isRecording) {
            // If there is no countdown timer, immediately capture photo/video
            captureTimer.triggered();
        } else {
            // Start countdown timer
            countdownTimer.countdownRemainingSeconds = root.captureTimerDuration;
            countdownTimer.restart();
            captureTimer.restart();
        }
    }

    // Emitted when a photo was captured
    signal photoCaptured()

    // Emitted when a video was captured
    signal videoCaptured()

    property Timer captureTimer: Timer {
        id: captureTimer

        interval: root.captureTimerDuration * 1000
        running: false
        repeat: false

        onTriggered: {
            if (root.captureMode === CameraPage.CaptureMode.Photo) {
                if (root.captureSession.readyForCapture) {
                    root.captureSession.captureImageToFile("");
                    console.log("Took a photo");
                    root.photoCaptured();
                } else {
                    showPassiveNotification(i18n("Failed to take a photo"));
                }

            } else if (root.isRecording) {
                root.captureSession.stopRecordingVideo();
                console.log("Stopped recording");
                root.videoCaptured();

            } else if (root.captureMode === CameraPage.CaptureMode.Video) {
                root.captureSession.startRecordingVideo();
                if (root.isRecording)
                    console.log("Started recording");
                else
                    showPassiveNotification(i18n("Failed to start recording"));
            }
        }

        onRunningChanged: {
            if (!running) {
                countdownTimer.stop();
            }
        }
    }

    // Counts the remaining seconds until the captureTimer invokes the capture action
    property Timer countdownTimer: Timer {
        id: countdownTimer
        property int countdownRemainingSeconds: 0

        interval: 1000
        running: false
        repeat: true

        onTriggered: {
            countdownTimer.countdownRemainingSeconds--;
        }
    }
}