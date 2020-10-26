#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

#include <QAbstractItemView>

#include "Model/filesystemmodel.h"

class ContextMenu : public QObject
{
    Q_OBJECT

public:
    enum ContextViewAspect {
        Selection,
        Background
    };

    explicit ContextMenu(QObject *parent = nullptr);

    virtual void show(const WId wId, const QPoint &pos, const QList<FileSystemItem *> fileSystemItems,
                      const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view);
    virtual void defaultAction(const WId wId, const FileSystemItem *fileSystemItem);
    virtual bool handleNativeEvent(const QByteArray &eventType, void *message, long *result);

signals:

public slots:
};

#endif // CONTEXTMENU_H
