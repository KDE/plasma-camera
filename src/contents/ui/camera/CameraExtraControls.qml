// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtMultimedia

import org.kde.kirigami as Kirigami
import org.kde.plasmacamera
import org.kde.kirigamiaddons.components as Components

Rectangle {
    id: root

    required property bool exposureValueEnabled

    signal exposureValueRequested(value: real)

    height: controlsLayout.implicitHeight
    color: Qt.rgba(0, 0, 0, 0.3)

    ColumnLayout {
        id: controlsLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top

        Kirigami.Theme.inherit: false
        Kirigami.Theme.colorSet: Kirigami.Theme.Complementary

        RowLayout {
            id: controlsRow
            visible: exposureControlsButton.visible
            spacing: Kirigami.Units.largeSpacing
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.bottomMargin: Kirigami.Units.largeSpacing

            Item { Layout.fillWidth: true }

            QQC2.ToolButton {
                id: exposureControlsButton
                icon.name: 'lighttable' // TODO: find better icon
                icon.color: "white"
                text: i18n('Exposure')
                display: QQC2.ToolButton.IconOnly
                visible: root.exposureValueEnabled
                onClicked: exposureSelectStrip.shown = !exposureSelectStrip.shown
            }

            Item { Layout.fillWidth: true }
        }

        ExposureSelectStrip {
            id: exposureSelectStrip
            visible: root.exposureValueEnabled
            shown: false // Set by exposureControlsButton

            onExposureValueRequested: (value) => root.exposureValueRequested(value)
            Layout.fillWidth: true
        }
    }
}