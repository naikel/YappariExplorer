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

};

#endif // DETAILEDVIEW_H
