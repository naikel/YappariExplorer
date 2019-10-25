#include <QFileIconProvider>
#include <QtConcurrent/QtConcurrentRun>
#include <QDebug>
#include <QDir>

#include "filesystemmodel.h"

#ifdef Q_OS_WIN
#include "winfileinforetriever.h"
#else
#include "unixfileinforetriever.h"
#endif

FileSystemModel::FileSystemModel(FileInfoRetriever::Scope scope, QObject *parent) : QAbstractItemModel(parent)
{
    // This is mandatory if you want the dataChanged signal to work when emitting it in a different thread
    // It's because of the roles argument of the signal. The fingerprint of the method is as follows:
    // dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
    qRegisterMetaType<QVector<int> >("QVector<int>");

#ifdef Q_OS_WIN
    fileInfoRetriever = new WinFileInfoRetriever();
#else
    fileInfoRetriever = new UnixFileInfoRetriever();
#endif

    // Get the default icons
    QFileIconProvider iconProvider;
    driveIcon = iconProvider.icon(QFileIconProvider::Drive);
    folderIcon = iconProvider.icon(QFileIconProvider::Folder);
    fileIcon = iconProvider.icon(QFileIconProvider::File);

    // Start the thread of the file retriever
    fileInfoRetriever->setScope(scope);

    connect(fileInfoRetriever, &FileInfoRetriever::parentUpdated, this, &FileSystemModel::parentUpdated);
}

FileSystemModel::~FileSystemModel()
{
    // Abort all the pending threads
    pool.clear();
    pool.waitForDone();

    if (root != nullptr)
        delete root;

    if (fileInfoRetriever != nullptr)
        delete fileInfoRetriever;
}

QModelIndex FileSystemModel::index(int row, int column, const QModelIndex &parent) const
{
    // qDebug() << "index " << row << " " << column;
    if (row < 0 || column < 0 || row >= rowCount(parent) || column >= columnCount(parent))
        return QModelIndex();

    // If parent is not valid it means TreeView is asking for the root
    // or it is a ListView
    if (!parent.isValid() && !root->getDisplayName().isEmpty())
        return createIndex(row, column, (fileInfoRetriever->getScope() == FileInfoRetriever::List) ? root->getChildAt(row) : this->root);

    if (parent.isValid() && parent.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(parent.internalPointer());

        return createIndex(row, column, fileSystemItem->getChildAt(row));
    }

    return QModelIndex();
}

QModelIndex FileSystemModel::parent(const QModelIndex &index) const
{
    // ListView: The parent of a top level index should be invalid
    if (fileInfoRetriever->getScope() == FileInfoRetriever::List)
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

    if (fileInfoRetriever->getScope() == FileInfoRetriever::List)
        return root->childrenCount();

    if (parent.isValid() && parent.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(parent.internalPointer());
        return fileSystemItem->childrenCount();
    }

    return 1;
}

int FileSystemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
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

                    // Get default icon first
                    if (fileSystemItem->isDrive())
                        icon = driveIcon;
                    else if (fileSystemItem->isFolder())
                        icon = folderIcon;
                    else
                        icon = fileIcon;

                    fileSystemItem->setIcon(icon);

                    QtConcurrent::run(const_cast<QThreadPool *>(&pool), const_cast<FileSystemModel *>(this), &FileSystemModel::getIcon, index);
                }
                return icon;
        }
    }
    return QVariant();
}

QVariant FileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation)

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

            fetchingMore.store(true);
            emit fetchStarted();

            // It seems this works when you don't know beforehand how many rows you're going to insert
            beginInsertRows(parent, 0, 0);

            fileInfoRetriever->getChildren(fileSystemItem);
        }
    }
}

void FileSystemModel::sort(int column, Qt::SortOrder order)
{
    Q_UNUSED(column)
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

    // Abort all the pending threads
    pool.clear();
    pool.waitForDone();

    FileSystemItem *deleteLater = root;
    root = fileInfoRetriever->getRoot(path);

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

void FileSystemModel::getIcon(const QModelIndex &index)
{
    if (index.isValid()  && index.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
        fileSystemItem->setIcon(fileInfoRetriever->getIcon(fileSystemItem));

        QVector<int> roles;
        roles.append(Qt::DecorationRole);
        emit dataChanged(index, index, roles);
    }
}
