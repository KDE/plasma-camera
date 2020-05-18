/****************************************************************************
**
** Copyright (C) 2018 Jonah Brüchert
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.7
import QtMultimedia 5.8
import org.kde.kirigami 2.0 as Kirigami
import QtQuick.Controls 2.0 as Controls
import QtQuick.Controls.Material 2.0
import QtQuick.Layouts 1.2
import QtGraphicalEffects 1.0


Kirigami.Page {
    id: cameraPage

    property var camera

    title: i18n("Camera")

    leftPadding: 0
    rightPadding: 0
    bottomPadding: 0
    topPadding: 0

    globalToolBarStyle: Kirigami.Settings.isMobile ? Kirigami.ApplicationHeaderStyle.None : Kirigami.ApplicationHeaderStyle.ToolBar
    onIsCurrentPageChanged: isCurrentPage && pageStack.depth > 1 && pageStack.pop()

    leftAction: Kirigami.Action {
        id: switchModeAction
        text: i18n("Switch mode")
        icon.color: "transparent"
        icon.name: {
            if (camera.captureMode === Camera.CaptureStillImage)
                return "emblem-videos-symbolic"
            else if (camera.captureMode === Camera.CaptureVideo)
                return "camera-photo-symbolic"
        }
        enabled: (camera.videoRecorder.recorderStatus !== CameraRecorder.RecordingStatus)
        onTriggered: {
            if (camera.captureMode === Camera.CaptureStillImage)
                camera.captureMode = Camera.CaptureVideo
            else
                camera.captureMode = Camera.CaptureStillImage

            console.log("Capture Mode switched")
        }
    }
    mainAction: Kirigami.Action {
        id: captureAction
        text: {
            if (selfTimer.running)
                return i18n("Cancel self-timer")
            else if (camera.captureMode === Camera.CaptureStillImage)
                return i18n("Capture photo")
            else if (camera.videoRecorder.recorderStatus === CameraRecorder.RecordingStatus)
                return i18n("Stop recording video")
            else if (camera.captureMode === Camera.CaptureVideo)
                return i18n("Start recording video")
        }
        icon.color: "transparent"
        icon.name: {
            if (selfTimer.running)
                return "dialog-error-symbolic"
            else if (camera.captureMode === Camera.CaptureStillImage)
                return "camera-photo-symbolic"
            else if (camera.videoRecorder.recorderStatus === CameraRecorder.RecordingStatus)
                return "media-playback-stop"
            else if (camera.captureMode === Camera.CaptureVideo)
                return "emblem-videos-symbolic"
        }
        onTriggered: {
            if (selfTimer.running) {
                selfTimer.stop()
            }
            else if ((camera.selfTimerDuration === 0) || (camera.videoRecorder.recorderStatus === CameraRecorder.RecordingStatus)) {
                selfTimer.onTriggered()
            }
            else {
                countdownTimer.remainingSeconds = camera.selfTimerDuration
                countdownTimer.start()
                selfTimer.start()
            }
        }
        enabled: {
            if ((camera.captureMode === camera.CaptureStillImage) && !selfTimer.running)
                return camera.imageCapture.ready
            else
                return true
        }
    }
    rightAction: Kirigami.Action {
        text: i18n("Switch Camera")
        icon.color: "transparent"
        icon.name: "camera-photo-symbolic"
        enabled: (camera.position !== Camera.UnspecifiedPosition)
        onTriggered: {
            if (settings.cameraPosition === Camera.BackFace)
                settings.cameraPosition = Camera.FrontFace
            else if (settings.cameraPosition === Camera.FrontFace)
                settings.cameraPosition = Camera.BackFace
        }
    }

    Rectangle {
        id: cameraUI
        state: "PhotoCapture"

        anchors {
            fill: parent
            centerIn: parent
        }

        color: "black"

        states: [
            State {
                name: "PhotoCapture"
                StateChangeScript {
                    script: {
                        cameraPage.camera.captureMode = Camera.CaptureStillImage
                        cameraPage.camera.start()
                    }
                }
            },
            State {
                name: "VideoCapture"
                StateChangeScript {
                    script: {
                        cameraPage.camera.captureMode = Camera.CaptureVideo
                        cameraPage.camera.start()
                    }
                }
                
            }
        ]

        Kirigami.Heading {
            anchors.centerIn: parent
            wrapMode: Text.WordWrap
            text: {
                if (cameraPage.camera.availability === Camera.Unavailable)
                    return i18n("Camera not available")
                else if (cameraPage.camera.availability === Camera.Busy)
                    return i18n("Camera is busy. Is another application using it?")
                else if (cameraPage.camera.availability === Camera.ResourceMissing)
                    return i18n("Missing camera resource.")
                else if (cameraPage.camera.availability === Camera.Available)
                    return ""
            }
        }

        VideoOutput {
            id: viewfinder
            visible: cameraUI.state == "PhotoCapture" || cameraUI.state == "VideoCapture"
        
            // Workaround
            orientation: Kirigami.Settings.isMobile ? -90 : 0

            anchors.fill: parent

            source: cameraPage.camera
        }

        PinchArea {
            anchors.fill: parent
            property real initialZoom
            onPinchStarted: {
                initialZoom = cameraPage.camera.digitalZoom;
            }
            onPinchUpdated: {
                var scale = cameraPage.camera.maximumDigitalZoom / 8 * pinch.scale - cameraPage.camera.maximumDigitalZoom / 8;
                cameraPage.camera.setDigitalZoom(Math.min(cameraPage.camera.maximumDigitalZoom, cameraPage.camera.digitalZoom + scale))
            }
        }

        MouseArea {
            anchors.fill: parent

            onClicked: {
                if (cameraPage.camera.lockStatus === cameraPage.camera.Unlocked) {
                    cameraPage.camera.searchAndLock();
                    console.log("searching focus...")
                }
                else {
                    cameraPage.camera.unlock();
                    console.log("unlocking focus...")
                }
            }
        }
    }

    ZoomControl {
        anchors {
            right: parent.right
            margins: Kirigami.Units.gridUnit * 2
        }
        width : Kirigami.Units.gridUnit * 2
        height: parent.height

        currentZoom: cameraPage.camera.digitalZoom
        maximumZoom: Math.min(4.0, cameraPage.camera.maximumDigitalZoom)
        onZoomTo: cameraPage.camera.setDigitalZoom(value)
    }

    Timer { // counts the seconds from the beginning of the current video recording
        id: recordingDurationTimer
        interval: 1000
        running: camera.videoRecorder.recorderStatus === CameraRecorder.RecordingStatus
        repeat: true
        property int recordingDurationSeconds: 0

        onTriggered: {
            recordingDurationSeconds++
        }

        onRunningChanged: {
            if (!running) {
                recordingDurationSeconds = 0
            }
        }
    }

    RowLayout {
        id: recordingFeedback
        visible: (camera.videoRecorder.recorderStatus === CameraRecorder.RecordingStatus)
        spacing: Kirigami.Units.gridUnit

        anchors {
            left: parent.left
            top: parent.top
            margins: Kirigami.Units.gridUnit * 2
        }

        Rectangle {
            color: "red"
            radius: Kirigami.Units.gridUnit
            height: Kirigami.Units.gridUnit * 2
            width: height
        }

        Text {
            text: {
                "%1%2:%3".arg(
                    (Math.trunc(recordingDurationTimer.recordingDurationSeconds / 60) > 59) ? // display hour count only on demand
                    (Math.trunc(Math.trunc(recordingDurationTimer.recordingDurationSeconds / 60) / 60) + ":") :
                    ""
                )
                .arg(
                    (((Math.trunc(recordingDurationTimer.recordingDurationSeconds / 60) % 60) < 10) ? "0" : "") + // zero padding
                    (Math.trunc(recordingDurationTimer.recordingDurationSeconds / 60) % 60)
                )
                .arg(
                    (((recordingDurationTimer.recordingDurationSeconds % 60) < 10) ? "0" : "") + // zero padding
                    (recordingDurationTimer.recordingDurationSeconds % 60)
                )
            }
            font.pixelSize: Kirigami.Units.gridUnit
            color: "white"
        }

        layer.enabled: recordingFeedback.enabled
        layer.effect: DropShadow {
            color: Material.dropShadowColor
            samples: 30
            spread: 0.5
        }
    }

    Timer {
        id: selfTimer
        interval: camera.selfTimerDuration * 1000
        running: false
        repeat: false

        onTriggered: {
            running = false

            if (camera.captureMode === Camera.CaptureStillImage) {
                if (camera.imageCapture.ready) {
                    camera.imageCapture.capture()
                    previewArea.setPhotoPreview()
                    showPassiveNotification(i18n("Took a photo"))
                }
                else {
                    showPassiveNotification(i18n("Failed to take a photo"))
                }
            }
            else if (camera.videoRecorder.recorderStatus === CameraRecorder.RecordingStatus) {
                camera.videoRecorder.stop()
                previewArea.setVideoPreview()
                showPassiveNotification(i18n("Stopped recording"))
            }
            else if (camera.captureMode === Camera.CaptureVideo) {
                camera.videoRecorder.record()

                if (camera.videoRecorder.recorderStatus === CameraRecorder.RecordingStatus) {
                    showPassiveNotification(i18n("Started recording"))
                }
                else {
                    showPassiveNotification(i18n("Failed to start recording"))
                }
            }
        }

        onRunningChanged: {
            if (!running) {
                camera.selfTimerRunning = false
                selfTimerAnimation.stop()
                countdownTimer.stop()
                countdownTimer.remainingSeconds = camera.selfTimerDuration
                selfTimerIcon.opacity = 1
            }
            else {
                camera.selfTimerRunning = true
            }
        }
    }

    Timer { // counts the remaining seconds until the selfTimer invokes the capture action
        id: countdownTimer
        interval: 1000
        running: false
        repeat: true
        property int remainingSeconds: 0

        onTriggered: {
            remainingSeconds--
        }
    }

    RowLayout {
        id: selfTimerInfo
        visible: !(camera.selfTimerDuration === 0) && !((camera.captureMode === Camera.CaptureVideo) && (camera.videoRecorder.recorderStatus === CameraRecorder.RecordingStatus))

        anchors {
            top: parent.top
            horizontalCenter: parent.horizontalCenter
            margins: Kirigami.Units.gridUnit * 1
        }

        Kirigami.Icon {
            id: selfTimerIcon
            source: "alarm-symbolic"
            color: selfTimer.running ? "red" : "white"
            Layout.preferredWidth: Kirigami.Units.gridUnit
            Layout.preferredHeight: Kirigami.Units.gridUnit
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignCenter
        }

        Text {
            text: {
                if (selfTimer.running) {
                    "%1 s".arg(countdownTimer.remainingSeconds)
                }
                else {
                    "%1 s".arg(camera.selfTimerDuration)
                }
            }
            font.pixelSize: Kirigami.Units.gridUnit
            color: {
                if (selfTimer.running) {
                    "red"
                }
                else {
                    "white"
                }
            }
        }

        layer.enabled: selfTimerInfo.enabled
        layer.effect: DropShadow {
            color: Material.dropShadowColor
            samples: 30
            spread: 0.5
        }
    }

    Rectangle {
        id: selfTimerRectangle
        visible: selfTimer.running
        color: "transparent"
        border.color: "red"
        border.width: Kirigami.Units.gridUnit / 6
        opacity: 0

        anchors {
            fill: parent
            centerIn: parent
        }
    }

    SequentialAnimation {
        id: selfTimerAnimation
        running: selfTimer.running
        loops: Animation.Infinite

        ParallelAnimation {
            OpacityAnimator {
                target: selfTimerIcon
                from: 0
                to: 1
                duration: 500
            }
            OpacityAnimator {
                target: selfTimerRectangle
                from: 0
                to: 1
                duration: 500
            }
        }

        ParallelAnimation{
            OpacityAnimator {
                target: selfTimerIcon
                from: 1
                to: 0
                duration: 500
            }
            OpacityAnimator {
                target: selfTimerRectangle
                from: 1
                to: 0
                duration: 500
            }
        }
    }

    PreviewArea {
        id: previewArea
        imageCapture: camera.imageCapture
        videoRecorder: camera.videoRecorder

        anchors {
            right: parent.right
            bottom: parent.bottom
            margins: Kirigami.Units.gridUnit
        }
    }
}
