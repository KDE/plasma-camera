// SPDX-FileCopyrightText: 2013 Digia Plc and/or its subsidiary(-ies)
// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtMultimedia

import org.kde.kirigami as Kirigami
import org.kde.plasmacamera
import org.kde.kirigamiaddons.components as Components

Rectangle {
    id: root
    required property PlasmaCameraManager captureSession
    required property CaptureController captureController

    required property int captureMode
    required property bool captureEnabled
    required property bool switchCameraEnabled
    required property bool timerButtonEnabled

    signal captureButtonClicked()
    signal switchCameraRequested()
    signal timerDurationRequested(duration: int)
    signal switchCaptureModeRequested(captureMode: int)

    height: controlsLayout.implicitHeight
    color: Qt.rgba(0, 0, 0, 0.3)

    ColumnLayout {
        id: controlsLayout
        width: parent.width

        // Select timer strip
        TimerSelectStrip {
            id: timerSelectStrip
            Layout.fillWidth: true

            shown: false // Set by timerButton

            onTimerDurationRequested: (duration) => {
                root.timerDurationRequested(duration);
            }
        }

        // Main footer controls row
        RowLayout {
            spacing: Kirigami.Units.gridUnit
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter

            Item {
                Layout.fillWidth: true
            }

            // Captured photo/video preview
            Item {
                id: capturedPreview
                implicitWidth: 40
                implicitHeight: 40

                PreviewArea {
                    id: previewArea
                    anchors.fill: parent

                    videoRecorder: root.captureSession.recorder

                    Component.onCompleted: {
                        root.captureSession.imageSaved.connect(updatePreview)
                    }
                    function updatePreview(_, fileName: string) {
                        previewArea.imageUrl = "file://" + fileName
                    }

                    // Listen for photo capture and video capture events
                    Connections {
                        target: root.captureController

                        function onPhotoCaptured() {
                            previewArea.setPhotoPreview();
                        }
                        function onVideoCaptured() {
                            previewArea.setVideoPreview();
                        }
                    }
                }

                Kirigami.Icon {
                    visible: !previewArea.visible
                    anchors.centerIn: parent
                    source: "photo"
                    color: "white"
                    height: Kirigami.Units.gridUnit * 1.3
                }
            }

            QQC2.ToolButton {
                id: timerButton
                enabled: root.timerButtonEnabled
                icon.name: "clock"
                icon.color: "white"

                Layout.fillWidth: true
                Layout.maximumWidth: height
                onClicked: {
                    timerSelectStrip.shown = !timerSelectStrip.shown;
                }
            }

            // Capture photo/video button
            CaptureButton {
                id: captureButton
                captureMode: root.captureMode
                isRecording: captureController.isRecording
                enabled: root.captureEnabled
                onClicked: root.captureButtonClicked()
            }

            QQC2.ToolButton {
                id: switchCameraButton
                icon.name: "circular-arrow-shape"
                icon.color: "white"

                Layout.fillWidth: true
                Layout.maximumWidth: height

                enabled: root.switchCameraEnabled
                opacity: enabled ? 1 : 0 // use opacity instead of visible, so space is still used
                onClicked: root.switchCameraRequested()
            }

            Item {
                implicitWidth: 40
                implicitHeight: 40

                QQC2.ToolButton {
                    id: settingsButton

                    icon.name: "settings-configure"
                    icon.color: "white"
                    anchors.centerIn: parent
                    onClicked: applicationWindow().openSettings()
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }

        Components.RadioSelector {
            id: modeSelector

            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing * 3
            Layout.maximumWidth: Kirigami.Units.gridUnit * 10
            consistentWidth: true

            actions: [
                Kirigami.Action {
                    text: i18n("Photo")
                    icon.name: "camera-photo-symbolic"
                    onTriggered: root.switchCaptureModeRequested(CameraPage.CaptureMode.Photo)
                },
                Kirigami.Action {
                    text: i18n("Video")
                    icon.name: "emblem-videos-symbolic"
                    onTriggered: root.switchCaptureModeRequested(CameraPage.CaptureMode.Video)
                }
            ]
        }
    }
}