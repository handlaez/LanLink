import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LanLinkApp

Window {
    width: 480
    height: 640
    visible: true
    title: "LanLink"
    color: "#1e1e1e"

    AppController {
        id: controller
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

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

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 100

            color: "#111111"
            radius: 4
            clip: true

            Flickable {
                id: logFlickable

                anchors.fill: parent
                anchors.margins: 6

                clip: true

                contentWidth: logText.width
                contentHeight: logText.height

                boundsBehavior: Flickable.StopAtBounds

                Text {
                    id: logText

                    width: Math.max(logFlickable.width, implicitWidth)
                    height: implicitHeight

                    text: controller.logLines.join("\n")

                    color: "#dddddd"
                    font.family: "Consolas"
                    font.pixelSize: 12

                    wrapMode: Text.NoWrap
                }

                Connections {
                    target: controller

                    function onLogLinesChanged() {
                        Qt.callLater(function() {
                            logFlickable.contentY = Math.max(0, logFlickable.contentHeight - logFlickable.height )
                        })
                    }
                }
            }
        }
    }
}