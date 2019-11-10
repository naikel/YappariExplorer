#include <QStyleOption>
#include <QDebug>
#include <QRect>

#include "customtabbarstyle.h"

CustomTabBarStyle::CustomTabBarStyle()
{

}

void CustomTabBarStyle::drawControl(QStyle::ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    QStyleOption *opt = const_cast<QStyleOption *>(option);
    if (element == CE_TabBarTabLabel) {
        if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {

            // Get rid of the padding (couldn't find another way to do this without losing the platform style)
            QFontMetrics fm = option->fontMetrics;
            int space = fm.size(0, " ").width();

            // If the tab text is empty it means this is the "add new tab" tab, we want that tab even smaller
            int padding = tab->text.isEmpty() ? (space*3)+2 : space*2;

            // We get rid of the padding by expanding the rectangle
            opt->rect.setX(opt->rect.x() - padding);
            opt->rect.setWidth(opt->rect.width() + padding);
        }
    }
    QProxyStyle::drawControl(element, opt, painter, widget);
}
