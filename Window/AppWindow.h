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

#ifndef APPWINDOW_H
#define APPWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QMutex>
#include <QAbstractItemView>
#include "Shell/ContextMenu.h"
#include "Shell/directorywatcher.h"
#include "Settings/Settings.h"

#ifdef WIN32_FRAMELESS
#include "Win/WinFramelessWindow.h"
#endif

#ifdef WIN32_FRAMELESS
#define MAINWINDOW          WinFramelessWindow
#define CENTRALWIDGET       WinFramelessWindow::contentWidget()
#else
#define MAINWINDOW          QMainWindow
#define CENTRALWIDGET       centralWidget()
#endif

// We can't include CustomExplorer.h here so we just declare it
class CustomExplorer;

/*!
 * \brief AppWindow class.
 *
 * This is the main QMainWindow of the Yappari Explorer application.
 *
 * This FileSystemModel has the following features:
 */
class AppWindow : public MAINWINDOW
{
    Q_OBJECT
public:
    explicit AppWindow(QWidget *parent = nullptr);
    ~AppWindow();

    QWidget *contentWidget();
    int getPhysicalPixels(int virtualPixels) { return (virtualPixels * (logicalDpiX() / 96.0)); };
    quint32 registerWatcher(DirectoryWatcher *watcher);

    static AppWindow *instance();

    WId getWindowId() const;

signals:

public slots:
    void showContextMenu(const QPoint &pos, const QModelIndexList &indexList,
                         const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view);
    void defaultActionForIndex(const QModelIndex &index);
    void updateTitle(const QModelIndex &index);

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    WId windowId      {};
    int  nExplorers   { 2 };
    bool verticalMode { true };

    QList<CustomExplorer *> explorers;
    QList<QSplitter *>      splitters;

    ContextMenu *contextMenu {};
    quint32 nextId {};
    QMutex regMutex;
    QMap<quint32, DirectoryWatcher *> watchers;
    static AppWindow *appWindow;

    QSize prevSize;

    void setupGui();
    void saveSettings();

private slots:
    void resizeOtherSplitter(QSplitter *splitter);

};

#endif // APPWINDOW_H
