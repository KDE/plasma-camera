// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2
import QtMultimedia

import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.plasmacamera


Kirigami.MenuDialog {
    id: root
    title: i18n("Settings")

    property PlasmaCamera camera
    property PlasmaCameraManager cameraManager

    actions: [
        Kirigami.Action {
            text: i18n("Select camera")
            icon.name: "camera-photo-symbolic"

            onTriggered: {
                selectCameraDialogLoader.active = true;
                selectCameraDialogLoader.item.open();
            }
        },

        Kirigami.Action {
            text: i18n("Video recording resolution")
            icon.name: 'view-fullscreen-symbolic'

            onTriggered: {
                selectVideoResolutionDialogLoader.active = true;
                selectVideoResolutionDialogLoader.item.open();
            }
        },

        Kirigami.Action {
            text: i18n("Video recording quality")
            icon.name: 'kstars_stars-symbolic' // TODO

            onTriggered: {
                selectVideoRecordingQualityLoader.active = true;
                selectVideoRecordingQualityLoader.item.open();
            }
        },

        Kirigami.Action {
            text: i18n("Video recording frame rate")
            icon.name: 'emblem-videos-symbolic'

            onTriggered: {
                videoFpsDialogLoader.active = true;
                videoFpsDialogLoader.item.open();
            }
        },

        Kirigami.Action {
            text: i18n("Video recording codec")
            icon.name: 'show-gpu-effects-symbolic' // TODO

            onTriggered: {
                videoCodecDialogLoader.active = true;
                videoCodecDialogLoader.item.open();
            }
        },

        Kirigami.Action {
            text: i18n("Filename pattern")
            icon.name: "document-save-as-symbolic"
            onTriggered: {
                filenamePatternDialogLoader.active = true;
                filenamePatternDialogLoader.item.open();
            }
        },

        Kirigami.Action {
            text: i18n("Output path")
            icon.name: "document-open-folder-symbolic"
            onTriggered: {
                outputPathDialogLoader.active = true;
                outputPathDialogLoader.item.open();
            }
        },

        Kirigami.Action {
            text: i18n("About")
            icon.name: "help-about"
            onTriggered: {
                if (Kirigami.Settings.isMobile) {
                    applicationWindow().pageStack.push(aboutPage);
                } else {
                    // HACK: pushDialogLayer is bugged because there's no way to exit the page on mobile
                    applicationWindow().pageStack.pushDialogLayer(aboutPage);
                }
            }
        }
    ]

    property Loader selectVideoResolutionDialogLoader: Loader {
        id: selectVideoResolutionDialogLoader
        active: false

        sourceComponent: Kirigami.Dialog {
            id: selectVideoResolutionDialog
            title: i18n("Video Recording Resolution")
            preferredWidth: Kirigami.Units.gridUnit * 16

            onClosed: selectVideoResolutionDialogLoader.active = false

            ColumnLayout {
                spacing: 0

                Repeater {
                    model: [
                        { name: i18n("Auto"), value: PlasmaCameraManager.ResolutionAuto },
                        { name: "540p", value: PlasmaCameraManager.Resolution540p },
                        { name: "720p", value: PlasmaCameraManager.Resolution720p },
                        { name: "1080p", value: PlasmaCameraManager.Resolution1080p },
                        { name: "1440p", value: PlasmaCameraManager.Resolution1440p },
                        { name: "2160p", value: PlasmaCameraManager.Resolution2160p },
                    ]

                    delegate: QQC2.RadioDelegate {
                        id: radioDelegate
                        property string name: modelData.name
                        property int value: modelData.value

                        Layout.fillWidth: true
                        topPadding: Kirigami.Units.smallSpacing * 2
                        bottomPadding: Kirigami.Units.smallSpacing * 2

                        text: name
                        checked: value == root.cameraManager.videoResolution
                        onCheckedChanged: {
                            if (checked) {
                                root.cameraManager.videoResolution = radioDelegate.value;
                                checked = Qt.binding(() => (radioDelegate.value == root.cameraManager.videoResolution));
                            }
                        }
                    }
                }
            }
        }
    }

    property Loader qualityLoader: Loader {
        id: selectVideoRecordingQualityLoader
        active: false

        sourceComponent: Kirigami.Dialog {
            id: selectVideoRecordingDialog
            title: i18n("Video Recording Quality")
            preferredWidth: Kirigami.Units.gridUnit * 16

            onClosed: selectVideoRecordingQualityLoader.active = false

            ColumnLayout {
                spacing: 0

                Repeater {
                    model: [
                        { name: i18n("Very low quality"), value: PlasmaCameraManager.VeryLowQuality },
                        { name: i18n("Low quality"), value: PlasmaCameraManager.LowQuality },
                        { name: i18n("Normal quality"), value: PlasmaCameraManager.NormalQuality },
                        { name: i18n("High quality"), value: PlasmaCameraManager.HighQuality },
                        { name: i18n("Very high quality"), value: PlasmaCameraManager.VeryHighQuality },
                    ]

                    delegate: QQC2.RadioDelegate {
                        id: radioDelegate
                        property string name: modelData.name
                        property int value: modelData.value

                        Layout.fillWidth: true
                        topPadding: Kirigami.Units.smallSpacing * 2
                        bottomPadding: Kirigami.Units.smallSpacing * 2

                        text: name
                        checked: value == root.cameraManager.quality
                        onCheckedChanged: {
                            if (checked) {
                                root.cameraManager.quality = radioDelegate.value;
                                checked = Qt.binding(() => (radioDelegate.value == root.cameraManager.quality));
                            }
                        }
                    }
                }
            }
        }
    }

    property Loader videoFpsDialogLoader: Loader {
        id: videoFpsDialogLoader
        active: false

        sourceComponent: Kirigami.Dialog {
            id: selectVideoRecordingDialog
            title: i18n("Video Recording Framerate")
            preferredWidth: Kirigami.Units.gridUnit * 16

            onClosed: videoFpsDialogLoader.active = false

            ColumnLayout {
                spacing: 0

                Repeater {
                    model: [
                        { name: "24 FPS", value: 24 },
                        { name: "30 FPS", value: 30 },
                        { name: "60 FPS", value: 60 },
                    ]

                    delegate: QQC2.RadioDelegate {
                        id: radioDelegate
                        property string name: modelData.name
                        property int value: modelData.value

                        Layout.fillWidth: true
                        topPadding: Kirigami.Units.smallSpacing * 2
                        bottomPadding: Kirigami.Units.smallSpacing * 2

                        text: name
                        checked: value == root.cameraManager.videoRecordingFps
                        onCheckedChanged: {
                            if (checked) {
                                root.cameraManager.videoRecordingFps = radioDelegate.value;
                                checked = Qt.binding(() => (radioDelegate.value == root.cameraManager.videoRecordingFps));
                            }
                        }
                    }
                }
            }
        }
    }

    property Loader videoCodecDialogLoader: Loader {
        id: videoCodecDialogLoader
        active: false

        sourceComponent: Kirigami.Dialog {
            id: videoCodecDialog
            title: i18n("Video Recording Codec")
            preferredWidth: Kirigami.Units.gridUnit * 16

            onClosed: videoFpsDialogLoader.active = false

            ColumnLayout {
                spacing: 0

                Repeater {
                    model: [
                        { name: 'H264 (Recommended)', value: PlasmaCameraManager.H264 },
                        { name: 'H265', value: PlasmaCameraManager.H265 },
                        { name: 'MPEG2 (Fastest)', value: PlasmaCameraManager.MPEG2 },
                    ]

                    delegate: QQC2.RadioDelegate {
                        id: radioDelegate
                        property string name: modelData.name
                        property int value: modelData.value

                        Layout.fillWidth: true
                        topPadding: Kirigami.Units.smallSpacing * 2
                        bottomPadding: Kirigami.Units.smallSpacing * 2

                        text: name
                        checked: value == root.cameraManager.videoCodec
                        onCheckedChanged: {
                            if (checked) {
                                root.cameraManager.videoCodec = radioDelegate.value;
                                checked = Qt.binding(() => (radioDelegate.value == root.cameraManager.videoCodec));
                            }
                        }
                    }
                }
            }
        }
    }

    property Loader cameraDialogLoader: Loader {
        id: selectCameraDialogLoader
        active: false

        sourceComponent: Kirigami.Dialog {
            id: selectCameraDialog
            title: i18n("Select Camera")
            preferredWidth: Kirigami.Units.gridUnit * 16

            onClosed: selectCameraDialogLoader.active = false

            ColumnLayout {
                spacing: 0
                Repeater {
                    model: {
                        let cameraIds = root.camera.cameraDeviceIds;
                        let cameraNames = root.camera.cameraDeviceNames;

                        let list = [];
                        for (let i = 0; i < cameraIds.length; i++) {
                            list.push({ "value": cameraIds[i], "name": cameraNames[i] });
                        }
                        return list;
                    }

                    delegate: QQC2.RadioDelegate {
                        property string name: modelData.name
                        property string value: modelData.value

                        Layout.fillWidth: true
                        topPadding: Kirigami.Units.smallSpacing * 2
                        bottomPadding: Kirigami.Units.smallSpacing * 2

                        text: name
                        checked: value == CameraSettings.cameraDeviceId
                        onCheckedChanged: {
                            if (checked) {
                                CameraSettings.cameraDeviceId = value;
                                checked = Qt.binding(() => (value == CameraSettings.cameraDeviceId));
                            }
                        }
                    }
                }
            }
        }
    }

    property Loader filenamePatternDialogLoader: Loader {
        id: filenamePatternDialogLoader
        active: false

        sourceComponent: Kirigami.Dialog {
            id: filenamePatternDialog
            title: i18n("Output filename pattern")
            preferredWidth: Kirigami.Units.gridUnit * 16

            onClosed: filenamePatternDialogLoader.active = false

            ColumnLayout {
                spacing: 0

                Repeater {
                    model: [
                        { name: 'image_0001', value: PlasmaCameraManager.FilenameNumericSequential },
                        { name: 'IMG(date)', value: PlasmaCameraManager.FilenameDateShort },
                        { name: 'IMG_(date)', value: PlasmaCameraManager.FilenameDateUnderscore },
                    ]

                    delegate: QQC2.RadioDelegate {
                        id: radioDelegate
                        property string name: modelData.name
                        property int value: modelData.value

                        Layout.fillWidth: true
                        topPadding: Kirigami.Units.smallSpacing * 2
                        bottomPadding: Kirigami.Units.smallSpacing * 2

                        text: name
                        checked: value == root.cameraManager.filenamePattern
                        onCheckedChanged: {
                            if (checked) {
                                root.cameraManager.filenamePattern = value;
                                checked = Qt.binding(() => (value == root.cameraManager.filenamePattern));
                            }
                        }
                    }
                }
            }
        }
    }

    property Loader outputPathDialogLoader: Loader {
        id: outputPathDialogLoader
        active: false

        sourceComponent: Kirigami.Dialog {
            id: outputPathDialog
            title: i18n("Output path")
            preferredWidth: Kirigami.Units.gridUnit * 16

            onClosed: outputPathDialogLoader.active = false

            ColumnLayout {
                spacing: 0

                Repeater {
                    model: [
                        { name: 'default', value: PlasmaCameraManager.OutputPathDefault },
                        { name: 'DCIM', value: PlasmaCameraManager.OutputPathDCIM },
                    ]

                    delegate: QQC2.RadioDelegate {
                        id: radioDelegate
                        property string name: modelData.name
                        property int value: modelData.value

                        Layout.fillWidth: true
                        topPadding: Kirigami.Units.smallSpacing * 2
                        bottomPadding: Kirigami.Units.smallSpacing * 2

                        text: name
                        checked: value == root.cameraManager.outputPath
                        onCheckedChanged: {
                            if (checked) {
                                root.cameraManager.outputPath = value;
                                checked = Qt.binding(() => (value == root.cameraManager.outputPath));
                            }
                        }
                    }
                }
            }
        }
    }

    property Component aboutPageComponent: Component {
        id: aboutPage

        FormCard.AboutPage {
            aboutData: About
        }
    }
}
