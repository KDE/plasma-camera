/****************************************************************************
**
** Copyright (C) 2018 Jonah Brüchert
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.7
import org.kde.kirigami 2.2 as Kirigami
import Qt.labs.settings 1.0
import QtMultimedia 5.8

Kirigami.ApplicationWindow {
    id: root

    readonly property int headerStyle: Kirigami.Settings.isMobile ? Kirigami.ApplicationHeaderStyle.None : Kirigami.ApplicationHeaderStyle.ToolBar

    Settings {
        id: settings
        
        // Default settings
        property size resolution
        property string cameraDeviceId
        property int cameraPosition
        property int whiteBalanceMode
    }

    Component {
        id: cameraPage

        CameraPage {
            camera: mainCamera
        }
    }

    Component {
        id: aboutPage

        AboutPage {}
    }

    Camera {
        id: mainCamera
        captureMode: Camera.CaptureStillImage
        deviceId: settings.cameraDeviceId
        imageProcessing.whiteBalanceMode: settings.whiteBalanceMode

        imageCapture {
            id: imageCapture
            resolution: settings.resolution
        }

        videoRecorder {
            id: videoRecorder
            resolution: settings.resolution
            frameRate: 30
        }
    }

    title: i18n("Camera")
    globalDrawer: GlobalDrawer {
        camera: mainCamera
    }

    pageStack.initialPage: cameraPage
    pageStack.globalToolBar.style: applicationWindow().headerStyle
}
