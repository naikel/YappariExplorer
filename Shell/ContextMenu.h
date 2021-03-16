#ifndef CONTEXTMENU_H
#define CONTEXTMENU_H

#include <QAbstractItemView>

#include "Model/FileSystemModel.h"

class ContextMenu : public QObject
{
    Q_OBJECT

public:
    enum ContextViewAspect {
        Selection,
        Background
    };

    explicit ContextMenu(QObject *parent = nullptr);

    virtual void show(const WId wId, const QPoint &pos, const QModelIndexList &indexList,
                      const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view) = 0;
    virtual void defaultActionForIndex(const WId wId, const QModelIndex &index) = 0;
    virtual bool handleNativeEvent(const QByteArray &eventType, void *message, long *result) = 0;

signals:

public slots:
};

#endif // CONTEXTMENU_H
