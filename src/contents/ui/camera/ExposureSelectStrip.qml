// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

Item {
    id: root

    // TODO: read value of exposure before opening

    // Whether the component should be shown
    required property bool shown

    // When an exposure value has been selected
    signal exposureValueRequested(value: real)

    implicitHeight: shown ? columnLayout.implicitHeight : 0
    clip: true

    Behavior on implicitHeight {
        PropertyAnimation {
            duration: Kirigami.Units.shortDuration
            easing.type: Easing.InOutCubic
        }
    }

    FontMetrics {
        id: fontMetrics
    }

    ColumnLayout {
        id: columnLayout
        spacing: Kirigami.Units.smallSpacing
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top

        QQC2.Label {
            Layout.topMargin: Kirigami.Units.smallSpacing
            Layout.fillWidth: true
            text: i18n("Exposure Value")
            horizontalAlignment: Text.AlignHCenter
            font.weight: Font.Bold
            font.pointSize: Kirigami.Theme.smallFont.pointSize
        }

        Item {
            Layout.fillWidth: true
            implicitHeight: Kirigami.Units.gridUnit * 2
            clip: true

            // The tumbler does some weird things with rotation, don't put it in layout
            QQC2.Tumbler {
                id: exposureTumbler
                anchors.centerIn: parent

                wrap: false
                implicitHeight: Kirigami.Units.gridUnit * 25

                // TODO: read minimum and maximum from Settings, and generate these values
                model: [-2, -1, -0.5, 0, 0.5, 1, 2]

                onMovingChanged: {
                    root.exposureValueRequested(model[currentIndex]);
                }

                rotation: -90
                visibleItemCount: 7

                delegate: delegateComponent
            }
        }
    }

    Component {
        id: delegateComponent

        Item {
            width: Kirigami.Units.gridUnit * 2
            opacity: 1 - Math.abs(QQC2.Tumbler.displacement) / (QQC2.Tumbler.tumbler.visibleItemCount / 2.5)

            QQC2.Label {
                anchors.centerIn: parent
                rotation: 90
                color: "white"
                text: modelData
                font.pixelSize: fontMetrics.font.pixelSize * 1.25
            }
        }
    }
}
