import org.kde.kirigami 2.0 as Kirigami
import QtQuick 2.7
import QtQml.Models 2.2
import QtMultimedia 5.8

Kirigami.GlobalDrawer {
    actions: [
        Kirigami.Action {
            text: qsTr("Video resolution")
            iconName: "ratiocrop"
            ListView {
                model: applicationWindow().cameraPage.camera.videoRecorder.supportedResolutions
                delegate: Kirigami.Action {
                    text: model
                    onTriggered: settings.videoResolution = text;
                }
            }
        },
        Kirigami.Action {
            text: qsTr("Photo resolution")
            iconName: "ratiocrop"
            ListView {
                model: applicationWindow().cameraPage.camera.imageCapture.supportedResolutions
                delegate: Kirigami.Action {
                    text: model
                    onTriggered: settings.videoResolution = text;
                }
            }
        },
        Kirigami.Action {
            text: qsTr("White balance")
            iconName: "whitebalance"
            ListView {
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
