#ifndef SORTMODEL_H
#define SORTMODEL_H

#include <unicode/coll.h>

#include <QSortFilterProxyModel>

class SortModel : public QSortFilterProxyModel
{
public:
    SortModel(QObject *parent = nullptr);
    ~SortModel();

    bool willRecycle(const QModelIndex &index);
    void removeIndexes(QModelIndexList indexList, bool permanent);
    Qt::DropAction defaultDropActionForIndex(QModelIndex index, const QMimeData *data, Qt::DropActions possibleActions);

protected:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

private:
    icu::Collator *coll {};
};

#endif // SORTMODEL_H
