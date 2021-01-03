#ifndef CUSTOMTABWIDGET_H
#define CUSTOMTABWIDGET_H

#include <QTabWidget>

#include "detailedview.h"
#include "Model/filesystemmodel.h"

class CustomTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    CustomTabWidget(QWidget *parent = nullptr);
    void addNewTab(const QString path);

public slots:
    bool setViewIndex(const QModelIndex &index);
    void changeRootPath(const QString path);
    void doubleClicked(const QModelIndex &index);
    void updateTab();
    void nameTab(int index, FileSystemItem *item);
    void tabFailed(qint32 err, QString errMessage);
    void newTabClicked();
    void closeTab(int index);
    void displayNameChanged(QString oldPath, FileSystemItem *item);
    void tabFocus(FileSystemItem *item);

signals:
    void rootRelativeChange(const QString path);
    void rootAbsoluteChange(const QString path);
    void rootChangeFailed(const QString path);
    void newTabRequested();
    void contextMenuRequestedForItems(const QPoint &pos, const QList<FileSystemItem *> fileSystemItems,
                                      const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view);
    void defaultActionRequestedForItem(FileSystemItem *fileSystemItem);
    void folderFocus(FileSystemItem *fileSystemItem);
    void viewModelChanged(FileSystemModel *model);


protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void emitContextMenu(const QPoint &pos, const QList<FileSystemItem *> fileSystemItems,
                         const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view);
    void rootChanged(QString path);
};

#endif // CUSTOMTABWIDGET_H
