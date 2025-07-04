// SPDX-FileCopyrightText: 2025 Andrew Wang
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

    required property MediaRecorder videoRecorder

    property bool showVideoPreview: false // when set to true, the preview for the videoRecorder is shown, if false for the imageCapture
    property bool videoThumbnailRequested: false
    property string imageUrl  // TODO: set default image url to some placeholder? (the last picture you took)

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

    visible: true
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
        visible: true
        fillMode: Image.PreserveAspectCrop
        anchors.fill: parent
        source: preview.imageUrl
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

            // TODO
            // visible: (videoRecorder.recorderState === MediaRecorder.FinalizingStatus)
            visible: false
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
