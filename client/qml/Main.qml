// Main application window. Placeholder shell for the ultrasound.money-inspired
// dark theme: near-black background, neon-green accent, muted secondary text.
// The real design system (Theme singleton, pages, navigation) arrives with
// the auth increment.

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import Modulo

ApplicationWindow {
    id: window

    visible: true
    width: 960
    height: 600
    minimumWidth: 480
    minimumHeight: 320
    title: qsTr("Modulo")

    Material.theme: Material.Dark
    Material.accent: "#00ffa3"
    color: "#10141b"

    ApiClient {
        id: api
    }

    Timer {
        interval: 3000
        repeat: true
        running: true
        triggeredOnStart: true
        onTriggered: api.checkHealth()
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 12

        Label {
            text: qsTr("Modulo")
            font.pixelSize: 48
            font.weight: Font.DemiBold
            color: "#e6edf3"
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: qsTr("personal investment tracker")
            font.pixelSize: 16
            color: "#8b949e"
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            spacing: 8
            Layout.alignment: Qt.AlignHCenter

            Rectangle {
                id: statusDot
                width: 10
                height: 10
                radius: width / 2
                color: api.serverReachable ? "#00ffa3" : "#f85149"

                SequentialAnimation on opacity {
                    running: api.serverReachable
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 0.35; duration: 900 }
                    NumberAnimation { from: 0.35; to: 1.0; duration: 900 }
                }
            }

            Label {
                text: api.serverStatus
                font.pixelSize: 14
                color: "#8b949e"
            }
        }
    }
}
