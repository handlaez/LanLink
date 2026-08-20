import QtQuick
import QtQuick.Controls
import LanLinkApp

Window {
    width: 400
    height: 300
    visible: true
    title: "LanLink Streamer"

    AppController {
        id: controller
    }

    Column {
        anchors.centerIn: parent
        spacing: 10

        Text {
            text: "Status: " + controller.statusText
            font.pixelSize: 16
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Button {
            text: controller.isStreaming ? "Stop" : "Start"
            anchors.horizontalCenter: parent.horizontalCenter
            onClicked: {
                if (controller.isStreaming) {
                    controller.stop()
                } else {
                    controller.start("192.168.0.100", 5000)
                }
            }
        }
    }
}