#ifndef DATEITEMDELEGATE_H
#define DATEITEMDELEGATE_H

#include <QStyledItemDelegate>

class DateItemDelegate : public QStyledItemDelegate
{
public:
    DateItemDelegate(QObject *parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

#endif // DATEITEMDELEGATE_H
