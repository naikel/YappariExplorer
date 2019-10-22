#ifndef CUSTOMTREEVIEW_H
#define CUSTOMTREEVIEW_H

#include <QTreeView>

class CustomTreeView : public QTreeView
{
    Q_OBJECT

public:
    CustomTreeView(QWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;
    void setRootIndex(const QModelIndex &index) override;
    QModelIndex selectedItem();

public slots:

    void initialize();
    void setNormalCursor();
    void setBusyCursor();

private:
    bool initialized {false};

};

#endif // CUSTOMTREEVIEW_H
