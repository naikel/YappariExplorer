#ifndef DETAILEDVIEW_H
#define DETAILEDVIEW_H

#include <QTreeView>

class DetailedView : public QTreeView
{
    Q_OBJECT

public:
    DetailedView(QWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;

    void setRoot(QString root);

public slots:

    void setNormalCursor();
    void setBusyCursor();
};

#endif // DETAILEDVIEW_H
