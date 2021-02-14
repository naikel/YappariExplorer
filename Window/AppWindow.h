#ifndef APPWINDOW_H
#define APPWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QMutex>
#include <QAbstractItemView>
#include "Shell/contextmenu.h"
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
#define getPhysicalPixels(virtualPixels) (virtualPixels * (logicalDpiX() / 96.0))
#endif

// We can't include CustomExplorer.h here so we just declare it
class CustomExplorer;

class AppWindow : public MAINWINDOW
{
    Q_OBJECT
public:
    explicit AppWindow(QWidget *parent = nullptr);
    ~AppWindow();

    QWidget *contentWidget();

    static WId getWinId();
    static quint32 registerWatcher(DirectoryWatcher *watcher);

signals:

public slots:
    void showContextMenu(const QPoint &pos, const QList<FileSystemItem *> fileSystemItems,
                         const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view);
    void defaultAction(const FileSystemItem *fileSystemItem);
    void updateTitle(FileSystemItem *item);

protected:
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

private:
    int  nExplorers   { 2 };
    bool verticalMode { true };

    QList<CustomExplorer *> explorers;
    QList<QSplitter *>      splitters;

    ContextMenu *contextMenu {};

    static WId windowId;
    static quint32 nextId;
    static QMutex regMutex;
    static QMap<quint32, DirectoryWatcher *> watchers;

    void setupGui();
    void saveSettings();

private slots:
    void resizeOtherSplitter(QSplitter *splitter);

};

#endif // APPWINDOW_H
