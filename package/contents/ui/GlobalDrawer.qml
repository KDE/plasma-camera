import org.kde.kirigami 2.0 as Kirigami
import QtQuick 2.7
import QtQml.Models 2.2

Kirigami.GlobalDrawer {
    actions: [
        Kirigami.Action {
            text: qsTr("Video resolution")
            DelegateModel {
                model: root.cameraPage.videoRecorder.supportedResolutions
                delegate: Kirigami.Action {
                    text: model
                    onTriggered: settings.videoResolution = text;
                }
            }
        }
    ]
    
//     Component.onCompleted: console.log(root.cameraPage.cameraUI.camera.videoRecorder.supportedResolutions)

}
