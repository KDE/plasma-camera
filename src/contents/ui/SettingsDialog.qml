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
    title: i18n('Settings')

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
            text: i18n('Video recording quality')
            icon.name: 'emblem-videos-symbolic'

            onTriggered: {
                selectVideoRecordingQualityLoader.active = true;
                selectVideoRecordingQualityLoader.item.open();
            }
        },

        Kirigami.Action {
            text: i18n("About")
            icon.name: "help-about"
            onTriggered: applicationWindow().pageStack.push(aboutPage)
        }
    ]

    property Loader qualityLoader: Loader {
        id: selectVideoRecordingQualityLoader
        active: false

        sourceComponent: Kirigami.Dialog {
            id: selectVideoRecordingDialog
            title: i18n('Video Recording Quality')
            preferredWidth: Kirigami.Units.gridUnit * 16

            onClosed: selectVideoRecordingQualityLoader.active = false

            ColumnLayout {
                spacing: 0

                Repeater {
                    model: [
                        { name: i18n('Very low quality'), value: PlasmaCameraManager.VeryLowQuality },
                        { name: i18n('Low quality'), value: PlasmaCameraManager.LowQuality },
                        { name: i18n('Normal quality'), value: PlasmaCameraManager.NormalQuality },
                        { name: i18n('High quality'), value: PlasmaCameraManager.HighQuality },
                        { name: i18n('Very high quality'), value: PlasmaCameraManager.VeryHighQuality },
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

    property Component aboutPageComponent: Component {
        id: aboutPage

        FormCard.AboutPage {
            aboutData: About
        }
    }
}
