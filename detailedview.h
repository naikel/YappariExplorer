#ifndef DETAILEDVIEW_H
#define DETAILEDVIEW_H

#include <QTreeView>

class DetailedView : public QTreeView
{
    Q_OBJECT

public:
    DetailedView(QWidget *parent = nullptr);
};

#endif // DETAILEDVIEW_H
