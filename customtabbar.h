#ifndef CUSTOMTABBAR_H
#define CUSTOMTABBAR_H

#include <QTabBar>

class CustomTabBar : public QTabBar
{
public:
    CustomTabBar(QWidget *parent = nullptr);

protected:
    QSize minimumTabSizeHint(int index) const override;
    QSize tabSizeHint(int index) const override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    int getPaddingForIndex(int index) const;
};

#endif // CUSTOMTABBAR_H
