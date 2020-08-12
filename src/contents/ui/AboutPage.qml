// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.6
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.1 as Controls
import org.kde.kirigami 2.6 as Kirigami

import org.kde.plasmacamera 1.0

Kirigami.AboutPage {
    aboutData: PlasmaCamera.aboutData
    globalToolBarStyle: Kirigami.Settings.isMobile ? Kirigami.ApplicationHeaderStyle.Breadcrumb : Kirigami.ApplicationHeaderStyle.ToolBar

    contextualActions: [
        Kirigami.Action {
            text: "Close"
            onTriggered: pageStack.pop()
        }
    ]
}
