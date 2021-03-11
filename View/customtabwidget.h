#ifndef CUSTOMTABWIDGET_H
#define CUSTOMTABWIDGET_H

#include <QTabWidget>

#include "detailedview.h"
#include "Model/filesystemmodel.h"

class CustomTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    CustomTabWidget(int pane, QWidget *parent = nullptr);
    void initialize(QAbstractItemModel *model);
    void addNewTab(const QString path);
    void addNewTab(const QModelIndex& sourceIndex);
    void addNewTab(const QString path, const QString displayName, const QIcon icon);
    void copyCurrentTab();
    void setUpTab(int tab, const QModelIndex& sourceIndex);
    void saveSettings(int pane);

public slots:
    bool setViewRootIndex(const QModelIndex &index);
    bool setViewRootPath(const QString &path);
    void doubleClicked(const QModelIndex &index);
    void updateTab();
    void setTabName(int index);
    void tabFailed(qint32 err, QString errMessage);
    void closeTab(int index);
    void tabFocus(FileSystemItem *item);
    void refreshCurrentTab();

    // New ones
    void viewIndexSelected(const QModelIndex &index);

signals:
    void rootRelativeChange(const QString path);
    void rootAbsoluteChange(const QString path);
    void rootChangeFailed(const QString path);
    void newTabRequested();
    void contextMenuRequestedForItems(const QPoint &pos, const QModelIndexList &indexList,
                                      const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view);
    void defaultActionRequestedForItem(FileSystemItem *fileSystemItem);
    void folderFocus(FileSystemItem *fileSystemItem);
    void viewModelChanged(FileSystemModel *model);

    // New Ones
    void viewIndexChanged(const QModelIndex &index);
    void folderFocusIndex(const QModelIndex &index);
    void defaultActionRequestedForIndex(const QModelIndex &index);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void modelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());
    void emitContextMenu(const QPoint &pos, const QModelIndexList &indexList, const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view);
    void rootChanged(QString path);

private:
    int pane;
    FileSystemModel *model {};

    QString iconToBase64(QIcon icon);
    QIcon base64ToIcon(QString base64);
};

#endif // CUSTOMTABWIDGET_H
