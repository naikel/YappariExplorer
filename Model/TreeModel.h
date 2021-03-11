#ifndef TREEMODEL_H
#define TREEMODEL_H

#include "Model/SortModel.h"

class TreeModel : public SortModel
{
public:
    TreeModel(QObject *parent = nullptr);

    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    virtual bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override;
};

#endif // TREEMODEL_H
