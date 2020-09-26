#ifndef BASEDATEITEMDELEGATE_H
#define BASEDATEITEMDELEGATE_H

#include <QStyledItemDelegate>

class BaseItemDelegate : public QStyledItemDelegate
{
public:
    BaseItemDelegate(QObject *parent);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

#endif // BASEDATEITEMDELEGATE_H
