QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
DEFINES += ICU_COLLATOR

CONFIG(release, debug|release) {

    DEFINES += QT_NO_DEBUG_OUTPUT
    QMAKE_CXXFLAGS_RELEASE += -O3 -march=native

}

CONFIG(debug, debug|release) {

    # If you want to debug this application with timestamps set the following environment variable in your Qt Creator project
    # QT_MESSAGE_PATTERN="[%{time hh:mm:ss.zzz}] %{message}"

    DEFINES += CRASH_REPORT
    QMAKE_CXXFLAGS_DEBUG += -g -O0 -march=native

    win32-g++* {
        LIBS += -lDbghelp
    }
}

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Model/FileSystemModel.cpp \
    Model/SortModel.cpp \
    Model/TreeModel.cpp \
    Settings/Settings.cpp \
    Shell/ContextMenu.cpp \
    Shell/DirectoryWatcher.cpp \
    Shell/FileInfoRetriever.cpp \
    Shell/FileSystemItem.cpp \
    Shell/ShellActions.cpp \
    View/Base/BaseItemDelegate.cpp \
    View/Base/BaseTreeView.cpp \
    View/CustomExplorer.cpp \
    View/CustomTabBar.cpp \
    View/CustomTabBarStyle.cpp \
    View/CustomTabWidget.cpp \
    View/CustomTreeView.cpp \
    View/DateItemDelegate.cpp \
    View/DetailedView.cpp \
    View/ExpandingLineEdit.cpp \
    View/PathBar.cpp \
    View/PathBarButton.cpp \
    View/PathWidget.cpp \
    View/StatusBar.cpp \
    View/Util/FileSystemHistory.cpp \
    Window/AppWindow.cpp \
    Window/TitleBar.cpp \
    main.cpp

HEADERS += \
    Model/FileSystemModel.h \
    Model/SortModel.h \
    Model/TreeModel.h \
    Settings/Settings.h \
    Shell/ContextMenu.h \
    Shell/DirectoryWatcher.h \
    Shell/FileInfoRetriever.h \
    Shell/FileSystemItem.h \
    Shell/ShellActions.h \
    View/Base/BaseItemDelegate.h \
    View/Base/BaseTreeView.h \
    View/CustomExplorer.h \
    View/CustomTabBar.h \
    View/CustomTabBarStyle.h \
    View/CustomTabWidget.h \
    View/CustomTreeView.h \
    View/DateItemDelegate.h \
    View/DetailedView.h \
    View/ExpandingLineEdit.h \
    View/PathBar.h \
    View/PathBarButton.h \
    View/PathWidget.h \
    View/StatusBar.h \
    View/Util/FileSystemHistory.h \
    Window/AppWindow.h \
    Window/TitleBar.h \
    once.h \
    version.h

unix {
    SOURCES += \
    Shell/Unix/UnixFileInfoRetriever.cpp
    HEADERS += \
    Shell/Unix/UnixFileInfoRetriever.h
    LIBS += -lstdc++fs -licui18n -licuuc
}

win32 {
    INCLUDEPATH += ThirdParty/Win/icu4c-68/include
    LIBS += -L$$PWD/ThirdParty/Win/icu4c-68/bin -licuin68 -licuuc68

    #QT += gui-private
    DEFINES += WIN32_FRAMELESS
    DEFINES += _WIN32_IE=0x700 _WIN32_WINNT=0x0A00
    SOURCES += \
    Shell/Win/WinContextMenu.cpp \
    Shell/Win/WinDirChangeNotifier.cpp \
    Shell/Win/WinDirectoryWatcher.cpp \
    Shell/Win/WinFileInfoRetriever.cpp \
    Shell/Win/WinShellActions.cpp \
        Window/Win/WinFramelessWindow.cpp
    HEADERS += \
    Shell/Win/WinContextMenu.h \
    Shell/Win/WinDirChangeNotifier.h \
    Shell/Win/WinDirectoryWatcher.h \
    Shell/Win/WinFileInfoRetriever.h \
    Shell/Win/WinShellActions.h \
        Window/Win/WinFramelessWindow.h
    LIBS += -lole32 -lgdi32 -luuid -ldwmapi -loleaut32 -luxtheme
}

# PARALLEL TEST
#DEFINES += PARALLEL
#QMAKE_CXXFLAGS += -fopenmp -D_GLIBCXX_PARALLEL
#QMAKE_LFLAGS += -fopenmp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    UML/filesystem.qmodel \
    YappariExplorer.rc \
    qt.conf

RESOURCES += \
    resources.qrc

RC_FILE += \
    YappariExplorer.rc

if ( !include( ThirdParty/YappariCrashReport/YappariCrashReport.pri ) ) {
    error( Could not find the YappariCrashReport.pri file. )
}
