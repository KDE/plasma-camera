// SPDX-FileCopyrightText: 2013 Digia Plc and/or its subsidiary(-ies)
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick 2.0
import org.kde.kirigami 2.0 as Kirigami
import QtMultimedia 5.0

Item {
    id: zoomControl

    property real currentZoom: 1
    property real maximumZoom: 1

    signal zoomTo(real value)

    visible: zoomControl.maximumZoom > 1

    MouseArea {
        id: mouseArea

        property real initialZoom: 0
        property real initialPos: 0

        anchors.fill: parent
        onPressed: {
            initialPos = mouseY;
            initialZoom = zoomControl.currentZoom;
        }
        onPositionChanged: {
            if (pressed) {
                var target = initialZoom * Math.pow(5, (initialPos - mouseY) / zoomControl.height);
                target = Math.max(1, Math.min(target, zoomControl.maximumZoom));
                zoomControl.zoomTo(target);
            }
        }
    }

    Item {
        id: bar

        x: 16
        y: parent.height / 4
        width: 24
        height: parent.height / 2

        Rectangle {
            anchors.fill: parent
            smooth: true
            radius: 8
            border.color: "white"
            border.width: 2
            color: "black"
            opacity: 0.3
        }

        Rectangle {
            id: groove

            x: 0
            y: parent.height * (1 - (zoomControl.currentZoom - 1) / (zoomControl.maximumZoom - 1))
            width: parent.width
            height: parent.height - y
            smooth: true
            radius: Kirigami.Units.gridUnit * 0.5
            color: "white"
            opacity: 0.5
        }

        Text {
            id: zoomText

            y: Math.min(parent.height - height, Math.max(0, groove.y - height / 2))
            text: "x" + Math.round(zoomControl.currentZoom * 100) / 100
            font.bold: true
            color: "white"
            style: Text.Raised
            styleColor: "black"
            opacity: 0.85
            font.pixelSize: Kirigami.Units.gridUnit

            anchors {
                right: bar.left
                rightMargin: Kirigami.Units.gridUnit * 1
            }

        }

    }

}
