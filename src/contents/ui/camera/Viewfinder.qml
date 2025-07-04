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

Rectangle {
    id: root

    required property PlasmaCamera camera
    property real bottomMargin: 0

    property alias videoSink: viewfinder.videoSink

    color: "black"

    Kirigami.Heading {
        anchors.centerIn: parent
        wrapMode: Text.WordWrap
        text: root.camera.available ? "" : i18n("Camera not available")
    }

    VideoOutput {
        id: viewfinder

        visible: root.camera.available
        width: parent.width
        height: parent.height
    }
}