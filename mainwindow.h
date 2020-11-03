#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAbstractItemView>
#include <QItemSelection>
#include <QMainWindow>
#include <QModelIndex>
#include <QMutex>

#include "Model/filesystemmodel.h"
#include "Shell/contextmenu.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    static WId getWinId();
    static quint32 registerWatcher(DirectoryWatcher *watcher);

public slots:
    void showContextMenu(const QPoint &pos, const QList<FileSystemItem *> fileSystemItems,
                         const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view);
    void defaultAction(const FileSystemItem *fileSystemItem);

    void resizeBottomTreeView();
    void resizeTopTreeView();

    void updateTitle(FileSystemItem *item);


protected:
    virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

private:
    Ui::MainWindow *ui;

    ContextMenu *contextMenu {};

    static WId windowId;
    static quint32 nextId;
    static QMutex regMutex;
    static QMap<quint32, DirectoryWatcher *> watchers;
};
#endif // MAINWINDOW_H
