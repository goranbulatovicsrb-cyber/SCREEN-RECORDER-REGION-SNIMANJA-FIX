cmake_minimum_required(VERSION 3.20)
project(ScreenMasterPro VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core Widgets Gui
    Multimedia MultimediaWidgets
)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/RegionSelector.cpp
    src/AnnotationWidget.cpp
    src/CaptureManager.cpp
    src/ScreenRecorder.cpp
    src/SettingsDialog.cpp
    src/HistoryPanel.cpp
    src/TrayManager.cpp
    src/CountdownOverlay.cpp
    src/Translator.cpp
    src/FlashOverlay.cpp
    src/PinWindow.cpp
    src/RecordingCompleteDialog.cpp
    src/ManualScrollCapture.cpp
    src/OcrEngine.cpp
    src/OcrDialog.cpp
)

set(RESOURCES src/resources.qrc)

if(WIN32)
    add_executable(ScreenMasterPro WIN32 ${SOURCES} ${RESOURCES})
    target_link_libraries(ScreenMasterPro PRIVATE dwmapi uxtheme)
    target_compile_definitions(ScreenMasterPro PRIVATE
        UNICODE _UNICODE WIN32_LEAN_AND_MEAN)
else()
    add_executable(ScreenMasterPro ${SOURCES} ${RESOURCES})
endif()

target_link_libraries(ScreenMasterPro PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Multimedia
    Qt6::MultimediaWidgets
)

target_include_directories(ScreenMasterPro PRIVATE src)

if(MSVC)
    target_compile_options(ScreenMasterPro PRIVATE /W3 /utf-8)
endif()

install(TARGETS ScreenMasterPro DESTINATION .)
