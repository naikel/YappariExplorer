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

    void initialize();
    void setRoot(QString root);
};

#endif // DETAILEDVIEW_H
