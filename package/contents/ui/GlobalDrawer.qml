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

import org.kde.kirigami 2.0 as Kirigami
import QtQuick 2.7
import QtQml.Models 2.2
import QtMultimedia 5.8

Kirigami.GlobalDrawer {
    actions: [
        Kirigami.Action {
            text: i18n("Camera")
            iconName: "camera-photo"
            DelegateModel {
                model: QtMultimedia.availableCameras
                delegate: Kirigami.Action {
                    text: model.displayName
                    onTriggered: settings.cameraDeviceId = model.deviceId
                }
            }
        },
        Kirigami.Action {
            text: i18n("Video resolution")
            iconName: "ratiocrop"
            DelegateModel {
                model: applicationWindow().cameraPage.camera.videoRecorder.supportedResolutions
                delegate: Kirigami.Action {
                    text: model
                    onTriggered: settings.videoResolution = text;
                }
            }
        },
        Kirigami.Action {
            text: i18n("Photo resolution")
            iconName: "ratiocrop"
            DelegateModel {
                model: applicationWindow().cameraPage.camera.imageCapture.supportedResolutions
                delegate: Kirigami.Action {
                    text: model
                    onTriggered: settings.videoResolution = text;
                }
            }
        },
        Kirigami.Action {
            text: i18n("White balance")
            iconName: "whitebalance"
            DelegateModel {
                model: ListModel {
                    ListElement {
                        icon: "images/camera_auto_mode.png"
                        value: CameraImageProcessing.WhiteBalanceAuto
                        text: "Auto"
                    }
                    ListElement {
                        icon: "images/camera_white_balance_sunny.png"
                        value: CameraImageProcessing.WhiteBalanceSunlight
                        text: "Sunlight"
                    }
                    ListElement {
                        icon: "images/camera_white_balance_cloudy.png"
                        value: CameraImageProcessing.WhiteBalanceCloudy
                        text: "Cloudy"
                    }
                    ListElement {
                        icon: "images/camera_white_balance_incandescent.png"
                        value: CameraImageProcessing.WhiteBalanceTungsten
                        text: "Tungsten"
                    }
                    ListElement {
                        icon: "images/camera_white_balance_flourescent.png"
                        value: CameraImageProcessing.WhiteBalanceFluorescent
                        text: "Fluorescent"
                    }
                }
                delegate: Kirigami.Action {
                    text: model.text
                    iconName: model.icon
                    onTriggered: {
                        applicationWindow().cameraPage.camera.imageProcessing.whiteBalanceMode = value
                    }
                }
            }
        }
    ]
}
