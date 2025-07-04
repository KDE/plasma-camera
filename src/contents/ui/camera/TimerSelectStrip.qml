// SPDX-FileCopyrightText: 2013 Digia Plc and/or its subsidiary(-ies)
// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2025 Andrew Wang
// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

Item {
    id: root

    // Whether the component should be shown
    required property bool shown

    // When a timer duration has been selected
    signal timerDurationRequested(duration: int)

    clip: true
    implicitHeight: shown ? Kirigami.Units.gridUnit * 2 : 0

    function formatText(count, modelData) {
        var data = count === 12 ? modelData + 1 : modelData;
        return data + " s";
    }

    Behavior on implicitHeight {
        PropertyAnimation {
            duration: Kirigami.Units.shortDuration
            easing.type: Easing.InOutCubic
        }
    }

    FontMetrics {
        id: fontMetrics
    }

    QQC2.Tumbler {
        id: timerTumbler

        anchors.centerIn: parent
        wrap: false
        implicitHeight: Kirigami.Units.gridUnit * 25
        model: [0, 2, 5, 10, 20]

        onMovingChanged: {
            root.timerDurationRequested(model[currentIndex]);
        }

        rotation: -90
        visibleItemCount: 7

        delegate: delegateComponent
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
                text: root.formatText(QQC2.Tumbler.tumbler.count, modelData)
                font.pixelSize: fontMetrics.font.pixelSize * 1.25
            }
        }
    }
}