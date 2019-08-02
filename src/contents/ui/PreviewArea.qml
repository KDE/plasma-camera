import QtQuick 2.7
import QtQuick.Controls.Material 2.0
import org.kde.kirigami 2.0 as Kirigami
import QtGraphicalEffects 1.0


Rectangle {
    id: preview

    property var imageCapture

    visible: imageCapture.capturedImagePath
    width: Kirigami.Units.gridUnit * 6
    height: width
    layer.enabled: preview.enabled
    layer.effect: DropShadow {
        color: Material.dropShadowColor
        samples: 30
        spread: 0.5
    }

    MouseArea {
        anchors.fill: parent
        onClicked: Qt.openUrlExternally("file://" + imageCapture.capturedImagePath)
    }

    Image {
        fillMode: Image.PreserveAspectCrop
        anchors.fill: parent
        source: imageCapture.capturedImagePath ? "file://" + imageCapture.capturedImagePath : ""
    }
}
