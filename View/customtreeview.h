#ifndef CUSTOMTREEVIEW_H
#define CUSTOMTREEVIEW_H

#include "Base/basetreeview.h"

class CustomTreeView : public BaseTreeView
{
    Q_OBJECT

public:
    CustomTreeView(QWidget *parent = nullptr);

    QModelIndex selectedItem();

signals:

    void resized();

public slots:

    void initialize() override;
    void selectIndex(QModelIndex index);

protected:

    void mousePressEvent(QMouseEvent *event) override;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
    void resizeEvent(QResizeEvent *event) override;


    void selectEvent() override;
};

#endif // CUSTOMTREEVIEW_H
