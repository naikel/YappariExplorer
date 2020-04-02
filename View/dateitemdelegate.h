#ifndef DATEITEMDELEGATE_H
#define DATEITEMDELEGATE_H

#include "View/Base/baseitemdelegate.h"

class DateItemDelegate : public BaseItemDelegate
{
public:
    DateItemDelegate(QObject *parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // DATEITEMDELEGATE_H
