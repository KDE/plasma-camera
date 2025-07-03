// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2013 Digia Plc and/or its subsidiary(-ies)
// SPDX-License-Identifier: BSD-3-Clause

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

    property var camera

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
            text: i18n("About")
            icon.name: "help-about"
            onTriggered: applicationWindow().pageStack.pushDialogLayer(aboutPage)
        }
    ]

    property Loader dialogLoader: Loader {
        id: selectCameraDialogLoader
        active: false
        parent: root

        sourceComponent: Kirigami.Dialog {
            id: selectCameraDialog
            title: qsTr("Select Camera")
            preferredWidth: Kirigami.Units.gridUnit * 16

            onClosed: selectCameraDialogLoader.active = false

            ColumnLayout {
                spacing: 0
                Repeater {
                    model: {
                        let cameraIds = root.camera.getCameraDevicesId();
                        let cameraNames = root.camera.getCameraDevicesName();

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
                                root.camera.cameraDevice = value;
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
