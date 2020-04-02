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
    void setViewIndex(const QModelIndex &index);
    void changeRootPath(const QString path);
    void doubleClicked(const QModelIndex &index);
    void updateTab();
    void newTabClicked();
    void closeTab(int index);

signals:
    void rootChanged(const QString path);
    void newTabRequested();
    void contextMenuRequestedForItems(const QPoint &pos, const QList<FileSystemItem *> fileSystemItems, const ContextMenu::ContextViewAspect viewAspect);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void emitContextMenu(const QPoint &pos, const QList<FileSystemItem *> fileSystemItems, const ContextMenu::ContextViewAspect viewAspect);
};

#endif // CUSTOMTABWIDGET_H
