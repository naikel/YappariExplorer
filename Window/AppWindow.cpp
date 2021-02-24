/*
 * Copyright (C) 2019, 2020, 2021 Naikel Aparicio. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ''AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the author and should not be interpreted as representing
 * official policies, either expressed or implied, of the copyright holder.
 */

#include <QApplication>
#include <QHeaderView>
#include <QJsonArray>
#include <QDebug>
#include <QWindow>
#include <QScreen>
#include <QVBoxLayout>

#include "AppWindow.h"

#include "View/CustomExplorer.h"

#define APPLICATION_TITLE   "Yappari Explorer"

#ifdef Q_OS_WIN

#include "Shell/Win/wincontextmenu.h"
#define PlatformContextMenu(PARENT)     WinContextMenu(PARENT)

#else
#include "Shell/Unix/unixcontextmenu.h"
#define PlatformContextMenu(PARENT)     UnixContextMenu(PARENT)

#endif

AppWindow *AppWindow::appWindow                        {};

AppWindow::AppWindow(QWidget *parent) : MAINWINDOW(parent)
{
    appWindow = this;

#ifdef Q_OS_WIN
    nextId = WM_USER + 200;
#endif

    Settings::settings = new Settings();
    connect(QApplication::instance(), &QApplication::aboutToQuit, this, &AppWindow::saveSettings);

    // Application settings
    setWindowIcon(QIcon(":/icons/app2.png"));
    setWindowTitle(APPLICATION_TITLE);

    contextMenu = new PlatformContextMenu(this);

    setupGui();
}

AppWindow::~AppWindow()
{
    qDebug() << "AppWindow::~AppWindow Destroyed";
}

quint32 AppWindow::registerWatcher(DirectoryWatcher *watcher)
{
    regMutex.lock();

    quint32 id = nextId++;
    watchers.insert(id, watcher);

    qDebug() << "AppWindow::registerWatcher watcher registered with id =" << id;

    regMutex.unlock();
    return id;
}

AppWindow *AppWindow::instance()
{
    return appWindow;
}

QWidget *AppWindow::contentWidget()
{
    return CENTRALWIDGET;
}

void AppWindow::showContextMenu(const QPoint &pos, const QList<FileSystemItem *> fileSystemItems, const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view)
{
    qDebug() << "AppWindow::showContextMenu";

    contextMenu->show(winId(), pos, fileSystemItems, viewAspect, view);
}

void AppWindow::defaultAction(const FileSystemItem *fileSystemItem)
{
    qDebug() << "AppWindow::defaultAction";

    contextMenu->defaultAction(winId(), fileSystemItem);
}

void AppWindow::updateTitle(FileSystemItem *item)
{
#ifdef WIN32_FRAMELESS
    QString title = APPLICATION_TITLE;
    if (item != nullptr) {
        title = item->getDisplayName() + " - " + title;
    }
    setWindowTitle(title);
#else
    Q_UNUSED(item)
#endif
}

bool AppWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    quint32 id {};

#ifdef Q_OS_WIN
    MSG *msg = static_cast< MSG * >(message);
    id = msg->message;
#endif

    if (watchers.keys().contains(id)) {
        DirectoryWatcher *watcher = watchers.value(id);
        return watcher->handleNativeEvent(eventType, message, result);
    }

    if (contextMenu && contextMenu->handleNativeEvent(eventType, message, result))
        return true;

    return MAINWINDOW::nativeEvent(eventType, message, result);
}

void AppWindow::resizeEvent(QResizeEvent *event)
{
    MAINWINDOW::resizeEvent(event);

    if (!isMaximized() && !event->size().isEmpty())
        prevSize = event->size();
}

