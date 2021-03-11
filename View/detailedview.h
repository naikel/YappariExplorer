#ifndef DETAILEDVIEW_H
#define DETAILEDVIEW_H

#include "Util/FileSystemHistory.h"
#include "Model/filesystemmodel.h"
#include "Shell/contextmenu.h"
#include "Base/basetreeview.h"

class DetailedView : public BaseTreeView
{
    Q_OBJECT

public:
    DetailedView(QWidget *parent = nullptr);

    bool setRoot(QString root) override;
    void setModel(QAbstractItemModel *model) override;
    FileSystemHistory *getHistory();

public slots:
    void setRootIndex(const QModelIndex &index) override;


signals:

    // TODO: DELETE
    void rootChanged(QString path);

    // New ones
    void rootIndexChanged(const QModelIndex &index);


protected:
    void selectEvent() override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void backEvent() override;
    void forwardEvent() override;

    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) override;
    void setSelectionFromViewportRect(const QRect &rect, QItemSelection &currentSelection, QItemSelectionModel::SelectionFlags command);

private:
    QRubberBand *rubberBand                         {};
    QPoint origin                                   {};
    QItemSelection currentSelection                 {};
    QItemSelectionModel::SelectionFlags command     {};
    FileSystemHistory *history                      {};

};

#endif // DETAILEDVIEW_H
