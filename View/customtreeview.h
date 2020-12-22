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
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool viewportEvent(QEvent *event) override;
    void selectEvent() override;
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

private:
    QModelIndex hoverIndex;

    void viewFocusChanged();
    void setDropAction(QDropEvent *event);
};

#endif // CUSTOMTREEVIEW_H
