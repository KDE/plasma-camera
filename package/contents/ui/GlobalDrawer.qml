import org.kde.kirigami 2.0 as Kirigami
import QtQuick 2.7

Kirigami.GlobalDrawer {
    actions: [
        Kirigami.Action {
            text: qsTr("Video resolution")
            Kirigami.Action {
                text: "640x480"
                onTriggered: settings.videoResolution = text;
            }
            Kirigami.Action {
                text: "1280x720"
                onTriggered: settings.videoResolution = text;
            }
            Kirigami.Action {
                text: "1920x1080"
                onTriggered: settings.resolution = text
            }
        }
    ]
    
    Component.onCompleted: console.log(imageCapture.supportedResolutions)

}
