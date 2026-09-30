cmake_minimum_required(VERSION 3.16)

project(JStationPro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 5.15 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Qml
    Quick
    Multimedia
    Concurrent
)

set(SOURCES
    src/main.cpp
    src/PieceTable.h
    src/PieceTable.cpp
    src/PeakCache.h
    src/PeakCache.cpp
    src/AudioEngine.h
    src/AudioEngine.cpp
    src/AudioPlayer.h
    src/AudioPlayer.cpp
    src/WaveformItem.h
    src/WaveformItem.cpp
    qml.qrc
)

add_executable(JStationPro ${SOURCES})

target_link_libraries(JStationPro PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Qml
    Qt5::Quick
    Qt5::Multimedia
    Qt5::Concurrent
)

set_target_properties(JStationPro PROPERTIES
    OUTPUT_NAME "JStationPro"
)

# Test target
add_executable(test_piecetable
    tests/test_piecetable.cpp
    src/PieceTable.cpp
    src/PeakCache.cpp
)
target_link_libraries(test_piecetable PRIVATE Qt5::Core)

add_executable(test_audio_engine
    tests/test_audio_engine.cpp
    src/AudioEngine.cpp
    src/AudioPlayer.cpp
    src/PieceTable.cpp
    src/PeakCache.cpp
)
target_link_libraries(test_audio_engine PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Multimedia
    Qt5::Concurrent
)



