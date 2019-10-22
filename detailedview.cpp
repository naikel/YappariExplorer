#include <QHeaderView>

#include "detailedview.h"

DetailedView::DetailedView(QWidget *parent) : QTreeView(parent)
{
    setRootIsDecorated(false);
    setFrameShape(QFrame::NoFrame);
    setSortingEnabled(true);
    this->header()->setSortIndicator(0, Qt::AscendingOrder);
}
