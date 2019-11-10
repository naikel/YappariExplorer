#include <QStyleOptionTab>
#include <QDebug>
#include <QLabel>
#include <QIcon>

#include "customtabbar.h"
#include "customtabbarstyle.h"

CustomTabBar::CustomTabBar(QWidget *parent) : QTabBar(parent)
{
    CustomTabBarStyle *style = new CustomTabBarStyle();
    setStyle(style);
    setExpanding(false);
    setDrawBase(false);
    addTab(QIcon(":/icons/addtab.png"), QString());
}

int CustomTabBar::getPaddingForIndex(int index) const
{
    QStyleOptionTab opt;
    initStyleOption(&opt, index);
    QFontMetrics fm = opt.fontMetrics;
    int space = fm.size(0, " ").width();

    // If the tab text is empty it means this is the "add new tab" tab, we want that tab even smaller
    return tabText(index).isEmpty() ? space*7 : space*3;
}

QSize CustomTabBar::minimumTabSizeHint(int index) const
{
    QSize size = QTabBar::minimumTabSizeHint(index);
    return QSize(size.width()-getPaddingForIndex(index), size.height());
}

QSize CustomTabBar::tabSizeHint(int index) const
{
    QSize size = QTabBar::tabSizeHint(index);
    return QSize(size.width()-getPaddingForIndex(index), size.height());
}

void CustomTabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    qDebug() << "CustomTabBar::mouseDoubleClickEvent close tab";

    QTabBar::mouseDoubleClickEvent(event);
}


