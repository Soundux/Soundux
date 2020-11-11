import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

// import QtQuick.Templates 2.15 as T
// import QtQuick.Controls.impl 2.15
// import QtQuick.Controls.Material.impl 2.15
Button {
    id: control
    property string iconName

    Material.elevation: 5

    font.capitalization: Font.MixedCase
    font.pointSize: 9.5
    implicitHeight: 42
    contentItem: Item {
        RowLayout {
            // anchors.fill: parent
            anchors.centerIn: parent

            MaterialDesignIcon {
                name: iconName
                size: control.font.pixelSize * 1.4
                Layout.fillWidth: true
            }
            Text {
                Layout.fillWidth: true
                // width: control.width
                elide: Text.ElideRight
                text: control.text
                font: control.font
                color: !control.enabled ? control.Material.hintTextColor : control.flat
                                          && control.highlighted ? control.Material.accentColor : control.highlighted ? control.Material.primaryHighlightedTextColor : control.Material.foreground
                visible: !(control.text === "")
            }
        }
    }
    // Component.onCompleted: {
    //     console.log("feneg", control.contentItem, control.width)
    // }
}
