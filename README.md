# JStation Pro

JStation Pro 是一个基于 **Qt 5.15**、**QML** 和 **CMake** 构建的高性能专业级音频波形编辑原型软件。

## 核心设计特性

### 1. 片表数据结构 (Piece Table)
- **非破坏性编辑**：采用大型 DAW 与现代编辑器的行业标准数据结构（片表 Piece Table），支持音频的毫秒级**剪切 (Cut)**、**复制 (Copy)**、**粘贴 (Paste)** 与**删除 (Delete)**。
- **零内存搬移**：剪切、复制与粘贴仅对元数据片段进行分裂与重组，复制操作开销接近 $O(1)$，无需在大内存中移动或重复拷贝巨量 PCM 音频数据。
- **只读基底与追加缓冲**：原始音频数据 (`OriginalBuffer`) 保持只读不可变，新增数据 (`AddBuffer`) 仅追加。

### 2. 多级细节层次波形缓存 (LOD Peak Pyramid)
- 为底层数据维护 5 级波形峰值缓存金字塔（64、256、1024、4096、16384 采样点/像素）。
- 在波形绘制时，自动将屏幕逻辑范围映射为底层物理片段并快速从缓存检索峰值，数千像素宽度的波形渲染在 **< 0.5 毫秒** 内完成，彻底保障缩放与拖拽时的 60 FPS 流畅交互。

### 3. 双波形图联动与交互蒙层 (QQuickPaintedItem)
- **上方视图（缩略图 Overview）**：展示音频全貌，支持鼠标滚轮缩放，带有动态高亮蒙层。支持拖拽蒙层左右手柄自由调节视口范围，支持拖拽蒙层主体进行视口平移。
- **下方视图（细节视图 Detail）**：与上方视口蒙层毫秒级双向联动，支持以鼠标指针为中心进行滚轮缩放。
- **横向选区**：在细节波形图上自由框选时间段（浅红色区域高亮），支持 `Ctrl+X`、`Ctrl+C`、`Ctrl+V`、`Delete` 等快捷键及工具栏操作，编辑后波形图实时重绘。

### 4. 异步音频导入与解析
- 采用 `QAudioDecoder` 异步解码与 WAV 快速通道，支持 MP3、WAV、FLAC、AAC、M4A 等常见音频格式。
- 导入和解析时提供实时进度条与状态显示。

---

## 目录结构

```text
├── CMakeLists.txt        # CMake 工程配置文件
├── README.md             # 工程说明文档
├── .gitignore            # Git 忽略规则配置
├── doc/
│   └── audio_editor_plan.md # 中英双语技术方案设计文档
├── src/
│   ├── main.cpp          # 程序入口
│   ├── AudioEngine.h/.cpp# 核心音频管理、异步解码与 QML 桥接
│   ├── PieceTable.h/.cpp # Piece Table 数据结构实现
│   ├── PeakCache.h/.cpp  # 多级波形峰值缓存
│   └── WaveformItem.h/.cpp # 基于 QQuickPaintedItem 的高性能波形绘制
├── qml/
│   └── main.qml          # QML 现代化界面实现
├── tests/
│   └── test_piecetable.cpp # Piece Table 核心算法单元测试
└── qml.qrc               # QML 资源文件
```

---

## 构建与运行

### 依赖环境
- **C++ 编译器**：支持 C++17（如 MSVC 2019 / 2022、GCC 9+、Clang 10+）
- **CMake**：>= 3.16
- **Qt**：Qt 5.15（包含 `Core`, `Gui`, `Qml`, `Quick`, `Multimedia`, `Concurrent` 模块）

### 构建步骤

```powershell
# 1. 配置工程 (指定 Qt5 路径)
cmake -B build -S . -DCMAKE_PREFIX_PATH="<Your-Qt5-Path>"

# 2. 编译 Release 版本
cmake --build build --config Release

# 3. 运行单元测试
./build/Release/test_piecetable.exe

# 4. 运行主程序
./build/Release/JStationPro.exe
```
