import QtQuick 2.15
import QtQuick.Controls.Material 2.15
import "MaterialDesign.js" as MD

Item {
    property real size: 24
    property string name
    property color color: Material.foreground

    width: size
    height: size

    Text {
        anchors.fill: parent

        color: parent.color

        font.family: materialFont.name
        font.pixelSize: parent.height

        text: MD.icons["mdi_" + parent.name]
    }

    FontLoader {
        id: materialFont
        source: "../lib/materialdesignicons/fonts/materialdesignicons-webfont.ttf"
    }
}
