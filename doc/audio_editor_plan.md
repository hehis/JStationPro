# Implementation Plan / 技术实现方案

## English Version

### Goal Description
Build a Qt5.15 (QML/C++) audio editor prototype utilizing the Piece Table data structure for efficient non-destructive editing (Cut, Copy, Paste, Delete) and LOD (Level of Detail) caching for high-performance waveform rendering. The app will feature a dual-view waveform UI (thumbnail + detail) with synchronization and interactive masking.

### User Review Required
> [!IMPORTANT]
> - **In-Memory vs Disk-Backed**: For the `OriginalBuffer`, decoding a 5-minute stereo audio file takes about 50MB of RAM. For this prototype, I propose keeping the decoded PCM data **entirely in memory** (`QByteArray`) to simplify the implementation. Memory mapping (`QFile::map`) can be added later for gigabyte-level files. Do you agree?
> - **Internal Clipboard**: The Copy/Paste functionality will use an **internal application clipboard** (copying logical Piece references) rather than the OS-level clipboard (which would require exporting raw PCM data). Is this acceptable?
> - **Playback**: Your requirements focus on importing, rendering, and editing. I have **not** included audio playback (`QAudioOutput`/`QAudioSink`) in this plan, only the data structures, rendering, and editing logic. Let me know if playback is required for this phase.

### Proposed Changes
#### Core Data Structures
*   **[NEW] `src/PieceTable.h` / `src/PieceTable.cpp`**
    *   Implements the Piece Table and LOD Peak Cache logic.
    *   `enum class BufferSource { Original, Add }`
    *   `struct Piece { BufferSource source; qint64 offset; qint64 length; }`
    *   `class PieceTable`: Manages a `std::list<Piece>`. Implements `insert`, `remove`, `copyToClipboard`, `pasteFromClipboard`.
    *   `class PeakCache`: Pre-computes and stores `std::vector<std::pair<short, short>>` (min/max) for various zoom levels.

#### Audio Decoding and Controller
*   **[NEW] `src/AudioController.h` / `src/AudioController.cpp`**
    *   Uses `QAudioDecoder` to asynchronously decode imported files to PCM.
    *   Exposes signals for progress: `decodeProgress(float)`.
    *   Triggers `PeakCache` generation in a background `QThreadPool` task after decoding.
    *   Exposes editing slots to QML: `cut()`, `copy()`, `paste()`, `deleteSelection()`.

#### UI Rendering
*   **[NEW] `src/WaveformItem.h` / `src/WaveformItem.cpp`**
    *   Subclasses `QQuickPaintedItem` for QML.
    *   Properties: `logicalStart`, `logicalEnd`, `selectionStart`, `selectionEnd`.
    *   `paint()`: Resolves logical boundaries to physical pieces via `PieceTable`, fetches LOD data from `PeakCache`, and draws the waveform and selection mask.

#### QML Interface
*   **[NEW] `qml/main.qml`**
    *   **Top Item (Thumbnail)**: Displays full duration. Contains a draggable/resizable mask.
    *   **Bottom Item (Detail)**: Binds display range to the top mask. Synchronizes wheel zooming.
    *   Implements mouse areas for selecting (red overlay) and buttons for edits.

#### Build System
*   **[NEW] `CMakeLists.txt`**: Standard Qt5 CMake setup.

### Verification Plan
1. Run application and import audio. Observe progress.
2. Use mouse wheel to zoom; verify synchronization between views.
3. Drag mask edges; verify bottom waveform updates.
4. Select a region, perform Cut/Paste; verify instant waveform updates.

---

## 中文版 (Chinese Version)

### 目标描述
构建一个基于 Qt5.15 (QML/C++) 的音频编辑器原型。该原型将使用片表（Piece Table）数据结构来实现高效的非破坏性编辑（剪切、复制、粘贴、删除），并结合细节层次（LOD）缓存技术来实现高性能的波形渲染。应用程序将包含一个双视图波形 UI（缩略图 + 细节视图），支持同步缩放和交互式蒙层控制。

### 需要用户确认的事项
> [!IMPORTANT]
> - **内存存储 vs 磁盘映射**：对于 `OriginalBuffer`（原始数据），解码一首 5 分钟的双声道音频大约需要 50MB 内存。为了简化原型的实现，我建议将解码后的 PCM 数据**全部保存在内存中**（使用 `QByteArray`）。如果未来需要处理 GB 级别的大文件，再引入内存映射（`QFile::map`）。你是否同意这个做法？
> - **内部剪切板**：复制/粘贴功能将使用**应用程序内部剪切板**（仅复制逻辑片段的引用），而不是操作系统的系统剪切板（那需要导出真实的 PCM 数据）。这样可以吗？
> - **播放功能**：你的需求主要集中在导入、渲染和编辑上。在当前的计划中，我**没有**包含音频播放功能（`QAudioOutput`）。如果当前阶段就需要播放功能，请告诉我。

### 拟定修改
#### 核心数据结构
*   **[NEW] `src/PieceTable.h` / `src/PieceTable.cpp`**
    *   实现 Piece Table 和 LOD 波形峰值缓存逻辑。
    *   `enum class BufferSource { Original, Add }`
    *   `struct Piece { BufferSource source; qint64 offset; qint64 length; }`
    *   `class PieceTable`: 管理 `std::list<Piece>`。实现 `insert`, `remove`, `copyToClipboard`, `pasteFromClipboard`。
    *   `class PeakCache`: 为不同的缩放级别预先计算并存储 `std::vector<std::pair<short, short>>`（最大值/最小值）。

#### 音频解码与控制器
*   **[NEW] `src/AudioController.h` / `src/AudioController.cpp`**
    *   使用 `QAudioDecoder` 异步解码导入的音频为 PCM。
    *   提供进度信号：`decodeProgress(float)`。
    *   解码完成后，在后台 `QThreadPool` 任务中触发 `PeakCache` 生成。
    *   向 QML 暴露编辑接口：`cut()`, `copy()`, `paste()`, `deleteSelection()`。

#### UI 渲染
*   **[NEW] `src/WaveformItem.h` / `src/WaveformItem.cpp`**
    *   继承 `QQuickPaintedItem` 供 QML 使用。
    *   属性：`logicalStart`, `logicalEnd`, `selectionStart`, `selectionEnd`。
    *   `paint()`: 通过 `PieceTable` 将逻辑边界解析为物理片段，从 `PeakCache` 获取 LOD 数据，并绘制波形和选择蒙版。

#### QML 界面
*   **[NEW] `qml/main.qml`**
    *   **上方 Item (缩略图)**: 显示完整时长。包含一个可拖拽/调整大小的蒙层。
    *   **下方 Item (细节图)**: 其显示范围绑定到上方蒙层。支持滚轮缩放并与上方同步。
    *   实现鼠标区域用于框选（红色遮罩层），并提供编辑操作按钮。

#### 构建系统
*   **[NEW] `CMakeLists.txt`**: 标准的 Qt5 CMake 配置。

### 验证计划
1. 运行程序，导入音频，观察异步解析进度条。
2. 使用鼠标滚轮缩放；验证上下两个视图是否同步。
3. 拖拽上方蒙层边缘；验证下方波形视图是否正确更新。
4. 在下方波形上框选区域，进行剪切/粘贴操作；验证波形是否能瞬间完成重新渲染。
