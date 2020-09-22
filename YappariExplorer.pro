QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

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
    Model/filesystemmodel.cpp \
    Shell/Win/windirectorywatcher.cpp \
    Shell/Win/winshellactions.cpp \
    Shell/contextmenu.cpp \
    Shell/fileinforetriever.cpp \
    Shell/filesystemitem.cpp \
    Shell/shellactions.cpp \
    View/Base/baseitemdelegate.cpp \
    View/Base/basetreeview.cpp \
    View/customexplorer.cpp \
    View/customtabbar.cpp \
    View/customtabbarstyle.cpp \
    View/customtabwidget.cpp \
    View/customtreeview.cpp \
    View/dateitemdelegate.cpp \
    View/detailedview.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    Model/filesystemmodel.h \
    Shell/Win/windirectorywatcher.h \
    Shell/Win/winshellactions.h \
    Shell/contextmenu.h \
    Shell/fileinforetriever.h \
    Shell/filesystemitem.h \
    Shell/shellactions.h \
    View/Base/baseitemdelegate.h \
    View/Base/basetreeview.h \
    View/customexplorer.h \
    View/customtabbar.h \
    View/customtabbarstyle.h \
    View/customtabwidget.h \
    View/customtreeview.h \
    View/dateitemdelegate.h \
    View/detailedview.h \
    mainwindow.h \
    once.h

FORMS += \
    mainwindow.ui

unix {
    CONFIG(debug, debug|release) {
        SOURCES += \
            Shell/Unix/unixfileinforetriever.cpp
        HEADERS += \
            Shell/Unix/unixfileinforetriever.h
        LIBS += -lstdc++fs
    }
}

win32 {
    CONFIG(debug, debug|release) {
        DEFINES += _WIN32_IE=0x700
        SOURCES += \
            Shell/Win/wincontextmenu.cpp \
            Shell/Win/winfileinforetriever.cpp
        HEADERS += \
            Shell/Win/wincontextmenu.h \
            Shell/Win/winfileinforetriever.h
        LIBS += -lole32 -lgdi32 -luuid
    }
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    UML/filesystem.qmodel \
    qt.conf

RESOURCES += \
    resources.qrc

RC_FILE += \
    YappariExplorer.rc
