#include <QStyleOption>
#include <QPainter>
#include <QDebug>
#include <QRect>

#include "CustomTabBar.h"
#include "CustomTabBarStyle.h"

CustomTabBarStyle::CustomTabBarStyle()
{

}

void CustomTabBarStyle::drawControl(QStyle::ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    QStyleOption *opt = const_cast<QStyleOption *>(option);

    if (element == CE_TabBarTabLabel) {
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {

            QWidget *w = const_cast<QWidget *>(widget);
            CustomTabBar *tabBar = static_cast<CustomTabBar *>(w);

            // This is a hack.
            // When dragging a tab QTabBar creates a new widget from a private class called QMovableTabWidget.
            // This widget will have a rect equal to tabRect(pressedIndex)
            // Somehow this rect is too big.
            // So we change the size of this rect with the size it should have by calling tabSizeHint()
            if (tabBar->isDragging() && option->state & QStyle::State_Sunken)
                opt->rect.setSize(tabBar->hintSizeOfTab(tabBar->getDragIndex()));

            // Get rid of the padding (couldn't find another way to do this without losing the platform style)
            QFontMetrics fm = option->fontMetrics;
            int space = fm.size(0, " ").width();

#ifdef Q_OS_WIN
            // If the tab text is empty it means this is the "add new tab" tab, we want that tab even smaller
            int padding = tab->text.isEmpty() ? (space*3)+2 : space*2;
#else
            int padding = tab->text.isEmpty() ? space : 0;
#endif

            // We get rid of the padding by expanding the rectangle
            opt->rect.setX(opt->rect.x() - padding);
            opt->rect.setWidth(opt->rect.width() + padding);
        }
    }

    // Set the same font we set for the CustomTabBar
    painter->setFont(widget->font());
    QProxyStyle::drawControl(element, opt, painter, widget);
}
