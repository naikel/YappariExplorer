#ifndef CUSTOMSTYLE_H
#define CUSTOMSTYLE_H

#include <QProxyStyle>

class CustomTabBarStyle : public QProxyStyle
{
public:
    CustomTabBarStyle();

    void drawControl(QStyle::ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;
};

#endif // CUSTOMSTYLE_H
