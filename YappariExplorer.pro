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
    Model/filesystemhistory.cpp \
    Model/filesystemmodel.cpp \
    Shell/contextmenu.cpp \
    Shell/directorywatcher.cpp \
    Shell/fileinforetriever.cpp \
    Shell/filesystemitem.cpp \
    Shell/shellactions.cpp \
    View/Base/baseitemdelegate.cpp \
    View/Base/basetreeview.cpp \
    View/CustomExplorer.cpp \
    View/customtabbar.cpp \
    View/customtabbarstyle.cpp \
    View/customtabwidget.cpp \
    View/customtreeview.cpp \
    View/dateitemdelegate.cpp \
    View/detailedview.cpp \
    View/expandinglineedit.cpp \
    Window/AppWindow.cpp \
    Window/TitleBar.cpp \
    Window/Win/WinFramelessWindow.cpp \
    main.cpp

HEADERS += \
    Model/filesystemhistory.h \
    Model/filesystemmodel.h \
    Shell/contextmenu.h \
    Shell/directorywatcher.h \
    Shell/fileinforetriever.h \
    Shell/filesystemitem.h \
    Shell/shellactions.h \
    View/Base/baseitemdelegate.h \
    View/Base/basetreeview.h \
    View/CustomExplorer.h \
    View/customtabbar.h \
    View/customtabbarstyle.h \
    View/customtabwidget.h \
    View/customtreeview.h \
    View/dateitemdelegate.h \
    View/detailedview.h \
    View/expandinglineedit.h \
    Window/AppWindow.h \
    Window/TitleBar.h \
    Window/Win/WinFramelessWindow.h \
    once.h \
    version.h

FORMS +=

unix {
    SOURCES += \
        Shell/Unix/unixfileinforetriever.cpp
    HEADERS += \
        Shell/Unix/unixfileinforetriever.h
    LIBS += -lstdc++fs -licui18n -licuuc
}

win32 {
    INCLUDEPATH += ThirdParty/Win/icu4c-68/include
    LIBS += -L$$PWD/ThirdParty/Win/icu4c-68/bin -licuin68 -licuuc68

    QT += gui-private
    DEFINES += WIN32_FRAMELESS
    DEFINES += _WIN32_IE=0x700 _WIN32_WINNT=0x0A00
    SOURCES += \
        Shell/Win/wincontextmenu.cpp \
        Shell/Win/winfileinforetriever.cpp \
        Shell/Win/windirectorywatcher.cpp \
        Shell/Win/windirectorywatcherv2.cpp \
        Shell/Win/winshellactions.cpp
    HEADERS += \
        Shell/Win/windirectorywatcher.h \
        Shell/Win/windirectorywatcherv2.h \
        Shell/Win/winshellactions.h \
        Shell/Win/wincontextmenu.h \
        Shell/Win/winfileinforetriever.h
    LIBS += -lole32 -lgdi32 -luuid -ldwmapi
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
