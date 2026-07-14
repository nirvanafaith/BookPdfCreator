#-------------------------------------------------
#
# Project created by QtCreator
#
#-------------------------------------------------

QT       += core gui widgets printsupport gui-private concurrent

TARGET = BookPdfCreator
TEMPLATE = app

# C++11 标准
CONFIG += c++11

# UTF-8 源文件编码配置
msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}

# Windows 7 兼容配置
win32 {
    # 兼容 Windows 7
    QMAKE_TARGET_OS = win7
    DEFINES += WINVER=0x0601 _WIN32_WINNT=0x0601
    # 静态链接运行时库（可选，根据需要启用）
    # QMAKE_CXXFLAGS_RELEASE += /MT
    # QMAKE_LFLAGS_RELEASE += /INCREMENT:NO /MANIFESTUAC:"level='asInvoker' uiAccess='false'"
}

# 禁用过时API（Qt5.12兼容）
DEFINES += QT_DEPRECATED_WARNINGS

# miniz库配置
DEFINES += MINIZ_NO_TIME
DEFINES += MINIZ_NO_ARCHIVE_WRITING_APIS
win32 {
    DEFINES += MINIZ_LITTLE_ENDIAN=1
    DEFINES += MINIZ_HAS_64BIT_REGISTERS=1
    DEFINES += MINIZ_USE_UNALIGNED_LOADS_AND_STORES=0
}

# 源代码目录
SOURCES += \
    src/main.cpp \
    src/ui/MainWindow.cpp \
    src/ui/AssetTreeWidget.cpp \
    src/ui/PaperSizeDialog.cpp \
    src/ui/PdfPreviewWidget.cpp \
    src/models/Book.cpp \
    src/parsers/ExcelParser.cpp \
    src/parsers/ImageFinder.cpp \
    src/parsers/ImageCache.cpp \
    src/utils/ZipReader.cpp \
    src/layout/PainterHelpers.cpp \
    src/layout/LayoutEngine.cpp \
    src/layout/SingleColumnLayout.cpp \
    src/layout/MultiColumnLayout.cpp \
    src/layout/AutoLayoutDetector.cpp \
    src/pdf/PdfExporter.cpp \
    src/editor/PageElement.cpp \
    src/editor/PageData.cpp \
    src/editor/ElementRenderer.cpp \
    src/editor/BaseEditorItem.cpp \
    src/editor/TextEditorItem.cpp \
    src/editor/ImageEditorItem.cpp \
    src/editor/ShapeEditorItem.cpp \
    src/editor/SelectionHandle.cpp \
    src/editor/SelectionDecorator.cpp \
    src/editor/EditorScene.cpp \
    src/editor/EditorView.cpp \
    src/editor/Commands.cpp \
    src/editor/TextFormatToolbar.cpp \
    src/editor/LayerPanel.cpp \
    src/editor/PropertyPanel.cpp \
    src/editor/SnapGuide.cpp \
    src/editor/GuideLineItem.cpp \
    third_party/miniz/miniz.c \
    third_party/miniz/miniz_tdef.c \
    third_party/miniz/miniz_tinfl.c \
    third_party/miniz/miniz_zip.c \

HEADERS += \
    src/ui/MainWindow.h \
    src/ui/AssetTreeWidget.h \
    src/ui/PaperSizeDialog.h \
    src/ui/PdfPreviewWidget.h \
    src/models/Book.h \
    src/models/LayoutMode.h \
    src/parsers/ExcelParser.h \
    src/parsers/ImageFinder.h \
    src/parsers/ImageCache.h \
    src/utils/ZipReader.h \
    src/layout/LayoutConstants.h \
    src/layout/PainterHelpers.h \
    src/layout/LayoutEngine.h \
    src/layout/SingleColumnLayout.h \
    src/layout/MultiColumnLayout.h \
    src/layout/AutoLayoutDetector.h \
    src/pdf/PdfExporter.h \
    src/editor/PageElement.h \
    src/editor/PageData.h \
    src/editor/ElementRenderer.h \
    src/editor/BaseEditorItem.h \
    src/editor/TextEditorItem.h \
    src/editor/ImageEditorItem.h \
    src/editor/ShapeEditorItem.h \
    src/editor/SelectionHandle.h \
    src/editor/SelectionDecorator.h \
    src/editor/EditorScene.h \
    src/editor/EditorView.h \
    src/editor/Commands.h \
    src/editor/TextFormatToolbar.h \
    src/editor/LayerPanel.h \
    src/editor/PropertyPanel.h \
    src/editor/SnapGuide.h \
    src/editor/GuideLineItem.h \
    third_party/miniz/miniz.h \
    third_party/miniz/miniz_common.h \
    third_party/miniz/miniz_export.h \
    third_party/miniz/miniz_tdef.h \
    third_party/miniz/miniz_tinfl.h \
    third_party/miniz/miniz_zip.h \

# 包含路径
INCLUDEPATH += \
    src \
    src/models \
    src/parsers \
    src/layout \
    src/pdf \
    src/ui \
    src/utils \
    src/editor \
    third_party \
    third_party/miniz \
    third_party/QXlsx-master \
    third_party/QXlsx-master/QXlsx/header

# QXlsx 集成（子目录编译）
QXLSX_PARENTPATH = $$PWD/third_party/QXlsx-master/
QXLSX_HEADERPATH = $$PWD/third_party/QXlsx-master/QXlsx/header/
QXLSX_SOURCEPATH = $$PWD/third_party/QXlsx-master/QXlsx/source/
include(third_party/QXlsx-master/QXlsx/QXlsx.pri)

# 资源文件
RESOURCES += \
    resources/resources.qrc

# 默认部署规则
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# 输出目录
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/build/debug
} else {
    DESTDIR = $$PWD/build/release
}

OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc
RCC_DIR = $$PWD/build/rcc
UI_DIR = $$PWD/build/ui
