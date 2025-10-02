// Header
            RowLayout {
                Layout.fillWidth: true

                Image {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    source: "image://theme/folder-download"
                }

                Label {
                    text: "Downloads"
                    font.bold: true
                    font.pixelSize: 18
                    Layout.fillWidth: true
                }

                ToolButton {
                    icon.name: "window-close"
                    text: "✕"
                    onClicked: downloadsDrawer.close()
                }
            };
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtWebEngine

ApplicationWindow {
    id: root
    width: 1280
    height: 800
    visible: true
    title: currentTab ? currentTab.title + " - Onu" : "Onu Browser"

    property var currentTab: tabBar.currentIndex >= 0 && tabBar.currentIndex < tabs.count ?
                            tabs.itemAt(tabBar.currentIndex) : null
    property bool isFullscreen: false

    // Set window icon
    Component.onCompleted: {
        root.icon.source = "qrc:/onu.svgz"

        var session = backend.restoreSession()
        if (session.length > 0) {
            for (var i = 0; i < session.length; i++) {
                addTab(session[i].url)
            }
        } else {
            addTab("https://www.google.com")
        }
    }

    Component.onDestruction: {
        var session = []
        for (var i = 0; i < tabs.count; i++) {
            var tab = tabs.itemAt(i)
            session.push({url: tab.url.toString(), title: tab.title})
        }
        backend.saveSession(session)
    }

    // Keyboard shortcuts
    Shortcut { sequence: "Ctrl+T"; onActivated: addTab("https://www.google.com") }
    Shortcut { sequence: "Ctrl+W"; onActivated: closeCurrentTab() }
    Shortcut { sequence: "Ctrl+R"; onActivated: if (currentTab) currentTab.reload() }
    Shortcut { sequence: "F5"; onActivated: if (currentTab) currentTab.reload() }
    Shortcut { sequence: "Ctrl+L"; onActivated: { urlBar.selectAll(); urlBar.forceActiveFocus() } }
    Shortcut { sequence: "Ctrl+D"; onActivated: toggleBookmark() }
    Shortcut { sequence: "Ctrl+H"; onActivated: historyDrawer.open() }
    Shortcut { sequence: "Ctrl+B"; onActivated: bookmarksDrawer.open() }
    Shortcut { sequence: "F11"; onActivated: toggleFullscreen() }
    Shortcut { sequence: "Ctrl++"; onActivated: if (currentTab) currentTab.zoomFactor += 0.1 }
    Shortcut { sequence: "Ctrl+-"; onActivated: if (currentTab) currentTab.zoomFactor = Math.max(0.25, currentTab.zoomFactor - 0.1) }
    Shortcut { sequence: "Ctrl+0"; onActivated: if (currentTab) currentTab.zoomFactor = 1.0 }
    Shortcut { sequence: "Ctrl+F"; onActivated: { findBar.visible = true; findTextField.forceActiveFocus() } }
    Shortcut { sequence: "Ctrl+Shift+Delete"; onActivated: clearDataDialog.open() }
    Shortcut { sequence: "Ctrl+Tab"; onActivated: tabBar.currentIndex = (tabBar.currentIndex + 1) % tabs.count }
    Shortcut { sequence: "Ctrl+Shift+Tab"; onActivated: tabBar.currentIndex = (tabBar.currentIndex - 1 + tabs.count) % tabs.count }

    function addTab(url) {
        var tab = tabComponent.createObject(tabs, {url: url})
        tabs.addItem(tab)
        tabBar.currentIndex = tabs.count - 1
    }

    function closeTab(index) {
        if (tabs.count === 1) {
            Qt.quit()
            return
        }
        tabs.removeItem(tabs.itemAt(index))
        tabs.itemAt(index).destroy()
    }

    function closeCurrentTab() {
        closeTab(tabBar.currentIndex)
    }

    function toggleBookmark() {
        if (!currentTab) return
        var url = currentTab.url.toString()
        if (backend.isBookmarked(url)) {
            backend.removeBookmark(url)
        } else {
            backend.addBookmark(currentTab.title, url, currentTab.icon.toString())
        }
    }

    function toggleFullscreen() {
        if (isFullscreen) {
            root.showNormal()
        } else {
            root.showFullScreen()
        }
        isFullscreen = !isFullscreen
    }

    // Top toolbar
    header: ToolBar {
        visible: !isFullscreen
        background: Rectangle {
            color: "#f5f5f5"
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "#ddd"
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 5
            spacing: 3

            // Logo
            Image {
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                source: "qrc:/onu.svgz"
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            // Navigation buttons
            ToolButton {
                icon.name: "go-previous"
                text: "◀"
                enabled: currentTab ? currentTab.canGoBack : false
                onClicked: if (currentTab) currentTab.goBack()
                ToolTip.visible: hovered
                ToolTip.text: "Back (Alt+Left)"
            }

            ToolButton {
                icon.name: "go-next"
                text: "▶"
                enabled: currentTab ? currentTab.canGoForward : false
                onClicked: if (currentTab) currentTab.goForward()
                ToolTip.visible: hovered
                ToolTip.text: "Forward (Alt+Right)"
            }

            ToolButton {
                icon.name: currentTab && currentTab.loading ? "process-stop" : "view-refresh"
                text: currentTab && currentTab.loading ? "✕" : "↻"
                onClicked: {
                    if (currentTab) {
                        if (currentTab.loading) currentTab.stop()
                        else currentTab.reload()
                    }
                }
                ToolTip.visible: hovered
                ToolTip.text: currentTab && currentTab.loading ? "Stop (Esc)" : "Reload (Ctrl+R)"
            }

            ToolButton {
                icon.name: "go-home"
                text: "🏠"
                onClicked: if (currentTab) currentTab.url = "https://www.google.com"
                ToolTip.visible: hovered
                ToolTip.text: "Home"
            }

            // URL bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                color: "#ffffff"
                radius: 6
                border.color: urlBar.activeFocus ? "#4a90e2" : "#d0d0d0"
                border.width: urlBar.activeFocus ? 2 : 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 6

                    // Security icon
                    Image {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        source: {
                            if (currentTab && currentTab.url.toString().startsWith("https")) {
                                return "image://theme/channel-secure-symbolic"
                            } else {
                                return "image://theme/channel-insecure-symbolic"
                            }
                        }
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                    }

                    TextField {
                        id: urlBar
                        Layout.fillWidth: true
                        background: Item {}
                        text: currentTab ? currentTab.url : ""
                        selectByMouse: true
                        font.pixelSize: 13
                        placeholderText: "Search or enter address"

                        onAccepted: {
                            var input = text.trim()
                            if (!input) return

                            if (input.indexOf("://") === -1 && input.indexOf(".") === -1) {
                                input = "https://www.google.com/search?q=" + encodeURIComponent(input)
                            } else if (input.indexOf("://") === -1) {
                                input = "https://" + input
                            }

                            if (currentTab) {
                                currentTab.url = input
                            }
                        }

                        // Auto-complete popup
                        Popup {
                            id: suggestionsPopup
                            y: urlBar.height + 2
                            width: urlBar.width
                            height: Math.min(suggestionsModel.count * 45, 300)
                            visible: suggestionsModel.count > 0 && urlBar.activeFocus && urlBar.text.length > 2
                            padding: 0

                            background: Rectangle {
                                color: "#ffffff"
                                border.color: "#d0d0d0"
                                border.width: 1
                                radius: 6

                                layer.enabled: true
                                layer.effect: DropShadow {
                                    transparentBorder: true
                                    horizontalOffset: 0
                                    verticalOffset: 2
                                    radius: 8
                                    samples: 16
                                    color: "#30000000"
                                }
                            }

                            ListView {
                                anchors.fill: parent
                                model: ListModel { id: suggestionsModel }
                                clip: true

                                delegate: ItemDelegate {
                                    width: ListView.view.width
                                    height: 45

                                    background: Rectangle {
                                        color: parent.hovered ? "#f0f0f0" : "transparent"
                                    }

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 8
                                        spacing: 10

                                        Image {
                                            Layout.preferredWidth: 16
                                            Layout.preferredHeight: 16
                                            source: "qrc:/icons/history.svg"
                                            fillMode: Image.PreserveAspectFit
                                        }

                                        Column {
                                            Layout.fillWidth: true
                                            spacing: 2

                                            Text {
                                                text: model.title
                                                font.bold: true
                                                font.pixelSize: 12
                                                elide: Text.ElideRight
                                                width: parent.width
                                            }
                                            Text {
                                                text: model.url
                                                color: "#666"
                                                font.pixelSize: 10
                                                elide: Text.ElideRight
                                                width: parent.width
                                            }
                                        }
                                    }

                                    onClicked: {
                                        urlBar.text = model.url
                                        urlBar.accepted()
                                        suggestionsModel.clear()
                                    }
                                }
                            }
                        }

                        onTextChanged: {
                            if (text.length > 2 && activeFocus) {
                                suggestionsModel.clear()
                                var results = backend.searchHistory(text)
                                for (var i = 0; i < results.length; i++) {
                                    suggestionsModel.append(results[i])
                                }
                            } else {
                                suggestionsModel.clear()
                            }
                        }
                    }

                    // Bookmark button
                    ToolButton {
                        icon.name: backend.isBookmarked(currentTab ? currentTab.url.toString() : "") ?
                                    "starred-symbolic" : "non-starred-symbolic"
                        text: backend.isBookmarked(currentTab ? currentTab.url.toString() : "") ? "★" : "☆"
                        onClicked: toggleBookmark()
                        ToolTip.visible: hovered
                        ToolTip.text: "Bookmark (Ctrl+D)"
                    }
                }
            }

            // Menu buttons
            ToolButton {
                icon.name: "folder-download"
                text: "📥"
                onClicked: downloadsDrawer.open()
                ToolTip.visible: hovered
                ToolTip.text: "Downloads"
            }

            ToolButton {
                icon.name: "bookmarks"
                text: "📚"
                onClicked: bookmarksDrawer.open()
                ToolTip.visible: hovered
                ToolTip.text: "Bookmarks (Ctrl+B)"
            }

            ToolButton {
                icon.name: "document-open-recent"
                text: "🕐"
                onClicked: historyDrawer.open()
                ToolTip.visible: hovered
                ToolTip.text: "History (Ctrl+H)"
            }

            ToolButton {
                icon.name: "preferences-system"
                text: "⚙"
                onClicked: settingsDrawer.open()
                ToolTip.visible: hovered
                ToolTip.text: "Settings"
            }

            ToolButton {
                icon.name: "list-add"
                text: "+"
                onClicked: addTab("https://www.google.com")
                ToolTip.visible: hovered
                ToolTip.text: "New Tab (Ctrl+T)"
                font.pixelSize: 18
                font.bold: true
            }
        }
    }

    // Tab bar
    TabBar {
        id: tabBar
        width: parent.width
        visible: !isFullscreen

        background: Rectangle {
            color: "#e8e8e8"
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: "#ccc"
            }
        }

        Repeater {
            model: tabs.count

            TabButton {
                width: Math.min(220, root.width / Math.max(tabs.count, 1))
                height: 36

                background: Rectangle {
                    color: tabBar.currentIndex === index ? "#f5f5f5" : "transparent"
                    border.color: tabBar.currentIndex === index ? "#ccc" : "transparent"
                    border.width: tabBar.currentIndex === index ? 1 : 0
                    radius: 6

                    Rectangle {
                        visible: tabBar.currentIndex === index
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: "#f5f5f5"
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 6

                    Image {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        source: tabs.itemAt(index) ? tabs.itemAt(index).icon : ""
                        fillMode: Image.PreserveAspectFit

                        Image {
                            anchors.fill: parent
                            source: "qrc:/onu.svgz"
                            visible: parent.status !== Image.Ready
                            fillMode: Image.PreserveAspectFit
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: tabs.itemAt(index) ? tabs.itemAt(index).title : ""
                        elide: Text.ElideRight
                        font.pixelSize: 12
                        color: tabBar.currentIndex === index ? "#000" : "#666"
                    }

                    ToolButton {
                        Layout.preferredWidth: 20
                        Layout.preferredHeight: 20
                        icon.name: "window-close"
                        text: "✕"
                        font.pixelSize: 10
                        background: Rectangle {
                            radius: 3
                            color: parent.hovered ? "#e0e0e0" : "transparent"
                        }
                        onClicked: {
                            closeTab(index)
                            mouse.accepted = true
                        }
                    }
                }
            }
        }
    }

    // Tab content area
    Item {
        id: tabs
        anchors.fill: parent

        property int count: 0

        function addItem(item) {
            item.parent = tabs
            item.anchors.fill = tabs
            item.visible = false
            count++
        }

        function removeItem(item) {
            count--
        }

        function itemAt(index) {
            return children[index]
        }
    }

    // Web view component
    Component {
        id: tabComponent

        WebEngineView {
            id: webView
            visible: tabBar.currentIndex === Array.prototype.indexOf.call(tabs.children, this)

            property bool privateMode: false

            profile: privateMode ? privateProfile : WebEngineProfile.defaultProfile

            settings.pluginsEnabled: true
            settings.javascriptEnabled: true
            settings.autoLoadImages: true
            settings.localStorageEnabled: true
            settings.allowRunningInsecureContent: false

            onLoadingChanged: function(loadRequest) {
                if (loadRequest.status === WebEngineView.LoadSucceededStatus) {
                    backend.addToHistory(title, url.toString())
                }
            }

            onNewWindowRequested: function(request) {
                addTab(request.requestedUrl)
                request.openIn(tabs.itemAt(tabs.count - 1))
            }

            onFullScreenRequested: function(request) {
                request.accept()
                toggleFullscreen()
            }

            onContextMenuRequested: function(request) {
                contextMenu.x = request.x
                contextMenu.y = request.y
                contextMenu.linkUrl = request.linkUrl
                contextMenu.selectedText = request.selectedText
                contextMenu.open()
            }

            onCertificateError: function(error) {
                error.defer()
                certErrorDialog.errorDescription = error.description
                certErrorDialog.errorUrl = error.url
                certErrorDialog.errorObj = error
                certErrorDialog.open()
            }

            // Find in page
            property string findText: ""

            function findInPage(text, forward) {
                findText = text
                findText(text, forward ? 0 : WebEngineView.FindBackward)
            }
        }
    }

    // Private profile
    WebEngineProfile {
        id: privateProfile
        offTheRecord: true
    }

    // Context menu
    Menu {
        id: contextMenu

        property string linkUrl: ""
        property string selectedText: ""

        MenuItem {
            icon.name: "go-previous"
            text: "Back"
            enabled: currentTab ? currentTab.canGoBack : false
            onTriggered: if (currentTab) currentTab.goBack()
        }

        MenuItem {
            icon.name: "go-next"
            text: "Forward"
            enabled: currentTab ? currentTab.canGoForward : false
            onTriggered: if (currentTab) currentTab.goForward()
        }

        MenuItem {
            icon.name: "view-refresh"
            text: "Reload"
            onTriggered: if (currentTab) currentTab.reload()
        }

        MenuSeparator {}

        MenuItem {
            icon.name: "edit-copy"
            text: "Copy"
            enabled: contextMenu.selectedText !== ""
            onTriggered: backend.copyToClipboard(contextMenu.selectedText)
        }

        MenuItem {
            icon.name: "edit-paste"
            text: "Paste"
            onTriggered: if (currentTab) currentTab.triggerWebAction(WebEngineView.Paste)
        }

        MenuSeparator { visible: contextMenu.linkUrl !== "" }

        MenuItem {
            visible: contextMenu.linkUrl !== ""
            text: "Open Link in New Tab"
            onTriggered: addTab(contextMenu.linkUrl)
        }

        MenuItem {
            icon.name: "edit-copy"
            visible: contextMenu.linkUrl !== ""
            text: "Copy Link"
            onTriggered: backend.copyToClipboard(contextMenu.linkUrl)
        }

        MenuSeparator {}

        MenuItem {
            text: "View Page Source"
            onTriggered: if (currentTab) currentTab.triggerWebAction(WebEngineView.ViewSource)
        }

        MenuItem {
            text: "Inspect Element"
            onTriggered: if (currentTab) currentTab.triggerWebAction(WebEngineView.InspectElement)
        }
    }

    // Bookmarks drawer
    Drawer {
        id: bookmarksDrawer
        width: 320
        height: root.height
        edge: Qt.RightEdge

        background: Rectangle {
            color: "#f9f9f9"
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            // Header
            RowLayout {
                Layout.fillWidth: true

                Image {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    source: "image://theme/bookmarks"
                }

                Label {
                    text: "Bookmarks"
                    font.bold: true
                    font.pixelSize: 18
                    Layout.fillWidth: true
                }

                ToolButton {
                    icon.name: "window-close"
                    text: "✕"
                    onClicked: bookmarksDrawer.close()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#ddd"
            }

            // Bookmarks list
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ListView {
                    spacing: 6

                    model: {
                        var bookmarks = backend.getBookmarks()
                        return bookmarks.length
                    }

                    delegate: ItemDelegate {
                        width: ListView.view.width
                        height: 60

                        background: Rectangle {
                            color: parent.hovered ? "#ffffff" : "transparent"
                            radius: 6
                            border.color: parent.hovered ? "#e0e0e0" : "transparent"
                            border.width: 1
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 10

                            Image {
                                Layout.preferredWidth: 24
                                Layout.preferredHeight: 24
                                source: {
                                    var bookmarks = backend.getBookmarks()
                                    return bookmarks[index].favicon || "qrc:/onu.svgz"
                                }
                                fillMode: Image.PreserveAspectFit
                            }

                            Column {
                                Layout.fillWidth: true
                                spacing: 4

                                Text {
                                    width: parent.width
                                    text: {
                                        var bookmarks = backend.getBookmarks()
                                        return bookmarks[index].title || "Untitled"
                                    }
                                    font.bold: true
                                    font.pixelSize: 13
                                    elide: Text.ElideRight
                                }
                                Text {
                                    width: parent.width
                                    text: {
                                        var bookmarks = backend.getBookmarks()
                                        return bookmarks[index].url || ""
                                    }
                                    color: "#666"
                                    font.pixelSize: 10
                                    elide: Text.ElideRight
                                }
                            }

                            ToolButton {
                                icon.name: "edit-delete"
                                text: "✕"
                                onClicked: {
                                    var bookmarks = backend.getBookmarks()
                                    backend.removeBookmark(bookmarks[index].url)
                                }
                            }
                        }

                        onClicked: {
                            var bookmarks = backend.getBookmarks()
                            if (currentTab) {
                                currentTab.url = bookmarks[index].url
                            }
                            bookmarksDrawer.close()
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        text: "No bookmarks yet\n\nPress Ctrl+D to bookmark\nthe current page"
                        visible: parent.count === 0
                        color: "#999"
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: 13
                    }
                }
            }
        }
    }

    // History drawer
    Drawer {
        id: historyDrawer
        width: 320
        height: root.height
        edge: Qt.RightEdge

        background: Rectangle {
            color: "#f9f9f9"
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            // Header
            RowLayout {
                Layout.fillWidth: true

                Image {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    source: "image://theme/document-open-recent"
                }

                Label {
                    text: "History"
                    font.bold: true
                    font.pixelSize: 18
                    Layout.fillWidth: true
                }

                ToolButton {
                    icon.name: "edit-clear-all"
                    text: "Clear"
                    onClicked: {
                        backend.clearHistory()
                        historyList.model = 0
                    }
                    ToolTip.visible: hovered
                    ToolTip.text: "Clear History"
                }

                ToolButton {
                    icon.name: "window-close"
                    text: "✕"
                    onClicked: historyDrawer.close()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#ddd"
            }

            // History list
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ListView {
                    id: historyList
                    spacing: 6

                    model: {
                        var history = backend.getHistory()
                        return history.length
                    }

                    delegate: ItemDelegate {
                        width: ListView.view.width
                        height: 65

                        background: Rectangle {
                            color: parent.hovered ? "#ffffff" : "transparent"
                            radius: 6
                            border.color: parent.hovered ? "#e0e0e0" : "transparent"
                            border.width: 1
                        }

                        Column {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 3

                            Text {
                                width: parent.width
                                text: {
                                    var history = backend.getHistory()
                                    return history[index].title || "Untitled"
                                }
                                font.bold: true
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }
                            Text {
                                width: parent.width
                                text: {
                                    var history = backend.getHistory()
                                    return history[index].url || ""
                                }
                                color: "#666"
                                font.pixelSize: 10
                                elide: Text.ElideRight
                            }
                            Text {
                                width: parent.width
                                text: {
                                    var history = backend.getHistory()
                                    var date = new Date(history[index].timestamp)
                                    return date.toLocaleString()
                                }
                                color: "#999"
                                font.pixelSize: 9
                            }
                        }

                        onClicked: {
                            var history = backend.getHistory()
                            if (currentTab) {
                                currentTab.url = history[index].url
                            }
                            historyDrawer.close()
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        text: "No history yet"
                        visible: parent.count === 0
                        color: "#999"
                        font.pixelSize: 13
                    }
                }
            }
        }
    }

    // Settings drawer
    Drawer {
        id: settingsDrawer
        width: 360
        height: root.height
        edge: Qt.RightEdge

        background: Rectangle {
            color: "#f9f9f9"
        }

        ScrollView {
            anchors.fill: parent
            clip: true

            ColumnLayout {
                width: settingsDrawer.width
                spacing: 16
                padding: 16

                // Header
                RowLayout {
                    Layout.fillWidth: true

                    Image {
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        source: "image://theme/preferences-system"
                    }

                    Label {
                        text: "Settings"
                        font.bold: true
                        font.pixelSize: 18
                        Layout.fillWidth: true
                    }

                    ToolButton {
                        icon.name: "window-close"
                        text: "✕"
                        onClicked: settingsDrawer.close()
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#ddd"
                }

                // Ad Blocker
                Rectangle {
                    Layout.fillWidth: true
                    height: 60
                    color: "#ffffff"
                    radius: 8
                    border.color: "#e0e0e0"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12

                        Image {
                            Layout.preferredWidth: 20
                            Layout.preferredHeight: 20
                            source: "image://theme/security-high"
                        }

                        Column {
                            Layout.fillWidth: true
                            Label {
                                text: "Ad Blocker"
                                font.bold: true
                            }
                            Label {
                                text: "Block ads and trackers"
                                font.pixelSize: 10
                                color: "#666"
                            }
                        }

                        Switch {
                            checked: backend.adBlockEnabled
                            onToggled: backend.adBlockEnabled = checked
                        }
                    }
                }

                // Download Path
                Rectangle {
                    Layout.fillWidth: true
                    height: 80
                    color: "#ffffff"
                    radius: 8
                    border.color: "#e0e0e0"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true

                            Image {
                                Layout.preferredWidth: 20
                                Layout.preferredHeight: 20
                                source: "image://theme/folder"
                            }

                            Label {
                                text: "Download Location"
                                font.bold: true
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            TextField {
                                Layout.fillWidth: true
                                text: backend.downloadPath
                                readOnly: true
                                font.pixelSize: 10
                                background: Rectangle {
                                    color: "#f5f5f5"
                                    radius: 4
                                    border.color: "#d0d0d0"
                                    border.width: 1
                                }
                            }
                        }
                    }
                }

                // Zoom Level
                Rectangle {
                    Layout.fillWidth: true
                    height: 80
                    color: "#ffffff"
                    radius: 8
                    border.color: "#e0e0e0"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true

                            Image {
                                Layout.preferredWidth: 20
                                Layout.preferredHeight: 20
                                source: "image://theme/zoom-in"
                            }

                            Label {
                                text: "Zoom Level: " + (currentTab ? Math.round(currentTab.zoomFactor * 100) : 100) + "%"
                                font.bold: true
                                Layout.fillWidth: true
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Button {
                                text: "-"
                                onClicked: if (currentTab) currentTab.zoomFactor = Math.max(0.25, currentTab.zoomFactor - 0.1)
                            }
                            Button {
                                text: "Reset"
                                Layout.fillWidth: true
                                onClicked: if (currentTab) currentTab.zoomFactor = 1.0
                            }
                            Button {
                                text: "+"
                                onClicked: if (currentTab) currentTab.zoomFactor = Math.min(5.0, currentTab.zoomFactor + 0.1)
                            }
                        }
                    }
                }

                // Privacy Section
                Label {
                    text: "Privacy"
                    font.bold: true
                    font.pixelSize: 16
                    Layout.topMargin: 8
                }

                Button {
                    Layout.fillWidth: true
                    icon.name: "edit-clear-all"
                    text: "Clear Browsing Data"
                    onClicked: clearDataDialog.open()
                }

                Button {
                    Layout.fillWidth: true
                    icon.name: "view-private"
                    text: "New Private Tab"
                    onClicked: {
                        var tab = tabComponent.createObject(tabs, {
                            url: "https://www.google.com",
                            privateMode: true
                        })
                        tabs.addItem(tab)
                        tabBar.currentIndex = tabs.count - 1
                        settingsDrawer.close()
                    }
                }

                // Developer Section
                Label {
                    text: "Developer"
                    font.bold: true
                    font.pixelSize: 16
                    Layout.topMargin: 8
                }

                Button {
                    Layout.fillWidth: true
                    icon.name: "tools-report-bug"
                    text: "Open DevTools"
                    onClicked: {
                        if (currentTab) {
                            currentTab.triggerWebAction(WebEngineView.InspectElement)
                        }
                    }
                }

                // About Section
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#ddd"
                    Layout.topMargin: 8
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 8

                    Image {
                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 64
                        Layout.alignment: Qt.AlignHCenter
                        source: "qrc:/onu.svgz"
                        fillMode: Image.PreserveAspectFit
                    }

                    Label {
                        text: "Onu Browser"
                        font.bold: true
                        font.pixelSize: 16
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Label {
                        text: "Version 1.0.0"
                        color: "#666"
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Label {
                        text: "A minimal, feature-rich Qt browser"
                        color: "#666"
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }

    // Downloads drawer
    Drawer {
        id: downloadsDrawer
        width: 360
        height: root.height
        edge: Qt.RightEdge

        background: Rectangle {
            color: "#f9f9f9"
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            // Header
            RowLayout {
                Layout.fillWidth: true

                Image {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    source: "qrc:/icons/download.svg"
                }

                Label {
                    text: "Downloads"
                    font.bold: true
                    font.pixelSize: 18
                    Layout.fillWidth: true
                }

                ToolButton {
                    icon.source: "qrc:/icons/close.svg"
                    text: "✕"
                    display: AbstractButton.IconOnly
                    onClicked: downloadsDrawer.close()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#ddd"
            }

            // Downloads list
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ListView {
                    spacing: 10

                    model: ListModel {
                        id: downloadsModel
                    }

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 80
                        color: "#ffffff"
                        radius: 8
                        border.color: "#e0e0e0"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 6

                            Text {
                                text: model.fileName
                                font.bold: true
                                font.pixelSize: 13
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            ProgressBar {
                                Layout.fillWidth: true
                                value: model.progress / 100
                                visible: model.state === "downloading"
                            }

                            RowLayout {
                                Layout.fillWidth: true

                                Text {
                                    text: model.state === "completed" ? "Completed" :
                                          model.state === "downloading" ? model.progress + "%" : "Paused"
                                    color: "#666"
                                    font.pixelSize: 11
                                    Layout.fillWidth: true
                                }

                                Button {
                                    text: "Open"
                                    visible: model.state === "completed"
                                    onClicked: Qt.openUrlExternally("file://" + model.path)
                                }
                            }
                        }
                    }

                    Label {
                        anchors.centerIn: parent
                        text: "No downloads yet"
                        visible: downloadsModel.count === 0
                        color: "#999"
                        font.pixelSize: 13
                    }
                }
            }
        }
    }

    // Find in page bar
    Rectangle {
        id: findBar
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.bottomMargin: 10
        width: 320
        height: 45
        color: "#ffffff"
        border.color: "#d0d0d0"
        border.width: 1
        radius: 8
        visible: false
        z: 100

        layer.enabled: true
        layer.effect: DropShadow {
            transparentBorder: true
            horizontalOffset: 0
            verticalOffset: 2
            radius: 8
            samples: 16
            color: "#30000000"
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 6

            Image {
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18
                source: "image://theme/edit-find"
            }

            TextField {
                id: findTextField
                Layout.fillWidth: true
                placeholderText: "Find in page..."
                background: Rectangle {
                    color: "#f5f5f5"
                    radius: 4
                }
                onTextChanged: {
                    if (currentTab && text.length > 0) {
                        currentTab.findInPage(text, true)
                    }
                }
                onAccepted: {
                    if (currentTab) {
                        currentTab.findInPage(text, true)
                    }
                }
            }

            ToolButton {
                icon.name: "go-up"
                text: "▲"
                onClicked: {
                    if (currentTab) {
                        currentTab.findInPage(findTextField.text, false)
                    }
                }
            }

            ToolButton {
                icon.name: "go-down"
                text: "▼"
                onClicked: {
                    if (currentTab) {
                        currentTab.findInPage(findTextField.text, true)
                    }
                }
            }

            ToolButton {
                icon.name: "window-close"
                text: "✕"
                onClicked: {
                    findBar.visible = false
                    if (currentTab) {
                        currentTab.findText("")
                    }
                }
            }
        }
    }

    // Clear data dialog
    Dialog {
        id: clearDataDialog
        anchors.centerIn: parent
        title: "Clear Browsing Data"
        standardButtons: Dialog.Ok | Dialog.Cancel

        background: Rectangle {
            color: "#ffffff"
            radius: 8
            border.color: "#d0d0d0"
            border.width: 1
        }

        ColumnLayout {
            spacing: 12

            Label {
                text: "Select data to clear:"
                font.bold: true
            }

            CheckBox {
                id: clearHistoryCheck
                text: "Browsing history"
                checked: true
            }

            CheckBox {
                id: clearCookiesCheck
                text: "Cookies and site data"
                checked: true
            }

            CheckBox {
                id: clearCacheCheck
                text: "Cached images and files"
                checked: true
            }
        }

        onAccepted: {
            if (clearHistoryCheck.checked) {
                backend.clearHistory()
            }
            if (clearCookiesCheck.checked || clearCacheCheck.checked) {
                WebEngineProfile.defaultProfile.clearHttpCache()
            }
        }
    }

    // Certificate error dialog
    Dialog {
        id: certErrorDialog
        anchors.centerIn: parent
        title: "Security Warning"
        standardButtons: Dialog.Yes | Dialog.No

        property string errorDescription: ""
        property string errorUrl: ""
        property var errorObj: null

        background: Rectangle {
            color: "#ffffff"
            radius: 8
            border.color: "#ff5252"
            border.width: 2
        }

        ColumnLayout {
            spacing: 12
            width: 450

            RowLayout {
                spacing: 12

                Image {
                    Layout.preferredWidth: 48
                    Layout.preferredHeight: 48
                    source: "image://theme/dialog-warning"
                }

                Label {
                    text: "This site's security certificate is not trusted!"
                    font.bold: true
                    font.pixelSize: 14
                    color: "#d32f2f"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#ffcdd2"
            }

            Label {
                text: certErrorDialog.errorDescription
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Label {
                text: "URL: " + certErrorDialog.errorUrl
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                color: "#666"
                font.pixelSize: 10
            }

            Label {
                text: "Do you want to proceed anyway? (Not recommended)"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                font.bold: true
            }
        }

        onAccepted: {
            if (errorObj) {
                errorObj.ignoreCertificateError()
            }
        }

        onRejected: {
            if (errorObj) {
                errorObj.rejectCertificate()
            }
        }
    }

    // Loading indicator
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 3
        color: "#4a90e2"
        visible: currentTab ? currentTab.loading : false
        z: 999

        SequentialAnimation on opacity {
            loops: Animation.Infinite
            running: currentTab ? currentTab.loading : false
            NumberAnimation { from: 1.0; to: 0.3; duration: 500 }
            NumberAnimation { from: 0.3; to: 1.0; duration: 500 }
        }
    }

    // Status bar
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 28
        color: "#f5f5f5"
        border.color: "#ddd"
        border.width: 1
        visible: !isFullscreen && statusText.text !== ""
        z: 100

        RowLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 10

            Text {
                id: statusText
                Layout.fillWidth: true
                elide: Text.ElideRight
                font.pixelSize: 11
                color: "#666"
            }

            Text {
                text: currentTab && currentTab.loading ? "Loading..." : "Ready"
                font.pixelSize: 11
                color: "#666"
            }
        }

        Connections {
            target: currentTab
            function onLinkHovered(url) {
                statusText.text = url
            }
        }
    }
}

import QtQuick
import QtGraphicalEffects
