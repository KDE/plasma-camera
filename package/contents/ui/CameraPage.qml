import QtQuick 2.7
import QtMultimedia 5.8
import org.kde.kirigami 2.0 as Kirigami
import QtQuick.Controls 2.0 as Controls
import QtQuick.Layouts 1.2

Kirigami.Page {
    id: cameraPage
    
    leftPadding: 0
    rightPadding: 0
    bottomPadding: 0
    topPadding: 0

    mainAction: Kirigami.Action {
        id: captureAction
        text: qsTr("Capture")
        iconName: {
            if (camera.captureMode == Camera.CaptureStillImage)
                return "camera-photo"
            else if (camera.captureMode == Camera.CaptureVideo)
                return "video-mp4"
        }
        onTriggered: camera.imageCapture.capture()
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
                frameRate: 15
            }
        }

        VideoPreview {
            id : videoPreview
            anchors.fill : parent
            onClosed: cameraUI.state = "VideoCapture"
            visible: cameraUI.state == "VideoPreview"
            focus: visible

            //don't load recorded video if preview is invisible
            source: visible ? camera.videoRecorder.actualLocation : ""
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
    }

    Controls.Slider {
        orientation: Qt.Vertical
        from: 1
        value: camera.digitalZoom
        to: Math.min(4.0, camera.maximumDigitalZoom)

        height: Kirigami.Units.gridUnit * 30

        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
        }

        onValueChanged: camera.setDigitalZoom(value)
    }
}
