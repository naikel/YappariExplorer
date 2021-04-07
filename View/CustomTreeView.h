#ifndef CUSTOMTREEVIEW_H
#define CUSTOMTREEVIEW_H

#include "Base/BaseTreeView.h"

class CustomTreeView : public BaseTreeView
{
    Q_OBJECT

public:
    CustomTreeView(QWidget *parent = nullptr);

    QModelIndex selectedItem();

protected:

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool viewportEvent(QEvent *event) override;
    void selectEvent() override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QModelIndex hoverIndex;
    bool draggingNothing {};

    void setDropAction(QDropEvent *event);
};

#endif // CUSTOMTREEVIEW_H
