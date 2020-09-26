#include <QApplication>
#include <QDateTime>
#include <QPainter>
#include <QDebug>
#include <algorithm>

#include "View/dateitemdelegate.h"
#include "Shell/filesystemitem.h"

#define QDATETIME_EMPTY     "18000000"

DateItemDelegate::DateItemDelegate(QObject *parent) : BaseItemDelegate(parent)
{

}

void DateItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const QWidget *widget = opt.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();

    if (opt.text == QDATETIME_EMPTY) {
        opt.text = QString();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
        return;
    }

    QFontMetrics fm = QFontMetrics(opt.font);
    QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(opt.text.toLongLong()).toLocalTime();

    // Get maximum time and date sizes
    QString sampleTimeStr = QTime(10, 00).toString(Qt::SystemLocaleShortDate);
    QString sampleDateStr = QDate(2000, 10, 30).toString(Qt::SystemLocaleShortDate);
    int timeSize = fm.size(0, "  " + sampleTimeStr).width();
    int dateSize = fm.size(0, "  " + sampleDateStr).width();

    QRect originalRect = opt.rect;
    opt.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;

    // Find out the maximum width of the date portion of the control
    int dateWidth = std::max(opt.rect.width() - timeSize, dateSize);
    dateWidth = std::min(opt.rect.width(), dateWidth);
    opt.rect.setWidth(dateWidth);

    opt.text = dateTime.date().toString(Qt::SystemLocaleShortDate);
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

    opt.rect = originalRect;

    // Only try to draw the time if there's enough space for it
    int timeWidth = std::min(timeSize, opt.rect.width() - dateSize);
    if (timeWidth > 0) {
        opt.rect.setX(opt.rect.x() + opt.rect.width() - timeWidth);
        opt.rect.setWidth(timeWidth);
        opt.text = dateTime.time().toString(Qt::SystemLocaleShortDate);
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
    }
}
