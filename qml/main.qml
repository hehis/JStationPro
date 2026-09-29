import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import JStation 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 1200
    height: 720
    minimumWidth: 800
    minimumHeight: 500
    title: "JStation Pro - Professional Audio Waveform Editor"
    color: "#12131A"

    function formatTime(seconds) {
        if (isNaN(seconds) || seconds < 0) seconds = 0
        var mins = Math.floor(seconds / 60)
        var secs = Math.floor(seconds % 60)
        var ms = Math.floor((seconds - Math.floor(seconds)) * 1000)
        return (mins < 10 ? "0" : "") + mins + ":" +
               (secs < 10 ? "0" : "") + secs + "." +
               (ms < 100 ? (ms < 10 ? "00" : "0") : "") + ms
    }

    AudioEngine {
        id: cppAudioEngine
    }

    Component.onCompleted: {
        if (initialAudioFile !== "") {
            cppAudioEngine.openAudioFile(initialAudioFile)
        }
    }

    // Drag and drop file support
    DropArea {
        anchors.fill: parent
        onDropped: {
            if (drop.hasUrls && drop.urls.length > 0 && cppAudioEngine) {
                cppAudioEngine.openAudioFile(drop.urls[0])
            }
        }
    }

    // Keyboard shortcuts for editing
    Shortcut {
        sequence: StandardKey.Cut
        onActivated: if (cppAudioEngine && cppAudioEngine.hasSelection) cppAudioEngine.cutSelection()
    }
    Shortcut {
        sequence: StandardKey.Copy
        onActivated: if (cppAudioEngine && cppAudioEngine.hasSelection) cppAudioEngine.copySelection()
    }
    Shortcut {
        sequence: StandardKey.Paste
        onActivated: {
            if (cppAudioEngine && cppAudioEngine.canPaste) {
                var insertPos = cppAudioEngine.hasSelection ? cppAudioEngine.selectionStart : cppAudioEngine.viewStartFrame
                cppAudioEngine.pasteAt(insertPos)
            }
        }
    }
    Shortcut {
        sequences: [StandardKey.Delete, "Backspace"]
        onActivated: if (cppAudioEngine && cppAudioEngine.hasSelection) cppAudioEngine.deleteSelection()
    }
    Shortcut {
        sequence: StandardKey.SelectAll
        onActivated: if (cppAudioEngine) cppAudioEngine.setSelection(0, cppAudioEngine.totalFrames)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ================= TOP TOOLBAR =================
        Rectangle {
            Layout.fillWidth: true
            height: 60
            color: "#181924"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                // Logo
                RowLayout {
                    spacing: 8
                    Rectangle {
                        width: 32; height: 32; radius: 6
                        color: "#00B4D8"
                        Text {
                            anchors.centerIn: parent
                            text: "JS"
                            font.bold: true
                            color: "#FFFFFF"
                        }
                    }
                    Text {
                        text: "JStation Pro"
                        font.bold: true
                        font.pixelSize: 17
                        color: "#E2E8F0"
                    }
                }

                Rectangle { width: 1; height: 28; color: "#2B2D3C"; Layout.leftMargin: 8; Layout.rightMargin: 8 }

                // Import Button - uses native system dialog
                Button {
                    text: "Import Audio"
                    font.pixelSize: 13
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: "#FFFFFF"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.down ? "#0077B6" : (parent.hovered ? "#0096C7" : "#00B4D8")
                        radius: 5
                    }
                    onClicked: {
                        if (cppAudioEngine) cppAudioEngine.importAudioDialog()
                    }
                }

                // Edit Buttons
                Button {
                    text: "Cut (Ctrl+X)"
                    font.pixelSize: 12
                    enabled: cppAudioEngine && cppAudioEngine.hasSelection
                    contentItem: Text {
                        text: parent.text; font: parent.font
                        color: parent.enabled ? "#F1F5F9" : "#64748B"
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.enabled ? (parent.down ? "#334155" : (parent.hovered ? "#475569" : "#1E293B")) : "#161E2E"
                        border.color: parent.enabled ? "#334155" : "#1E293B"
                        radius: 5
                    }
                    onClicked: {
                        if (cppAudioEngine) cppAudioEngine.cutSelection()
                    }
                }

                Button {
                    text: "Copy (Ctrl+C)"
                    font.pixelSize: 12
                    enabled: cppAudioEngine && cppAudioEngine.hasSelection
                    contentItem: Text {
                        text: parent.text; font: parent.font
                        color: parent.enabled ? "#F1F5F9" : "#64748B"
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.enabled ? (parent.down ? "#334155" : (parent.hovered ? "#475569" : "#1E293B")) : "#161E2E"
                        border.color: parent.enabled ? "#334155" : "#1E293B"
                        radius: 5
                    }
                    onClicked: {
                        if (cppAudioEngine) cppAudioEngine.copySelection()
                    }
                }

                Button {
                    text: "Paste (Ctrl+V)"
                    font.pixelSize: 12
                    enabled: cppAudioEngine && cppAudioEngine.canPaste
                    contentItem: Text {
                        text: parent.text; font: parent.font
                        color: parent.enabled ? "#F1F5F9" : "#64748B"
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.enabled ? (parent.down ? "#334155" : (parent.hovered ? "#475569" : "#1E293B")) : "#161E2E"
                        border.color: parent.enabled ? "#334155" : "#1E293B"
                        radius: 5
                    }
                    onClicked: {
                        if (cppAudioEngine) {
                            var pos = cppAudioEngine.hasSelection ? cppAudioEngine.selectionStart : cppAudioEngine.viewStartFrame
                            cppAudioEngine.pasteAt(pos)
                        }
                    }
                }

                Button {
                    text: "Delete (Del)"
                    font.pixelSize: 12
                    enabled: cppAudioEngine && cppAudioEngine.hasSelection
                    contentItem: Text {
                        text: parent.text; font: parent.font
                        color: parent.enabled ? "#EF4444" : "#64748B"
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.enabled ? (parent.down ? "#451A1A" : (parent.hovered ? "#5F1E1E" : "#2E1515")) : "#161E2E"
                        border.color: parent.enabled ? "#7F1D1D" : "#1E293B"
                        radius: 5
                    }
                    onClicked: {
                        if (cppAudioEngine) cppAudioEngine.deleteSelection()
                    }
                }

                Rectangle { width: 1; height: 28; color: "#2B2D3C"; Layout.leftMargin: 8; Layout.rightMargin: 8 }

                // Zoom controls
                Button {
                    text: "Zoom In (+)"
                    font.pixelSize: 12
                    enabled: cppAudioEngine && cppAudioEngine.isLoaded
                    contentItem: Text {
                        text: parent.text; font: parent.font
                        color: parent.enabled ? "#F1F5F9" : "#64748B"
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.enabled ? (parent.hovered ? "#334155" : "#1E293B") : "#161E2E"
                        radius: 5
                    }
                    onClicked: {
                        if (cppAudioEngine) cppAudioEngine.zoomAt(0.5, 0.7)
                    }
                }

                Button {
                    text: "Zoom Out (-)"
                    font.pixelSize: 12
                    enabled: cppAudioEngine && cppAudioEngine.isLoaded
                    contentItem: Text {
                        text: parent.text; font: parent.font
                        color: parent.enabled ? "#F1F5F9" : "#64748B"
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.enabled ? (parent.hovered ? "#334155" : "#1E293B") : "#161E2E"
                        radius: 5
                    }
                    onClicked: {
                        if (cppAudioEngine) cppAudioEngine.zoomAt(0.5, 1.4)
                    }
                }

                Button {
                    text: "Reset View"
                    font.pixelSize: 12
                    enabled: cppAudioEngine && cppAudioEngine.isLoaded
                    contentItem: Text {
                        text: parent.text; font: parent.font
                        color: parent.enabled ? "#F1F5F9" : "#64748B"
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.enabled ? (parent.hovered ? "#334155" : "#1E293B") : "#161E2E"
                        radius: 5
                    }
                    onClicked: {
                        if (cppAudioEngine) cppAudioEngine.resetView()
                    }
                }

                Item { Layout.fillWidth: true }

                // Selection info
                Text {
                    visible: cppAudioEngine && cppAudioEngine.hasSelection
                    text: (cppAudioEngine && cppAudioEngine.hasSelection) ?
                          ("Selected: " + formatTime(cppAudioEngine.frameToSeconds(cppAudioEngine.selectionEnd - cppAudioEngine.selectionStart)) +
                          " (" + cppAudioEngine.selectionStart + " - " + cppAudioEngine.selectionEnd + ")") : ""
                    font.pixelSize: 12
                    color: "#F87171"
                }
            }
        }

        // ================= PROGRESS BAR (DURING IMPORT) =================
        Rectangle {
            Layout.fillWidth: true
            height: (cppAudioEngine && cppAudioEngine.isDecoding) ? 30 : 0
            visible: cppAudioEngine && cppAudioEngine.isDecoding
            color: "#1E293B"
            clip: true

            Behavior on height { NumberAnimation { duration: 150 } }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Text {
                    text: "Decoding Audio..."
                    font.pixelSize: 12
                    color: "#00B4D8"
                }

                ProgressBar {
                    Layout.fillWidth: true
                    value: cppAudioEngine ? cppAudioEngine.decodeProgress : 0
                }

                Text {
                    text: Math.floor((cppAudioEngine ? cppAudioEngine.decodeProgress : 0) * 100) + "%"
                    font.pixelSize: 12
                    font.bold: true
                    color: "#FFFFFF"
                }
            }
        }

        // ================= MAIN WAVEFORM AREA =================
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8
            Layout.margins: 12

            // Label for Thumbnail
            RowLayout {
                Text {
                    text: "OVERVIEW (Thumbnail)"
                    font.bold: true
                    font.pixelSize: 11
                    color: "#94A3B8"
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "Total: " + formatTime(cppAudioEngine ? cppAudioEngine.durationSeconds : 0) + " | Zoom via Wheel | Drag Mask Handles to Resize"
                    font.pixelSize: 11
                    color: "#64748B"
                }
            }

            // 1. TOP ITEM: THUMBNAIL WAVEFORM WITH MASK
            Rectangle {
                id: thumbContainer
                Layout.fillWidth: true
                height: 110
                color: "#161824"
                radius: 6
                border.color: "#282B3E"
                clip: true

                WaveformItem {
                    id: thumbWave
                    anchors.fill: parent
                    anchors.margins: 2
                    audioEngine: cppAudioEngine
                    isThumbnail: true
                    waveColor: "#48CAE4"
                    backgroundColor: "#161824"
                }

                // Interactive Mask Overlay for Thumbnail
                Item {
                    id: maskOverlay
                    anchors.fill: thumbWave

                    // Background MouseArea (Bottom of Z-order)
                    MouseArea {
                        anchors.fill: parent
                        onWheel: {
                            if (!cppAudioEngine) return
                            var ratio = wheel.x / width
                            if (wheel.angleDelta.y > 0) {
                                cppAudioEngine.zoomAt(ratio, 0.8)
                            } else if (wheel.angleDelta.y < 0) {
                                cppAudioEngine.zoomAt(ratio, 1.25)
                            }
                            wheel.accepted = true
                        }
                        onPressed: {
                            if (!cppAudioEngine || cppAudioEngine.totalFrames <= 0) return
                            // Click outside mask jumps active window
                            var ratio = mouse.x / width
                            var len = cppAudioEngine.viewEndFrame - cppAudioEngine.viewStartFrame
                            var nStart = Math.floor(ratio * cppAudioEngine.totalFrames - len / 2)
                            var nEnd = nStart + len
                            if (nStart < 0) { nStart = 0; nEnd = len; }
                            if (nEnd > cppAudioEngine.totalFrames) { nEnd = cppAudioEngine.totalFrames; nStart = nEnd - len; }
                            cppAudioEngine.setViewRange(nStart, nEnd)
                        }
                    }

                    property real rStart: (cppAudioEngine && cppAudioEngine.totalFrames > 0 && cppAudioEngine.viewEndFrame > cppAudioEngine.viewStartFrame) ?
                                          (cppAudioEngine.viewStartFrame / cppAudioEngine.totalFrames) : 0.0
                    property real rEnd: (cppAudioEngine && cppAudioEngine.totalFrames > 0 && cppAudioEngine.viewEndFrame > cppAudioEngine.viewStartFrame) ?
                                        (cppAudioEngine.viewEndFrame / cppAudioEngine.totalFrames) : 1.0

                    property real maskX: rStart * width
                    property real maskW: Math.max(8, (rEnd - rStart) * width)

                    // Dimmed area before active window
                    Rectangle {
                        x: 0; y: 0
                        width: maskOverlay.maskX
                        height: parent.height
                        color: "#AA090B10"
                        visible: maskOverlay.maskX > 1
                    }

                    // Active window mask
                    Rectangle {
                        id: activeWindow
                        x: maskOverlay.maskX
                        y: 0
                        width: maskOverlay.maskW
                        height: parent.height
                        color: "#2500B4D8"
                        border.color: "#00B4D8"
                        border.width: 2

                        // Left Handle
                        Rectangle {
                            id: leftHandle
                            width: 10
                            height: parent.height
                            anchors.left: parent.left
                            color: leftHandleMouse.containsMouse || leftHandleMouse.drag.active ? "#00B4D8" : "#8000B4D8"

                            MouseArea {
                                id: leftHandleMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.SizeHorCursor

                                onPositionChanged: {
                                    if (pressed && cppAudioEngine && cppAudioEngine.totalFrames > 0) {
                                        var pt = mapToItem(maskOverlay, mouse.x, 0)
                                        var newStart = Math.round((pt.x / maskOverlay.width) * cppAudioEngine.totalFrames)
                                        cppAudioEngine.setViewStartFrame(newStart)
                                    }
                                }
                            }
                        }

                        // Right Handle
                        Rectangle {
                            id: rightHandle
                            width: 10
                            height: parent.height
                            anchors.right: parent.right
                            color: rightHandleMouse.containsMouse || rightHandleMouse.drag.active ? "#00B4D8" : "#8000B4D8"

                            MouseArea {
                                id: rightHandleMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.SizeHorCursor

                                onPositionChanged: {
                                    if (pressed && cppAudioEngine && cppAudioEngine.totalFrames > 0) {
                                        var pt = mapToItem(maskOverlay, mouse.x, 0)
                                        var newEnd = Math.round((pt.x / maskOverlay.width) * cppAudioEngine.totalFrames)
                                        cppAudioEngine.setViewEndFrame(newEnd)
                                    }
                                }
                            }
                        }

                        // Center body drag for panning
                        MouseArea {
                            id: panMouse
                            anchors.left: leftHandle.right
                            anchors.right: rightHandle.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            cursorShape: Qt.SizeAllCursor

                            property real startMappedX: 0
                            property real origStartFrame: 0
                            property real origEndFrame: 0

                            onPressed: {
                                startMappedX = mapToItem(maskOverlay, mouse.x, 0).x
                                origStartFrame = cppAudioEngine ? cppAudioEngine.viewStartFrame : 0
                                origEndFrame = cppAudioEngine ? cppAudioEngine.viewEndFrame : 0
                            }
                            onPositionChanged: {
                                if (pressed && cppAudioEngine && cppAudioEngine.totalFrames > 0) {
                                    var currentMappedX = mapToItem(maskOverlay, mouse.x, 0).x
                                    var deltaX = currentMappedX - startMappedX
                                    var deltaFrames = Math.round((deltaX / maskOverlay.width) * cppAudioEngine.totalFrames)
                                    var len = origEndFrame - origStartFrame
                                    var nStart = origStartFrame + deltaFrames
                                    var nEnd = origEndFrame + deltaFrames

                                    if (nStart < 0) {
                                        nStart = 0
                                        nEnd = len
                                    }
                                    if (nEnd > cppAudioEngine.totalFrames) {
                                        nEnd = cppAudioEngine.totalFrames
                                        nStart = nEnd - len
                                    }

                                    cppAudioEngine.setViewRange(nStart, nEnd)
                                }
                            }
                        }
                    }

                    // Dimmed area after active window
                    Rectangle {
                        x: maskOverlay.maskX + maskOverlay.maskW
                        y: 0
                        width: Math.max(0, parent.width - x)
                        height: parent.height
                        color: "#AA090B10"
                        visible: width > 1
                    }

                    // End of maskOverlay
                }
            }

            // Label for Detail Waveform
            RowLayout {
                Text {
                    text: "DETAIL WAVEFORM (Editable Track)"
                    font.bold: true
                    font.pixelSize: 11
                    color: "#94A3B8"
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: (cppAudioEngine && cppAudioEngine.isLoaded) ?
                          ("View: " + formatTime(cppAudioEngine.frameToSeconds(cppAudioEngine.viewStartFrame)) +
                          " - " + formatTime(cppAudioEngine.frameToSeconds(cppAudioEngine.viewEndFrame)) +
                          " | Click & Drag to Select | Wheel to Zoom") :
                          "View: 00:00.000 - 00:00.000 | Click & Drag to Select | Wheel to Zoom"
                    font.pixelSize: 11
                    color: "#64748B"
                }
            }

            // 2. BOTTOM ITEM: DETAIL WAVEFORM (EDITABLE & SELECTABLE)
            Rectangle {
                id: detailContainer
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#161824"
                radius: 6
                border.color: "#282B3E"
                clip: true

                WaveformItem {
                    id: detailWave
                    anchors.fill: parent
                    anchors.margins: 2
                    audioEngine: cppAudioEngine
                    isThumbnail: false
                    waveColor: "#00B4D8"
                    backgroundColor: "#161824"
                    selectionColor: "#55FF4D4D"
                }

                // Interactive Mouse Area for Detail: Wheel Zoom + Drag Selection
                MouseArea {
                    id: detailMouseArea
                    anchors.fill: detailWave
                    hoverEnabled: true
                    cursorShape: Qt.IBeamCursor

                    property real dragStartFrame: 0
                    property bool isSelecting: false

                    onWheel: {
                        if (!cppAudioEngine) return
                        var ratio = wheel.x / width
                        if (wheel.angleDelta.y > 0) {
                            cppAudioEngine.zoomAt(ratio, 0.8)
                        } else if (wheel.angleDelta.y < 0) {
                            cppAudioEngine.zoomAt(ratio, 1.25)
                        }
                        wheel.accepted = true
                    }

                    onPressed: {
                        if (!cppAudioEngine || !cppAudioEngine.isLoaded || cppAudioEngine.totalFrames <= 0) return
                        forceActiveFocus()
                        dragStartFrame = detailWave.xToFrame(mouse.x)
                        isSelecting = true
                        cppAudioEngine.setSelection(dragStartFrame, dragStartFrame)
                    }

                    onPositionChanged: {
                        if (isSelecting && cppAudioEngine && cppAudioEngine.isLoaded) {
                            var currentFrame = detailWave.xToFrame(mouse.x)
                            cppAudioEngine.setSelection(dragStartFrame, currentFrame)
                        }
                    }

                    onReleased: {
                        isSelecting = false
                        if (cppAudioEngine && Math.abs(cppAudioEngine.selectionEnd - cppAudioEngine.selectionStart) < 10) {
                            cppAudioEngine.setSelection(dragStartFrame, dragStartFrame)
                        }
                    }
                }
            }
        }

        // ================= BOTTOM STATUS BAR =================
        Rectangle {
            Layout.fillWidth: true
            height: 28
            color: "#10111A"
            border.color: "#1E202E"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12

                Text {
                    text: cppAudioEngine ? cppAudioEngine.statusMessage : ""
                    font.pixelSize: 11
                    color: "#94A3B8"
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: (cppAudioEngine && cppAudioEngine.isLoaded) ?
                          ("Sample Rate: " + cppAudioEngine.sampleRate + " Hz | Channels: " + cppAudioEngine.channels + " | Frames: " + cppAudioEngine.totalFrames) :
                          "No Audio Loaded"
                    font.pixelSize: 11
                    color: "#64748B"
                }
            }
        }
    }
}
