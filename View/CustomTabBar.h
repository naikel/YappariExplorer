#ifndef CUSTOMTABBAR_H
#define CUSTOMTABBAR_H

#include <QTabBar>

class CustomTabBar : public QTabBar
{
    Q_OBJECT

public:
    CustomTabBar(QWidget *parent = nullptr);

    bool isDragging() const;

    int getDragIndex() const;
    QSize hintSizeOfTab(int index);

signals:
    void newTabClicked();

protected:
    QSize minimumTabSizeHint(int index) const override;
    QSize tabSizeHint(int index) const override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QPoint dragStartPosition;
    bool dragInProgress {};
    int dragIndex;

    int getPaddingForIndex(int index) const;
    void tabClicked(int index);
};

#endif // CUSTOMTABBAR_H
