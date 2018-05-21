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

    leftPadding: 0
    rightPadding: 0
    bottomPadding: 0
    topPadding: 0

    mainAction: Kirigami.Action {
        id: captureAction
        text: {
            if (camera.captureMode == Camera.CaptureStillImage)
                return qsTr("Capture photo")
            else if (camera.videoRecorder.recorderStatus == CameraRecorder.RecordingStatus)
                return qsTr("Stop recording video")
            else if (camera.captureMode == Camera.CaptureVideo)
                return qsTr("Start recording video")
        }
        iconName: {
            if (camera.captureMode == Camera.CaptureStillImage)
                return qsTr("camera-photo")
            else if (camera.videoRecorder.recorderStatus == CameraRecorder.RecordingStatus)
                return qsTr("window-close")
            else if (camera.captureMode == Camera.CaptureVideo)
                return qsTr("video-mp4")
        }
        onTriggered: {
            if (camera.captureMode == Camera.CaptureStillImage) {
                camera.imageCapture.capture()
                showPassiveNotification(qsTr("Took a photo"))
            }
            else if (camera.videoRecorder.recorderStatus == CameraRecorder.RecordingStatus) {
                camera.videoRecorder.stop()
                recordingFeedback.visible = false
                showPassiveNotification(qsTr("Stopped recording"))
            }
            else if (camera.captureMode == Camera.CaptureVideo) {
                camera.videoRecorder.record()
                recordingFeedback.visible = true
                showPassiveNotification(qsTr("Started recording"))
            }
        }
    }
    
    leftAction: Kirigami.Action {
        id: switchAction
        text: qsTr("Switch mode")
        iconName: "document-swap"
        onTriggered: {
            if (camera.captureMode == Camera.CaptureStillImage)
                camera.captureMode = Camera.CaptureVideo
            else
                camera.captureMode = Camera.CaptureStillImage
                
            console.log("Capture Mode switched")
        }
    }

    Rectangle {
        id: cameraUI
        state: "PhotoCapture"

        anchors {
            fill: parent
            centerIn: parent
        }

        color: "darkgrey"

        states: [
            State {
                name: "PhotoCapture"
                StateChangeScript {
                    script: {
                        camera.captureMode = Camera.CaptureStillImage
                        camera.start()
                    }
                }
            },
            State {
                name: "VideoCapture"
                StateChangeScript {
                    script: {
                        camera.captureMode = Camera.CaptureVideo
                        camera.start()
                    }
                }
                
            }
        ]

        Camera {
            id: camera
            captureMode: Camera.CaptureStillImage
            imageProcessing.whiteBalanceMode: CameraImageProcessing.WhiteBalanceFlash

            imageCapture {
                id: imageCapture
            }

            videoRecorder {
                id: videoRecorder
                resolution: settings.videoResolution
                frameRate: 30
            }
        }

        VideoOutput {
            id: viewfinder
            visible: cameraUI.state == "PhotoCapture" || cameraUI.state == "VideoCapture"
        
            // Workaround
            orientation: Kirigami.Settings.isMobile ? -90 : 0

            anchors.fill: parent

            source: camera
        }

        PinchArea {
            anchors.fill: parent
            property real initialZoom
            onPinchStarted: {
                initialZoom = camera.digitalZoom;
            }
            onPinchUpdated: {
                var scale = camera.maximumDigitalZoom / 8 * pinch.scale - camera.maximumDigitalZoom / 8;
                camera.setDigitalZoom(Math.min(camera.maximumDigitalZoom, camera.digitalZoom + scale))
            }
        }

        MouseArea {
            anchors.fill: parent

            onClicked: {
                if (camera.lockStatus == Camera.Unlocked) {
                    camera.searchAndLock();
                    console.log("searching focus...")
                }
                else {
                    camera.unlock();
                    console.log("unlocking focus...")
                }
            }
        }
    }

    Controls.Slider {
        orientation: Qt.Vertical

        from: 1
        value: camera.digitalZoom
        to: Math.min(4.0, camera.maximumDigitalZoom)

        height: Kirigami.Units.gridUnit * 20

        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
        }

        onValueChanged: camera.setDigitalZoom(value)
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

    Rectangle {
        id: preview
        visible: imageCapture.capturedImagePath
        width: Kirigami.Units.gridUnit * 6
        height: width
        layer.enabled: preview.enabled
        layer.effect: DropShadow {
            color: Material.dropShadowColor
            samples: 30
            spread: 0.5
        }

        MouseArea {
            anchors.fill: parent
            onClicked: Qt.openUrlExternally(imageCapture.capturedImagePath)
        }

        anchors {
            right: parent.right
            bottom: parent.bottom
            margins: Kirigami.Units.gridUnit
        }

        Image {
            fillMode: Image.PreserveAspectCrop
            anchors.fill: parent
            source: imageCapture.capturedImagePath
        }
    }
}