void AppWindow::setupGui()
{
#ifndef WIN32_FRAMELESS
    QWidget *cWidget = new QWidget(this);
    cWidget->setObjectName(QString::fromUtf8("centralwidget"));
    QVBoxLayout *verticalLayout = new QVBoxLayout(cWidget);
    verticalLayout->setSpacing(0);
    verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    setCentralWidget(cWidget);
#endif

    QWidget *w;
    if (nExplorers > 1) {
        QSplitter *splitter = new QSplitter(contentWidget());
        splitter->setObjectName(QString::fromUtf8("splitter"));
        splitter->setOrientation(verticalMode ? Qt::Vertical : Qt::Horizontal);
        splitter->setStyleSheet("QSplitter::handle { background: #9fcdb3;  } QSplitter::handle:vertical { height: 1px; }");
        w = splitter;
    } else
        w = contentWidget();

    // Create explorers
    CustomExplorer *cExplorer {};
    for (int i = 0; i < nExplorers; i++) {
        cExplorer = new CustomExplorer(i, w);
        cExplorer->setObjectName(QString::fromUtf8("customExplorer") + QString::number(i));

        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(cExplorer->sizePolicy().hasHeightForWidth());

        cExplorer->setSizePolicy(sizePolicy);
        cExplorer->setFrameShape(QFrame::NoFrame);
        cExplorer->setFrameShadow(QFrame::Raised);
        cExplorer->setLineWidth(0);

        QSplitter *cSplitter = cExplorer->getSplitter();
        splitters.append(cExplorer->getSplitter());
        explorers.append(cExplorer);

        if (verticalMode)
            connect(cExplorer->getTreeView(), &CustomTreeView::resized, this, [this, cSplitter]() { this->resizeOtherSplitter(cSplitter); } );
    }

    if (nExplorers == 1) {
        contentWidget()->layout()->addWidget(cExplorer);
    } else {
       QSplitter *splitter = dynamic_cast<QSplitter *>(w);
       splitter->addWidget(cExplorer);
       contentWidget()->layout()->addWidget(splitter);

       QJsonArray sizes = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_SPLITTER_SIZES).toArray();
       if (!sizes.isEmpty()) {
           QList<int> intSizes;
           for (QJsonValue size : sizes)
               intSizes.append(size.toInt());

           splitter->setSizes(intSizes);
       }
    }

    // Default main window settings
    setMinimumWidth(getPhysicalPixels(350));

    QWindow *window = this->windowHandle();

    if (window != nullptr) {
        qDebug() << "AppWindow::setupGui about to restore windows geometry" << window->screen()->name();

        QString screenName = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_SCREEN).toString();

        for (QScreen *screen : QApplication::screens())
            if (screen->name() == screenName)
                window->setScreen(screen);
    }

    int x = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_X).toInt();
    int y = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_Y).toInt();
    int height = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_HEIGHT).toInt();
    int width = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_WIDTH).toInt();

    prevSize = QSize(width, height);

    if (height >= 0 && width >= 0)
        resize(width, height);
    else
        resize(getPhysicalPixels(1024), getPhysicalPixels(768));

    if (x >= 0)
        move(x, y);
    else {
#ifdef WIN32_FRAMELESS
        // Center the window in the screen
        QRect ag = this->windowHandle()->screen()->availableGeometry();
        QRect wg({}, frameSize().boundedTo(ag.size()));
        move(ag.center() - wg.center());
#endif
    }

    if (Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_MAXIMIZED).toBool()) {
        showMaximized();
    }


    qDebug() << "AppWindow::setupGui windows geometry restored";
}

void AppWindow::saveSettings()
{
    for (const CustomExplorer *cExplorer : explorers)
       cExplorer->saveSettings();

    Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_X, x());
    Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_Y, y());
    Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_SCREEN, this->windowHandle()->screen()->name());

    Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_MAXIMIZED, isMaximized());
    if (isMaximized()) {
        Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_WIDTH, prevSize.width());
        Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_HEIGHT, prevSize.height());
    } else {
        Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_WIDTH, width());
        Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_HEIGHT, height());
    }

    Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_EXPLORERS, nExplorers);
    if (nExplorers > 1) {
        QSplitter *splitter = dynamic_cast<QSplitter *>(contentWidget()->layout()->itemAt(0)->widget());
        QJsonArray sizes;
        for (int size : splitter->sizes())
            sizes.append(size);
        Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_SPLITTER_SIZES, sizes);
    }

    Settings::settings->save();
}

void AppWindow::resizeOtherSplitter(QSplitter *splitter)
{
    if (nExplorers > 1) {

        QSplitter *otherSplitter {};
        for (QSplitter *s : splitters)
            if (splitter != s)
                otherSplitter = s;

        if (otherSplitter != nullptr) {
            QList<int> currentSizes = splitter->sizes();
            if (currentSizes.at(0) != otherSplitter->sizes().at(0))
                otherSplitter->setSizes(currentSizes);
        }
    }
}
