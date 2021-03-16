#include <QDebug>

#include "TreeModel.h"

#include "Model/FileSystemModel.h"

TreeModel::TreeModel(QObject *parent) : SortModel(parent)
{
}

bool TreeModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (source_parent.isValid()) {

        if (!(source_parent.data(FileSystemModel::AllChildrenFetchedRole).toBool()))
            return false;

        QModelIndex source = source_parent.model()->index(source_row, 0, source_parent);

        if (source.isValid())
            return source.data(FileSystemModel::DriveRole).toBool() || source.data(FileSystemModel::FolderRole).toBool() || !source.parent().isValid();
    }

    return true;
}

bool TreeModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_parent)
    return source_column == 0;
}

bool TreeModel::hasChildren(const QModelIndex &parent) const
{
    QModelIndex source = mapToSource(parent);
    if (source.isValid())
        return source.data(FileSystemModel::HasSubFoldersRole).toBool();

    return true;
}

bool TreeModel::canFetchMore(const QModelIndex &parent) const
{
    QModelIndex source = mapToSource(parent);
    if (source.isValid())
        return source.data(FileSystemModel::HasSubFoldersRole).toBool() && !(source.data(FileSystemModel::AllChildrenFetchedRole).toBool());

    return false;
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    // Turn off color customization for the tree view
    if (index.isValid() && role == Qt::ForegroundRole)
        return QVariant();

    return SortModel::data(index, role);
}

