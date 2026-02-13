// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Sebastian Pettke <sebpe@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtMultimedia
import QtQuick.Controls as Controls
import QtQuick.Effects

import org.kde.kirigami as Kirigami

Rectangle {
    id: preview

    required property MediaRecorder videoRecorder
    required property bool isSavingVideo

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
        if (videoThumbnailRequested && !preview.isSavingVideo) {
            video.source = videoRecorder.actualLocation;
            video.play();
            video.pause();
            videoThumbnailRequested = false;
        }
    }

    visible: true
    width: Kirigami.Units.gridUnit * 6
    height: width

    Component.onCompleted: {
        videoRecorder.recorderStateChanged.connect(createVideoThumbnail);
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: {
            if (showVideoPreview)
                Qt.openUrlExternally(videoRecorder.actualLocation);
            else
                Qt.openUrlExternally(preview.imageUrl);
        }
    }

    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary
    color: Kirigami.Theme.backgroundColor
    radius: Kirigami.Units.cornerRadius

    border.color: Qt.lighter(preview.color, 1.3)
    border.width: 1

    Rectangle {
        id: mask
        anchors.fill: parent
        visible: false
        color: "black"
        radius: Kirigami.Units.cornerRadius
        layer.enabled: true
    }

    Image {
        id: imagePreview
        fillMode: Image.PreserveAspectCrop
        anchors.fill: parent
        source: preview.imageUrl
        visible: status == Image.Ready

        layer.enabled: true
        layer.effect: MultiEffect {
            maskEnabled: true
            maskSource: mask
        }

        onSourceChanged: {
            if (status === Image.Ready) {
                squishAnim.restart();
            }
        }

        transform: Scale {
            id: scaleTransform
            origin.x: imagePreview.width / 2
            origin.y: imagePreview.height / 2
            xScale: 1
            yScale: 1
        }

        SequentialAnimation {
            id: squishAnim
            running: false

            ParallelAnimation {
                PropertyAnimation {
                    target: scaleTransform
                    property: "xScale"
                    to: 0.7
                    duration: 100
                    easing.type: Easing.InExpo
                }
                PropertyAnimation {
                    target: scaleTransform
                    property: "yScale"
                    to: 0.7
                    duration: 100
                    easing.type: Easing.InExpo
                }
            }

            ParallelAnimation {
                PropertyAnimation {
                    target: scaleTransform
                    property: "xScale"
                    to: 1
                    duration: 200
                    easing.type: Easing.OutExpo
                }
                PropertyAnimation {
                    target: scaleTransform
                    property: "yScale"
                    to: 1
                    duration: 200
                    easing.type: Easing.OutExpo
                }
            }
        }
    }

    Rectangle {
        visible: showVideoPreview
        anchors.fill: parent
        color: "grey"

        layer.enabled: true
        layer.effect: MultiEffect {
            maskEnabled: true
            maskSource: mask
        }

        Video {
            id: video

            anchors.fill: parent
            muted: true
            fillMode: VideoOutput.PreserveAspectCrop
        }

        Controls.BusyIndicator {
            id: thumbnailBusyIdicator

            visible: preview.isSavingVideo
            Kirigami.Theme.textColor: "white"
            anchors.fill: parent
            layer.enabled: thumbnailBusyIdicator.enabled
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
        }
    }

    Rectangle {
        id: pressFeedback
        anchors.fill: parent
        visible: mouseArea.pressed
        color: Qt.rgba(0, 0, 0, 0.3)
        radius: Kirigami.Units.cornerRadius
    }
}
