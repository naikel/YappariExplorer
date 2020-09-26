#include <QApplication>
#include <QPainter>
#include <QDebug>


#include "baseitemdelegate.h"
#include "basetreeview.h"

BaseItemDelegate::BaseItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

void BaseItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());

    QStyleOptionViewItem origOpt = option;
    initStyleOption(&origOpt, index);

    QStyleOptionViewItem opt = origOpt;

    // Turn off that really ugly gray dotted border when an item has focus
    if (opt.state & QStyle::State_HasFocus)
        opt.state &= ~QStyle::State_HasFocus;

    QStyledItemDelegate::paint(painter, opt, index);

    // Draw our custom focus rectangle
    if (origOpt.state & QStyle::State_HasFocus && !(origOpt.state & (QStyle::State_Selected|QStyle::State_MouseOver))) {
        BaseTreeView *treeView = static_cast<BaseTreeView *>(parent());
        QRect rect = treeView->visualRect(index);
        rect = rect.adjusted(0, 0, -1, -1);

        QPen pen;
        pen.setColor(opt.palette.color(QPalette::Highlight).lighter());
        painter->setPen(pen);
        painter->drawRect(rect);
    }
}

void BaseItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);

    // Turn off that really ugly gray dotted border when an item has focus
    //if (option->state & QStyle::State_HasFocus)
    //    option->state &= ~QStyle::State_HasFocus;

    // No mouse over for columns > 0
    if (index.column() > 0 && (option->state & QStyle::State_MouseOver))
        option->state &= ~QStyle::State_MouseOver;

    // No mouse over when dragging over items that are not droppable
    // Funny that widget is an undocumented member of the class QStyleOptionViewTime
    const BaseTreeView *treeView = static_cast<const BaseTreeView *>(option->widget);
    if (treeView->isDragging() && index.column() == 0 && !(index.flags() & Qt::ItemIsDropEnabled))
        option->state &= ~QStyle::State_MouseOver;
}

