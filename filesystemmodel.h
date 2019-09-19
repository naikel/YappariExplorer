#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QAbstractItemModel>

#include "filesystemitem.h"
#include "winfileinforetriever.h"

class FileSystemModel : public QAbstractItemModel
{
public:
    FileSystemModel(QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

private:
    FileSystemItem *root;
    WinFileInfoRetriever fileInfoRetriever;
};

#endif // FILESYSTEMMODEL_H
