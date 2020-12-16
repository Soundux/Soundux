import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3 as Dialogs

ApplicationWindow {
    id: window
    visible: true
    width: 995
    height: 550
    title: "Soundux"
    minimumWidth: 850
    minimumHeight: 500

    property var isWindows: core.isWindows()
    property var darkMode:  core.getDarkMode()
    property var cBg: darkMode ? Material.color(Material.Grey, Material.Shade900) : Material.color(Material.Grey, Material.Shade400)
    property var cBgDarker: darkMode ? "#313131" : Material.color(Material.Grey, Material.Shade500)
    property var cBgDarkest: darkMode ? Material.color(Material.Grey, Material.Shade800) : Material.color(Material.Grey, Material.Shade600)

    Material.theme: darkMode ? Material.Dark : Material.Light
    Material.background: cBg

    Material.accent: Material.Green
    property var currentTab: undefined
    property var lastModifedLocal: true

    onWidthChanged:
    {
        core.onSizeChanged(width, height)
    }
    onHeightChanged:
    {
        core.onSizeChanged(width, height)
    }

    onClosing: {
        core.onClose()
    }

    Shortcut {
        sequence: "Ctrl+F"
        onActivated: {
            searchBtn.clicked();
        }
    }

    Component.onCompleted:
    {
        core.loadSettings();
    }

    Connections {
        target: core
        function onInvalidApplication() {
            invalidAppDialog.visible = true
            refreshOutputBtn.clicked();
        }
        function onSetLocalVolume(volume)
        {
            localVolume.value = volume * 100
        }
        function onSetRemoteVolume(volume)
        {
            remoteVolume.value = volume * 100
        }
        function onSetOutputApplication(index)
        {
            outputApplicationBox.currentIndex = index
        }
        function onSetSize(width, height)
        {
            window.width = width
            window.height = height
        }
    }

    Dialog
    {
        id: invalidAppDialog
        visible: false
        modal: true

        anchors.centerIn: parent

        Label
        {
            text: "Invalid output appliaction!"
        }

        standardButtons: Dialog.Ok
    }

    Image {
        id: icon
        source: "icon.jpg"
        width: 64
        fillMode: Image.PreserveAspectFit
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 10
        anchors.leftMargin: 5
    }

    Label {
        id: programName
        font.pointSize: 22
        anchors.left: icon.right
        anchors.verticalCenter: icon.verticalCenter
        anchors.leftMargin: 5
        text: "Soundux"
    }

    IconButton {
        id: stopButton
        text: "Stop"
        iconName: "stop"
        anchors.top: icon.bottom
        anchors.left: parent.left
        anchors.topMargin: 5
        anchors.leftMargin: 5
        height: 60
        width: 130

        onClicked: {
            core.stopPlayback()
        }
    }

    Pane {
        id: volumePane
        anchors.left: stopButton.right
        anchors.right: syncVolumeStates.left
        anchors.top: stopButton.top
        anchors.bottom: stopButton.bottom
        anchors.leftMargin: 35

        GridLayout {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            columns: 2
            rows: 2
            rowSpacing: -20
            Label {
                text: "Local volume"
                Layout.column: 0
                Layout.row: 0
            }
            Slider {
                id: localVolume

                value: 100
                stepSize: 1
                from: 0
                to: 100

                Layout.fillWidth: true
                Layout.column: 1
                Layout.row: 0

                ToolTip {
                    parent: localVolume.handle
                    visible: localVolume.pressed
                    text: value.toFixed(0)

                    readonly property int value: localVolume.valueAt(localVolume.position)
                }

                onValueChanged: {
                    core.changeLocalVolume(localVolume.value)
                    lastModifedLocal = true
                    if (syncVolumeStates.checked)
                    {
                        remoteVolume.value = localVolume.value
                    }
                }
            }
            Label {
                text: "Remote volume"
                Layout.column: 0
                Layout.row: 1
            }
            Slider {
                id: remoteVolume

                value: 100
                stepSize: 1
                from: 0
                to: 100

                Layout.fillWidth: true
                Layout.column: 1
                Layout.row: 1

                ToolTip {
                    parent: remoteVolume.handle
                    visible: remoteVolume.pressed
                    text: value.toFixed(0)

                    readonly property int value: remoteVolume.valueAt(remoteVolume.position)
                }

                onValueChanged: {
                    core.changeRemoteVolume(remoteVolume.value)
                    lastModifedLocal = false
                    if (syncVolumeStates.checked)
                    {
                        localVolume.value = remoteVolume.value
                    }
                }
            }
        }
    }

    CheckBox
    {
        text: "Sync"
        id: syncVolumeStates
        anchors.right: controlPane.left
        anchors.verticalCenter: volumePane.verticalCenter

        onCheckedChanged:
        {
            if (lastModifedLocal)
            {
                remoteVolume.value = localVolume.value
            }
            else
            {
                localVolume.value = remoteVolume.value
            }
        }
    }

    Pane {
        id: controlPane
        anchors.right: searchPane.left

        GridLayout {
            anchors.fill: parent
            columns: 1
            rowSpacing: -5

            ComboBox {
                id: outputApplicationBox

                model: ListModel {
                    id: outputApplicationModel
                }

                Component.onCompleted: {
                    var outputApplications = isWindows ? core.getPlaybackDevices() : core.getOutputApplications();
                    for (var child in outputApplications) {
                        outputApplicationModel.append({
                            "text": isWindows ? outputApplications[child] : outputApplications[child].getName()
                        })
                    }
                    outputApplicationBox.currentIndex = 0
                }

                Label {
                    text: isWindows ? "Output Device" : "Output application"
                    anchors.right: parent.left
                    anchors.rightMargin: 5
                    anchors.verticalCenter: parent.verticalCenter
                }

                onCurrentIndexChanged: {
                    core.currentOutputApplicationChanged(outputApplicationBox.currentIndex)
                }
            }

            IconButton {
                id: refreshOutputBtn
                Layout.fillWidth: true
                text: "Refresh"
                iconName: "reload"

                onClicked: {
                    outputApplicationModel.clear()
                    var outputApplications = isWindows ? core.getPlaybackDevices() : core.getOutputApplications();
                    for (var child in outputApplications) {
                        outputApplicationModel.append({
                            "text": isWindows ? outputApplications[child] : outputApplications[child].getName()
                        })
                    }
                    outputApplicationBox.currentIndex = 0
                }
            }
        }
    }

    Pane {
        id: searchPane
        visible: false

        width: visible ? implicitWidth : 0

        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        property var _sounds;

        onVisibleChanged:
        {
            if (visible)
            {
                var sounds = core.getAllSounds()
                _sounds = sounds
            }
        }

        ColumnLayout {
            spacing: 2
            anchors.fill: parent

            TextField {
                id: searchField
                width: 88
                placeholderText: "Search sounds..."
                onTextChanged:
                {
                    searchList.clear();
                    for (var child in searchPane._sounds) {
                        if (text && searchPane._sounds[child].getName().toLowerCase().includes(text.toLowerCase()))
                        {
                            searchList.append({
                                "name": searchPane._sounds[child].getName(),
                                "path": searchPane._sounds[child].getPath()
                            })
                        }
                    }
                }
            }

            ListView
            {
                Layout.fillHeight: true
                Layout.fillWidth: true

                id: searchResults

                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    width: 8
                }

                model: ListModel
                {
                    id: searchList
                }
                spacing: 0
                clip: true

                delegate: Rectangle {

                    color: cBgDarker
                    height: 25
                    width: searchResults.width
                    Layout.fillWidth: true

                    Text {
                        text: name
                        color: "white"
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onClicked: {
                            searchResults.currentIndex = index
                        }
                        onDoubleClicked: {
                            core.playSound(path)
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: setHotkeyDialog
        property string soundName;

        title: "Set hotkey for " + soundName
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Reset | Dialog.Cancel

        TextField {
            id: hotkeyField
            anchors.fill: parent
            placeholderText: "Hotkey"
            readOnly: true
        }

        Connections {
            target: core
            function onKeyPress(key) {
                hotkeyField.text = key.join(" + ")
            }
            function onKeyCleared() {
                hotkeyField.text = ""
            }
        }

        onVisibleChanged: {
            hotkeyField.text = ""
            if (visible)
            {
                hotkeyField.text = core.getCurrentHotKey(soundsListView.currentIndex).join(" + ")
            }
        }

        onActiveFocusChanged:
        {
            if (activeFocus)
            {
                hotkeyField.text = ""
            }
            core.hotkeyDialogFocusChanged(activeFocus);
        }

        onAccepted: {
            core.setHotkey(soundsListView.currentIndex)
        }

        onReset: {
            hotkeyField.text = ""
        }
    }

    Dialog {
        id: settingsDialog
        title: "Settings"
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel

        ColumnLayout {
            CheckBox {
                checked: true
                text: "Hotkeys only for current tab"
                onCheckedChanged:
                {
                    core.onTabHotkeyOnlyChanged(checked)
                }
                Component.onCompleted:
                {
                    checked = core.getTabHotkeysOnly();
                }
            }
            CheckBox {
                checked: false
                text: "Allow sound overlapping"
                onCheckedChanged:
                {
                    core.onAllowOverlappingChanged(checked);
                }
                Component.onCompleted:
                {
                    checked = core.getAllowOverlapping();
                }
            }
            CheckBox {
                checked: true
                Component.onCompleted:
                {
                    checked = darkMode;
                }

                text: "Dark Theme"
                onCheckedChanged:
                {
                    darkMode = checked;
                    core.onDarkModeChanged(darkMode);
                }
            }
            RowLayout {
                Label {
                    text: "Stop hotkey"
                }
                TextField {
                    text: "none"
                    id: stopHotkey
                    readOnly: true

                    Connections {
                        target: core
                        function onKeyPress(key) {
                            stopHotkey.text = key.join(" + ")
                        }
                        function onKeyCleared() {
                            stopHotkey.text = ""
                        }
                    }

                    onActiveFocusChanged:
                    {
                        if(activeFocus)
                        {
                            stopHotkey.text = ""
                        }
                        core.hotkeyDialogFocusChanged(activeFocus);
                    }
                }
            }
        }

        onVisibleChanged:
        {
            stopHotkey.text = ""
            if (visible)
            {
                stopHotkey.text = core.getCurrentHotKey(-100).join(" + ")
            }
        }

        onAccepted:
        {
            core.setHotkey(-100)
        }
    }

    Dialog {
        id: removeTabDialog
        title: "Remove tab"
        modal: true
        anchors.centerIn: parent
        contentHeight: -20
        standardButtons: Dialog.Yes | Dialog.No

        onAccepted: {
            core.removeTab()
        }
    }

    Dialogs.FileDialog {
        id: addTabDialog
        title: "Please choose a folder"
        selectFolder: true
        selectMultiple: true
        folder: shortcuts.home
        onAccepted: {
            core.addFolderTab(addTabDialog.fileUrls)
        }
    }

    TabBar {
        id: bar
        contentHeight: 35
        anchors.top: stopButton.bottom
        anchors.left: parent.left
        anchors.leftMargin: 5
        anchors.right: controlPane.left

        Repeater {
            model: ListModel {
                id: barModel
            }

            TabButton {
                text: modelData
                width: implicitWidth
            }
        }

        Connections {
            target: core
            function onFoldersChanged() {
                barModel.clear()
                var tabs = core.getTabs()
                for (var child in tabs) {
                    barModel.append({
                        "text": tabs[child].getTitle()
                    })
                }
                bar.currentIndex = -1
                bar.currentIndex = 0
            }
        }

        onCurrentIndexChanged: {
            core.currentTabChanged(bar.currentIndex)
            soundsListStack.updateItems()
        }

        Component.onCompleted: {
            var tabs = core.getTabs()
            for (var child in tabs) {
                barModel.append({
                    "text": tabs[child].getTitle()
                })
            }
            bar.currentIndex = 0
        }
    }

    StackLayout {
        id: soundsListStack
        anchors.top: bar.bottom
        anchors.left: parent.left
        anchors.right: controlPane.left
        anchors.bottom: parent.bottom

        function updateItems() {
            soundsList.clear()
            var sounds = core.getSounds()
            for (var child in sounds) {
                soundsList.append({
                    "name": sounds[child].getName(),
                    "path": sounds[child].getPath(),
                    "hotKey": sounds[child].getKeyBinds().join("+")
                })
            }
        }

        Component.onCompleted: {
            updateItems()
        }

        ListView {
            id: soundsListView
            anchors.fill: parent
            anchors.leftMargin: 5
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {
                width: 8
            }

            model: ListModel {
                id: soundsList
            }
            spacing: 0
            clip: true

            Rectangle
            {
                anchors.fill: parent
                color: cBgDarker
                z: -1
            }

            delegate: Rectangle {
                color: soundsListView.currentIndex == index ? cBgDarkest : cBgDarker
                width: soundsListView.width
                Layout.fillWidth: true
                height: 25

                Text {
                    text: name
                    color: "white"
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                }

                Rectangle
                {
                    visible: hotKey.length > 0

                    radius: 5
                    color: cBg

                    height: 20
                    width: hotkeyDisplay.implicitWidth + 10

                    anchors.rightMargin: 5
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter

                    Text
                    {
                        text: hotKey
                        color: "white"
                        id: hotkeyDisplay
                        anchors.centerIn: parent
                    }
                }

                MouseArea {
                    hoverEnabled: true
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        soundsListView.currentIndex = index
                    }
                    onDoubleClicked: {
                        core.playSound(path)
                    }
                    ToolTip {
                        x: parent.mouseX + 20
                        y: parent.mouseY + 20

                        text: path
                        delay: 1000
                        parent: parent
                        visible: parent.containsMouse
                    }
                }
            }
        }
    }

    Pane {
        anchors.top: controlPane.bottom
        anchors.left: controlPane.left
        anchors.right: controlPane.right
        anchors.bottom: parent.bottom

        GridLayout {
            anchors.fill: parent
            columns: 1
            rowSpacing: -5

            IconButton {
                Layout.fillWidth: true
                id: searchBtn
                text: "Search"
                iconName: "magnify"

                onClicked: {
                    searchPane.visible = !searchPane.visible
                    if (searchPane.visible) {
                        searchField.forceActiveFocus();
                    }
                }
            }

            IconButton {
                Layout.fillWidth: true
                text: "Add tab"
                iconName: "folder_plus"

                onClicked: {
                    addTabDialog.visible = true
                }
            }

            IconButton {
                Layout.fillWidth: true
                text: "Remove tab"
                iconName: "tab_minus"

                onClicked: {
                    // TODO: open only when on a tab
                    removeTabDialog.visible = true
                }
            }

            IconButton {
                id: buttonRefreshFolder
                Layout.fillWidth: true
                text: "Refresh"
                iconName: "refresh"

                onClicked: {
                    core.updateFolderSounds(core.getCurrentTab())
                    soundsListStack.updateItems()
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            IconButton {
                Layout.fillWidth: true
                text: "Set hotkey"
                iconName: "keyboard"

                onClicked: {
                    if (soundsListView.currentIndex >= 0)
                    {
                        setHotkeyDialog.visible = true
                        setHotkeyDialog.soundName = core.getSounds()[soundsListView.currentIndex].getName()
                    }
                }
            }

            IconButton {
                Layout.fillWidth: true
                text: "Play"
                iconName: "play"

                onClicked: {
                    core.playSound(soundsListView.currentIndex)
                }
            }

            IconButton {
                Layout.fillWidth: true
                text: "Settings"
                iconName: "cog"

                onClicked: {
                    settingsDialog.visible = true
                }
            }
        }
    }
}
