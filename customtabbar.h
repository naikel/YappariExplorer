#ifndef CUSTOMTABBAR_H
#define CUSTOMTABBAR_H

#include <QTabBar>

class CustomTabBar : public QTabBar
{
    Q_OBJECT

public:
    CustomTabBar(QWidget *parent = nullptr);

signals:
    void newTabRequested();

protected:
    QSize minimumTabSizeHint(int index) const override;
    QSize tabSizeHint(int index) const override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    int getPaddingForIndex(int index) const;
    void tabClicked(int index);
};

#endif // CUSTOMTABBAR_H
