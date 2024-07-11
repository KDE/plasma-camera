// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Sebastian Pettke <sebpe@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtMultimedia
import QtQuick.Controls.Material
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import Qt5Compat.GraphicalEffects

Rectangle {
    id: preview

    property var imageCapture
    property var videoRecorder
    property bool showVideoPreview: false // when set to true, the preview for the videoRecorder is shown, if false for the imageCapture
    property bool videoThumbnailRequested: false
    property string imageUrl

    function setPhotoPreview() {
        showVideoPreview = false;
    }

    function setVideoPreview() {
        videoThumbnailRequested = true;
        showVideoPreview = true;
    }

    function createVideoThumbnail() {
        if (videoThumbnailRequested && !(videoRecorder.recorderState === MediaRecorder.FinalizingStatus)) {
            video.source = videoRecorder.actualLocation;
            video.play();
            video.pause();
            videoThumbnailRequested = false;
        }
    }

    visible: ((imageCapture.preview && !showVideoPreview) || (videoRecorder.actualLocation && showVideoPreview)) && !(videoRecorder.recorderState === MediaRecorder.RecordingStatus)
    width: Kirigami.Units.gridUnit * 6
    height: width
    layer.enabled: preview.enabled
    Component.onCompleted: {
        videoRecorder.recorderStateChanged.connect(createVideoThumbnail);
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (showVideoPreview)
                Qt.openUrlExternally(videoRecorder.actualLocation);
            else
                Qt.openUrlExternally(preview.imageUrl);
        }
    }

    Image {
        visible: !showVideoPreview
        fillMode: Image.PreserveAspectCrop
        anchors.fill: parent
        source: imageCapture.preview
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

            visible: (videoRecorder.recorderState === MediaRecorder.FinalizingStatus)
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

    layer.effect: DropShadow {
        color: Material.dropShadowColor
        samples: 30
        spread: 0.5
    }

}
