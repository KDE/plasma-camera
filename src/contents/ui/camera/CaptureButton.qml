// SPDX-FileCopyrightText: 2013 Digia Plc and/or its subsidiary(-ies)
// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

import org.kde.plasmacamera

QQC2.ToolButton {
    id: root

    // The current capture mode of the camera (CameraPage.CaptureMode)
    required property int captureMode

    // Whether a video is currently being recorded
    required property bool isRecording

    implicitWidth: Kirigami.Units.gridUnit * 4
    implicitHeight: Kirigami.Units.gridUnit * 4

    background: Rectangle {
        height: parent.height
        width: height
        radius: height / 2
        border.color: "white"
        border.width: {
            if (parent.down) {
                return parent.height / 2;
            }
            if (parent.hovered && !Kirigami.Settings.isMobile) {
                return 10;
            }
            return 5;
        }

        color: {
            if (root.captureMode === CameraPage.CaptureMode.Photo) {
                return Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.hoverColor, "transparent", 0.6);
            }
            return Kirigami.Theme.negativeTextColor;
        }

        // Recording indicator
        Rectangle {
            opacity: (root.captureMode === CameraPage.CaptureMode.Video) ? 1 : 0
            height: (root.captureMode === CameraPage.CaptureMode.Video) ? Kirigami.Units.gridUnit : 0
            width: height
            anchors.centerIn: parent

            radius: root.isRecording ? Kirigami.Units.smallSpacing : width / 2

            Behavior on opacity {
                PropertyAnimation {
                    duration: Kirigami.Units.shortDuration
                    easing.type: Easing.InOutCubic
                }
            }

            Behavior on height {
                PropertyAnimation {
                    duration: Kirigami.Units.shortDuration
                    easing.type: Easing.InOutCubic
                }
            }
        }

        Behavior on border.width {
            PropertyAnimation {
                duration: Kirigami.Units.shortDuration
                easing.type: Easing.InOutCubic
            }
        }

        Behavior on color {
            PropertyAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.InOutCubic
            }
        }
    }
}