#ifndef CUSTOMTREEVIEW_H
#define CUSTOMTREEVIEW_H

#include <QTreeView>

class CustomTreeView : public QTreeView
{
    Q_OBJECT

public:
    CustomTreeView(QWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;

public slots:

    void setNormalCursor();
    void setBusyCursor();
};

#endif // CUSTOMTREEVIEW_H
