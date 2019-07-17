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

    leftPadding: 0
    rightPadding: 0
    bottomPadding: 0
    topPadding: 0

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
            if (camera.captureMode === Camera.CaptureStillImage)
                return i18n("Capture photo")
            else if (camera.videoRecorder.recorderStatus === CameraRecorder.RecordingStatus)
                return i18n("Stop recording video")
            else if (camera.captureMode === Camera.CaptureVideo)
                return i18n("Start recording video")
        }
        icon.color: "transparent"
        icon.name: {
            if (camera.captureMode === Camera.CaptureStillImage)
                return "camera-photo-symbolic"
            else if (camera.videoRecorder.recorderStatus === CameraRecorder.RecordingStatus)
                return "window-close-symbolic"
            else if (camera.captureMode === Camera.CaptureVideo)
                return "emblem-videos-symbolic"
        }
        onTriggered: {
            if (camera.captureMode === Camera.CaptureStillImage) {
                camera.imageCapture.capture()
                showPassiveNotification(i18n("Took a photo"))
            }
            else if (camera.videoRecorder.recorderStatus === CameraRecorder.RecordingStatus) {
                camera.videoRecorder.stop()
                recordingFeedback.visible = false
                showPassiveNotification(i18n("Stopped recording"))
            }
            else if (camera.captureMode === Camera.CaptureVideo) {
                camera.videoRecorder.record()
                recordingFeedback.visible = true
                showPassiveNotification(i18n("Started recording"))
            }
        }
        enabled: {
            if (camera.captureMode === camera.CaptureStillImage)
                return camera.imageCapture.ready
            else
                return true
        }
    }
    rightAction: Kirigami.Action {
        id: switchCameaAction
        text: i18n("Switch Camera")
        icon.color: "transparent"
        icon.name: "camera-photo-symbolic"
        enabled: (camera.position !== Camera.UnspecifiedPosition)
        onTriggered: {
            if (settings.cameraPosition == Camera.BackFace)
                settings.cameraPosition = Camera.FrontFace
            else if (settings.cameraPosition == Camera.FrontFace)
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

    Rectangle {
        id: recordingFeedback
        visible: false
        color: "red"
        radius: Kirigami.Units.gridUnit
        height: Kirigami.Units.gridUnit * 2
        width: height

        layer.enabled: recordingFeedback.enabled
        layer.effect: DropShadow {
            color: Material.dropShadowColor
            samples: 30
            spread: 0.5
        }

        anchors {
            left: parent.left
            top: parent.top
            margins: Kirigami.Units.gridUnit * 2
        }
    }

    PreviewArea {
        imageCapture: camera.imageCapture

        anchors {
            right: parent.right
            bottom: parent.bottom
            margins: Kirigami.Units.gridUnit
        }
    }
}
