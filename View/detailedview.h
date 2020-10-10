#ifndef DETAILEDVIEW_H
#define DETAILEDVIEW_H

#include "Model/filesystemmodel.h"
#include "Shell/contextmenu.h"
#include "Base/basetreeview.h"

class DetailedView : public BaseTreeView
{
    Q_OBJECT

public:
    DetailedView(QWidget *parent = nullptr);

    void initialize() override;
    bool setRoot(QString root) override;

protected:
    void selectEvent() override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void setSelectionFromViewportRect(const QRect &rect, QItemSelection &currentSelection, QItemSelectionModel::SelectionFlags command);

private:
    QRubberBand *rubberBand                         {};
    QPoint origin                                   {};
    QItemSelection currentSelection                 {};
    QItemSelectionModel::SelectionFlags command     {};

};

#endif // DETAILEDVIEW_H
