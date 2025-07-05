// SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtMultimedia

import org.kde.kirigami as Kirigami

QQC2.Control {
    id: root

    property real duration

    background: Rectangle {
        color: Kirigami.Theme.negativeTextColor
        radius: Kirigami.Units.cornerRadius
    }

    topPadding: Kirigami.Units.smallSpacing
    bottomPadding: Kirigami.Units.smallSpacing
    leftPadding: Kirigami.Units.largeSpacing
    rightPadding: Kirigami.Units.largeSpacing
    width: timeFontMetrics.width + leftPadding + rightPadding
    height: timeFontMetrics.height + topPadding + bottomPadding

    contentItem: Item {
        QQC2.Label {
            id: videoRecordingDurationLabel
            anchors.centerIn: parent

            function convertMsToDuration(ms) {
                const millis = ms % 1000;
                let seconds = Math.floor(ms / 1000);
                const secs = seconds % 60;
                let minutes = Math.floor(seconds / 60);
                const mins = minutes % 60;
                const hours = Math.floor(minutes / 60);

                // Format for display (e.g., adding leading zeros)
                const pad = (num) => String(num).padStart(2, '0');
                const padMs = (num) => String(num).padStart(3, '0');
                return `${pad(hours)}:${pad(mins)}:${pad(secs)}`;
            }

            text: convertMsToDuration(root.duration)
            color: 'white'

            font.weight: Font.Bold
        }
    }

    QQC2.Label {
        id: timeFontMetrics
        visible: false
        text: '00:00:00'
        font.weight: Font.Bold
    }
}