#include <QDebug>
#include <QDir>

#include "filesystemmodel.h"

FileSystemModel::FileSystemModel(FileInfoRetriever::Scope scope, QObject *parent) : QAbstractItemModel(parent)
{
    // Start the thread of the file retriever
    fileInfoRetriever.setScope(scope);
    fileInfoRetriever.start();

    connect(&fileInfoRetriever, SIGNAL(parentUpdated(FileSystemItem *)), this, SLOT(parentUpdated(FileSystemItem *)));

}

FileSystemModel::~FileSystemModel()
{
    if (root != nullptr)
        delete root;
}

QModelIndex FileSystemModel::index(int row, int column, const QModelIndex &parent) const
{
    // qDebug() << "index " << row << " " << column;
    if (row < 0 || column < 0 || row >= rowCount(parent) || column >= columnCount(parent))
        return QModelIndex();

    // If parent is not valid it means TreeView is asking for the root
    // or it is a ListView
    if (!parent.isValid() && !root->getDisplayName().isEmpty())
        return createIndex(row, column, (fileInfoRetriever.getScope() == FileInfoRetriever::List) ? root->getChildAt(row) : this->root);

    if (parent.isValid() && parent.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(parent.internalPointer());

        return createIndex(row, column, fileSystemItem->getChildAt(row));
    }

    return QModelIndex();
}

QModelIndex FileSystemModel::parent(const QModelIndex &index) const
{
    // ListView: The parent of a top level index should be invalid
    if (fileInfoRetriever.getScope() == FileInfoRetriever::List)
        return QModelIndex();

    if (index.isValid() && index.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
        FileSystemItem *parent = fileSystemItem->getParent();
        if (parent != nullptr)
            return createIndex(parent->childRow(fileSystemItem), 0, parent);
    }
    return QModelIndex();
}

int FileSystemModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    if (fileInfoRetriever.getScope() == FileInfoRetriever::List)
        return root->childrenCount();

    if (parent.isValid() && parent.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(parent.internalPointer());
        return fileSystemItem->childrenCount();
    }

    return 1;
}

int FileSystemModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant FileSystemModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()  && index.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
        switch (role) {
            case Qt::DisplayRole:
                return QVariant(fileSystemItem->getDisplayName());
            case Qt::DecorationRole:
                QIcon icon = fileSystemItem->getIcon();
                if (icon.isNull()) {
                    // icon hasn't been retrieved yet
                    icon = fileInfoRetriever.getIcon(fileSystemItem);
                    fileSystemItem->setIcon(icon);
                }
                return icon;
        }
    }
    return QVariant();
}

QVariant FileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        switch (section) {
            case 0:
                return QVariant(tr("Name"));
        }
    }
    return QVariant();
}

bool FileSystemModel::hasChildren(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(parent.internalPointer());
        return fileSystemItem->getHasSubFolders();
    }
    return true;
}

bool FileSystemModel::canFetchMore(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(parent.internalPointer());

        if (fileSystemItem->getHasSubFolders() && !fileSystemItem->areAllChildrenFetched())
            return true;
    }

    return false;
}

void FileSystemModel::fetchMore(const QModelIndex &parent)
{
    if (parent.isValid() && parent.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(parent.internalPointer());

        if (fileSystemItem->getHasSubFolders() && !fileSystemItem->areAllChildrenFetched()) {

            // It seems this works when you don't know beforehand how many rows you're going to insert
            fetchingMore.store(true);
            emit fetchStarted();
            beginInsertRows(parent, 0, 0);
            fileInfoRetriever.getChildren(fileSystemItem);
        }
    }
}

void FileSystemModel::sort(int column, Qt::SortOrder order)
{
    qDebug() << "FileSystemModel::Model sort requested";
    emit layoutAboutToBeChanged();
    root->sortChildren(order);
    emit layoutChanged();
}

QModelIndex FileSystemModel::relativeIndex(QString path, QModelIndex parent)
{
    if (parent.isValid() && parent.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(parent.internalPointer());
        FileSystemItem *child = fileSystemItem->getChild(path);

        return createIndex(fileSystemItem->childRow(child), 0, child);

    }
    return QModelIndex();
}

void FileSystemModel::setRoot(const QString path)
{
    qDebug() << "FileSystemModel::setRoot " << path;
    settingRoot.store(true);
    beginResetModel();

    FileSystemItem *deleteLater = root;
    root = fileInfoRetriever.getRoot(path);

    if (deleteLater != nullptr)
        delete deleteLater;
}

FileSystemItem *FileSystemModel::getRoot()
{
    return root;
}

void FileSystemModel::parentUpdated(FileSystemItem *parent)
{
    qDebug() << "FileSystemModel::parentUpdated: " << parent->childrenCount() << " children";

    if (fetchingMore.load()) {
        qDebug() << "FileSystemModel::parentUpdated: endInsertRows";
        endInsertRows();
        fetchingMore.store(false);
    }

    if (settingRoot.load()) {
        qDebug() << "FileSystemModel::parentUpdated: endResetModel";
        endResetModel();
        settingRoot.store(false);
    }

    emit fetchFinished();
}
