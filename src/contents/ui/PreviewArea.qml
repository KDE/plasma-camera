import QtQuick 2.7
import QtMultimedia 5.8
import QtQuick.Controls.Material 2.0
import QtQuick.Controls 2.0 as Controls
import org.kde.kirigami 2.4 as Kirigami
import QtGraphicalEffects 1.0


Rectangle {
    id: preview

    property var imageCapture
    property var videoRecorder
    property bool showVideoPreview: false // when set to true, the preview for the videoRecorder is shown, if false for the imageCapture
    property bool videoThumbnailRequested: false

    visible: ((imageCapture.capturedImagePath && !showVideoPreview) || (videoRecorder.actualLocation && showVideoPreview)) && !(videoRecorder.recorderStatus === CameraRecorder.RecordingStatus)
    width: Kirigami.Units.gridUnit * 6
    height: width
    layer.enabled: preview.enabled
    layer.effect: DropShadow {
        color: Material.dropShadowColor
        samples: 30
        spread: 0.5
    }

    Component.onCompleted: {
        videoRecorder.recorderStatusChanged.connect(createVideoThumbnail)
    }

    function setPhotoPreview() {
        showVideoPreview = false
    }

    function setVideoPreview() {
        videoThumbnailRequested = true
        showVideoPreview = true
    }

    function createVideoThumbnail() {
        if (videoThumbnailRequested && !(videoRecorder.recorderStatus === CameraRecorder.FinalizingStatus)) {
            video.source = videoRecorder.actualLocation
            video.play()
            video.pause()
            videoThumbnailRequested = false
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (showVideoPreview) {
                Qt.openUrlExternally(videoRecorder.actualLocation)
            }
            else {
                Qt.openUrlExternally("file://" + imageCapture.capturedImagePath)
            }
        }
    }

    Image {
        visible: !showVideoPreview
        fillMode: Image.PreserveAspectCrop
        anchors.fill: parent
        source: imageCapture.capturedImagePath ? "file://" + imageCapture.capturedImagePath : ""
    }

    Rectangle {
        visible: showVideoPreview
        anchors.fill: parent
        color: "grey"

        Video {
            id: video
            anchors.fill: parent
            muted: true
            fillMode: VideoOutput.PreserveAspectCrop
        }

        Controls.BusyIndicator {
            id: thumbnailBusyIdicator
            visible: (videoRecorder.recorderStatus === CameraRecorder.FinalizingStatus)
            Kirigami.Theme.textColor: "white"
            anchors.fill: parent

            layer.enabled: thumbnailBusyIdicator.enabled
            layer.effect: DropShadow {
                color: Material.dropShadowColor
                samples: 30
                spread: 0.5
            }
        }

        Kirigami.Icon {
            id: playbackIcon
            visible: !thumbnailBusyIdicator.visible
            source: "media-playback-start"
            color: "white"
            width: parent.width * 0.75
            height: width
            anchors.centerIn: parent

            layer.enabled: playbackIcon.enabled
            layer.effect: DropShadow {
                color: Material.dropShadowColor
                samples: 30
                spread: 0.5
            }
        }
    }
}
