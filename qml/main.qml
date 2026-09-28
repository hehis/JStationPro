import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.3
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

    // Drag and drop file support
    DropArea {
        anchors.fill: parent
        onDropped: {
            if (drop.hasUrls && drop.urls.length > 0) {
                audioEngine.openAudioFile(drop.urls[0])
            }
        }
    }

    // Keyboard shortcuts for editing
    Item {
        focus: true
        Keys.onPressed: {
            if (event.matches(StandardKey.Copy)) {
                audioEngine.copySelection()
                event.accepted = true
            } else if (event.matches(StandardKey.Cut)) {
                audioEngine.cutSelection()
                event.accepted = true
            } else if (event.matches(StandardKey.Paste)) {
                var insertPos = audioEngine.hasSelection ? audioEngine.selectionStart : audioEngine.viewStartFrame
                audioEngine.pasteAt(insertPos)
                event.accepted = true
            } else if (event.matches(StandardKey.Delete) || event.key === Qt.Key_Backspace) {
                audioEngine.deleteSelection()
                event.accepted = true
            } else if (event.matches(StandardKey.SelectAll)) {
                audioEngine.setSelection(0, audioEngine.totalFrames)
                event.accepted = true
            }
        }
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

                // Import Button
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
                    onClicked: fileDialog.open()
                }

                // Edit Buttons
                Button {
                    text: "Cut (Ctrl+X)"
                    font.pixelSize: 12
                    enabled: audioEngine.hasSelection
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
                    onClicked: audioEngine.cutSelection()
                }

                Button {
                    text: "Copy (Ctrl+C)"
                    font.pixelSize: 12
                    enabled: audioEngine.hasSelection
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
                    onClicked: audioEngine.copySelection()
                }

                Button {
                    text: "Paste (Ctrl+V)"
                    font.pixelSize: 12
                    enabled: audioEngine.canPaste
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
                        var pos = audioEngine.hasSelection ? audioEngine.selectionStart : audioEngine.viewStartFrame
                        audioEngine.pasteAt(pos)
                    }
                }

                Button {
                    text: "Delete (Del)"
                    font.pixelSize: 12
                    enabled: audioEngine.hasSelection
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
                    onClicked: audioEngine.deleteSelection()
                }

                Rectangle { width: 1; height: 28; color: "#2B2D3C"; Layout.leftMargin: 8; Layout.rightMargin: 8 }

                // Zoom controls
                Button {
                    text: "Zoom In (+)"
                    font.pixelSize: 12
                    enabled: audioEngine.isLoaded
                    contentItem: Text {
                        text: parent.text; font: parent.font
                        color: parent.enabled ? "#F1F5F9" : "#64748B"
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.enabled ? (parent.hovered ? "#334155" : "#1E293B") : "#161E2E"
                        radius: 5
                    }
                    onClicked: audioEngine.zoomAt(0.5, 0.7)
                }

                Button {
                    text: "Zoom Out (-)"
                    font.pixelSize: 12
                    enabled: audioEngine.isLoaded
                    contentItem: Text {
                        text: parent.text; font: parent.font
                        color: parent.enabled ? "#F1F5F9" : "#64748B"
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.enabled ? (parent.hovered ? "#334155" : "#1E293B") : "#161E2E"
                        radius: 5
                    }
                    onClicked: audioEngine.zoomAt(0.5, 1.4)
                }

                Button {
                    text: "Reset View"
                    font.pixelSize: 12
                    enabled: audioEngine.isLoaded
                    contentItem: Text {
                        text: parent.text; font: parent.font
                        color: parent.enabled ? "#F1F5F9" : "#64748B"
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: parent.enabled ? (parent.hovered ? "#334155" : "#1E293B") : "#161E2E"
                        radius: 5
                    }
                    onClicked: audioEngine.resetView()
                }

                Item { Layout.fillWidth: true }

                // Selection info
                Text {
                    visible: audioEngine.hasSelection
                    text: "Selected: " + formatTime(audioEngine.frameToSeconds(audioEngine.selectionEnd - audioEngine.selectionStart)) +
                          " (" + audioEngine.selectionStart + " - " + audioEngine.selectionEnd + ")"
                    font.pixelSize: 12
                    color: "#F87171"
                }
            }
        }

        // ================= PROGRESS BAR (DURING IMPORT) =================
        Rectangle {
            Layout.fillWidth: true
            height: audioEngine.isDecoding ? 30 : 0
            visible: audioEngine.isDecoding
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
                    value: audioEngine.decodeProgress
                }

                Text {
                    text: Math.floor(audioEngine.decodeProgress * 100) + "%"
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
                    text: "Total: " + formatTime(audioEngine.durationSeconds) + " | Zoom via Wheel | Drag Mask Handles to Resize"
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
                    audioEngine: audioEngine
                    isThumbnail: true
                    waveColor: "#48CAE4"
                    backgroundColor: "#161824"
                }

                // Interactive Mask Overlay for Thumbnail
                Item {
                    id: maskOverlay
                    anchors.fill: thumbWave

                    property real rStart: (audioEngine.totalFrames > 0 && audioEngine.viewEndFrame > audioEngine.viewStartFrame) ?
                                          (audioEngine.viewStartFrame / audioEngine.totalFrames) : 0.0
                    property real rEnd: (audioEngine.totalFrames > 0 && audioEngine.viewEndFrame > audioEngine.viewStartFrame) ?
                                        (audioEngine.viewEndFrame / audioEngine.totalFrames) : 1.0

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

                                property real startX: 0
                                property real origFrame: 0

                                onPressed: {
                                    startX = mouse.x
                                    origFrame = audioEngine.viewStartFrame
                                }
                                onPositionChanged: {
                                    if (pressed && audioEngine.totalFrames > 0) {
                                        var deltaX = mouse.x - startX
                                        var deltaFrames = (deltaX / maskOverlay.width) * audioEngine.totalFrames
                                        var newStart = Math.round(origFrame + deltaFrames)
                                        audioEngine.setViewStartFrame(newStart)
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

                                property real startX: 0
                                property real origFrame: 0

                                onPressed: {
                                    startX = mouse.x
                                    origFrame = audioEngine.viewEndFrame
                                }
                                onPositionChanged: {
                                    if (pressed && audioEngine.totalFrames > 0) {
                                        var deltaX = mouse.x - startX
                                        var deltaFrames = (deltaX / maskOverlay.width) * audioEngine.totalFrames
                                        var newEnd = Math.round(origFrame + deltaFrames)
                                        audioEngine.setViewEndFrame(newEnd)
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

                            property real startMouseX: 0
                            property real origStartFrame: 0
                            property real origEndFrame: 0

                            onPressed: {
                                startMouseX = mouse.x
                                origStartFrame = audioEngine.viewStartFrame
                                origEndFrame = audioEngine.viewEndFrame
                            }
                            onPositionChanged: {
                                if (pressed && audioEngine.totalFrames > 0) {
                                    var deltaX = mouse.x - startMouseX
                                    var deltaFrames = Math.round((deltaX / maskOverlay.width) * audioEngine.totalFrames)
                                    var len = origEndFrame - origStartFrame
                                    var nStart = origStartFrame + deltaFrames
                                    var nEnd = origEndFrame + deltaFrames

                                    if (nStart < 0) {
                                        nStart = 0
                                        nEnd = len
                                    }
                                    if (nEnd > audioEngine.totalFrames) {
                                        nEnd = audioEngine.totalFrames
                                        nStart = nEnd - len
                                    }

                                    audioEngine.setViewRange(nStart, nEnd)
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

                    // Mouse Wheel on Thumbnail: Zooms the mask
                    MouseArea {
                        anchors.fill: parent
                        propagateComposedEvents: true
                        onWheel: {
                            var ratio = wheel.x / width
                            if (wheel.angleDelta.y > 0) {
                                audioEngine.zoomAt(ratio, 0.8)
                            } else if (wheel.angleDelta.y < 0) {
                                audioEngine.zoomAt(ratio, 1.25)
                            }
                            wheel.accepted = true
                        }
                        onPressed: {
                            // Click outside mask jumps active window
                            if (mouse.x < activeWindow.x || mouse.x > activeWindow.x + activeWindow.width) {
                                var ratio = mouse.x / width
                                var len = audioEngine.viewEndFrame - audioEngine.viewStartFrame
                                var nStart = Math.floor(ratio * audioEngine.totalFrames - len / 2)
                                var nEnd = nStart + len
                                if (nStart < 0) { nStart = 0; nEnd = len; }
                                if (nEnd > audioEngine.totalFrames) { nEnd = audioEngine.totalFrames; nStart = nEnd - len; }
                                audioEngine.setViewRange(nStart, nEnd)
                            }
                        }
                    }
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
                    text: "View: " + formatTime(audioEngine.frameToSeconds(audioEngine.viewStartFrame)) +
                          " - " + formatTime(audioEngine.frameToSeconds(audioEngine.viewEndFrame)) +
                          " | Click & Drag to Select | Wheel to Zoom"
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
                    audioEngine: audioEngine
                    isThumbnail: false
                    viewStartFrame: audioEngine.viewStartFrame
                    viewEndFrame: audioEngine.viewEndFrame
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
                        var ratio = wheel.x / width
                        if (wheel.angleDelta.y > 0) {
                            audioEngine.zoomAt(ratio, 0.8)
                        } else if (wheel.angleDelta.y < 0) {
                            audioEngine.zoomAt(ratio, 1.25)
                        }
                        wheel.accepted = true
                    }

                    onPressed: {
                        if (!audioEngine.isLoaded || audioEngine.totalFrames <= 0) return
                        forceActiveFocus()
                        dragStartFrame = detailWave.xToFrame(mouse.x)
                        isSelecting = true
                        audioEngine.setSelection(dragStartFrame, dragStartFrame)
                    }

                    onPositionChanged: {
                        if (isSelecting && audioEngine.isLoaded) {
                            var currentFrame = detailWave.xToFrame(mouse.x)
                            audioEngine.setSelection(dragStartFrame, currentFrame)
                        }
                    }

                    onReleased: {
                        isSelecting = false
                        // If tiny selection (< 10 frames), clear selection
                        if (Math.abs(audioEngine.selectionEnd - audioEngine.selectionStart) < 10) {
                            audioEngine.clearSelection()
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
                    text: audioEngine.statusMessage
                    font.pixelSize: 11
                    color: "#94A3B8"
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: audioEngine.isLoaded ?
                          ("Sample Rate: " + audioEngine.sampleRate + " Hz | Channels: " + audioEngine.channels + " | Frames: " + audioEngine.totalFrames) :
                          "No Audio Loaded"
                    font.pixelSize: 11
                    color: "#64748B"
                }
            }
        }
    }

    // File Dialog for Audio Import
    FileDialog {
        id: fileDialog
        title: "Please choose an audio file"
        nameFilters: [
            "All supported audio (*.wav *.mp3 *.flac *.aac *.ogg *.m4a *.wma)",
            "Wave files (*.wav)",
            "MP3 files (*.mp3)",
            "All files (*.*)"
        ]
        onAccepted: {
            audioEngine.openAudioFile(fileDialog.fileUrl)
        }
    }
}
