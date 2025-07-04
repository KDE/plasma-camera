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

Item {
    id: root

    // The capture mode of the camera (photo or video)
    required property int captureMode

    // The controller object for camera capturing
    required property CaptureController captureController

    RowLayout {
        id: captureTimerInfo

        visible: {
            if (root.captureController.captureTimerDuration === 0) {
                return false;
            }
            if (root.captureMode === CameraPage.CaptureMode.Video && captureController.isRecording) {
                return false;
            }
            return true;
        }

        anchors {
            top: parent.top
            horizontalCenter: parent.horizontalCenter
            margins: Kirigami.Units.gridUnit
        }

        Kirigami.Icon {
            id: captureTimerIcon
            source: "alarm-symbolic"
            color: captureController.captureTimerRunning ? "red" : "white"

            Layout.preferredWidth: Kirigami.Units.gridUnit
            Layout.preferredHeight: Kirigami.Units.gridUnit
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignCenter
        }

        Text {
            text: {
                if (captureController.captureTimerRunning)
                    "%1 s".arg(captureController.countdownRemainingSeconds);
                else
                    "%1 s".arg(captureController.captureTimerDuration);
            }
            font.pixelSize: Kirigami.Units.gridUnit
            color: (captureController.captureTimerRunning) ? "red" : "white";
        }
    }

    Rectangle {
        id: captureTimerRectangle

        visible: root.captureController.captureTimerRunning
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
        id: captureTimerAnimation

        running: root.captureController.captureTimerRunning
        loops: Animation.Infinite

        ParallelAnimation {
            OpacityAnimator {
                target: captureTimerIcon
                from: 0
                to: 1
                duration: 500
            }

            OpacityAnimator {
                target: captureTimerRectangle
                from: 0
                to: 1
                duration: 500
            }
        }

        ParallelAnimation {
            OpacityAnimator {
                target: captureTimerIcon
                from: 1
                to: 0
                duration: 500
            }

            OpacityAnimator {
                target: captureTimerRectangle
                from: 1
                to: 0
                duration: 500
            }
        }
    }
}