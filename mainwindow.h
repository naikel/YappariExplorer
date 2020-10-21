#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QItemSelection>
#include <QMainWindow>
#include <QModelIndex>

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

public slots:
    void showContextMenu(const QPoint &pos, const QList<FileSystemItem *> fileSystemItems, const ContextMenu::ContextViewAspect viewAspect);
    void defaultAction(const FileSystemItem *fileSystemItem);

    void resizeBottomTreeView();
    void resizeTopTreeView();

    void updateTitle(FileSystemItem *item);


protected:
    virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

private:
    Ui::MainWindow *ui;

    ContextMenu *contextMenu {};
};
#endif // MAINWINDOW_H
