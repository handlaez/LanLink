import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LanLinkApp

Window {
    width: 400
    height: 300
    visible: true
    title: "LanLink"
    color: "#1e1e1e"

    AppController {
        id: controller
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 12
        width: 280

        Text {
            text: "Status: " + controller.statusText
            font.bold: true
            color: "#ffffff"
            Layout.alignment: Qt.AlignHCenter
        }

        TextField {
            id: ipInput
            text: "192.168.0.100"
            placeholderText: "IP Address"
            enabled: !controller.isStreaming
            Layout.fillWidth: true
        }

        TextField {
            id: portInput
            text: "5000"
            placeholderText: "Port"
            enabled: !controller.isStreaming
            Layout.fillWidth: true
        }

        Button {
            text: controller.isStreaming ? "Stop" : "Start"
            
            palette.buttonText: "#000000"

            Layout.preferredWidth: 140
            Layout.alignment: Qt.AlignHCenter

            onClicked: {
                if (controller.isStreaming) {
                    controller.stop()
                } else {
                    controller.start(ipInput.text, parseInt(portInput.text))
                }
            }
        }
    }
}