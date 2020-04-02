#ifndef DETAILEDVIEW_H
#define DETAILEDVIEW_H

#include <QTreeView>

#include "Model/filesystemmodel.h"
#include "Shell/contextmenu.h"

class DetailedView : public QTreeView
{
    Q_OBJECT

public:
    DetailedView(QWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;

    void setRoot(QString root);

signals:
    void contextMenuRequestedForItems(const QPoint &pos, const QList<FileSystemItem *> fileSystemItems, const ContextMenu::ContextViewAspect viewAspect);

public slots:

    void setNormalCursor();
    void setBusyCursor();

    void contextMenuRequested(const QPoint &pos);
};

#endif // DETAILEDVIEW_H
