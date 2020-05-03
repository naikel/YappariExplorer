#include <QApplication>
#include <QFileIconProvider>
#include <QtConcurrent/QtConcurrentRun>
#include <QMimeData>
#include <QBrush>
#include <QDebug>
#include <QDir>
#include <QUrl>

#include <limits>
#include <cmath>

#include "once.h"
#include "filesystemmodel.h"

#ifdef Q_OS_WIN
#include "Shell/Win/winfileinforetriever.h"
#define PlatformInfoRetriever()     WinFileInfoRetriever()
#else
#include "Shell/Unix/unixfileinforetriever.h"
#define PlatformInfoRetriever()     UnixFileInfoRetriever()
#endif

FileSystemModel::FileSystemModel(FileInfoRetriever::Scope scope, QObject *parent) : QAbstractItemModel(parent)
{
    // This is mandatory if you want the dataChanged signal to work when emitting it in a different thread.
    // It's because of the "roles" argument of the signal. The fingerprint of the method is as follows:
    // dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
    qRegisterMetaType<QVector<int>>("QVector<int>");

    fileInfoRetriever = new PlatformInfoRetriever();

    // Get the default icons
    QFileIconProvider iconProvider;
    driveIcon = iconProvider.icon(QFileIconProvider::Drive);
    folderIcon = iconProvider.icon(QFileIconProvider::Folder);
    fileIcon = iconProvider.icon(QFileIconProvider::File);

    // Start the thread of the file retriever
    fileInfoRetriever->setScope(scope);

    connect(fileInfoRetriever, &FileInfoRetriever::parentUpdated, this, &FileSystemModel::parentUpdated);
    connect(fileInfoRetriever, &FileInfoRetriever::itemUpdated, this, &FileSystemModel::itemUpdated);
    connect(fileInfoRetriever, &FileInfoRetriever::extendedInfoUpdated, this, &FileSystemModel::extendedInfoUpdated);

}

FileSystemModel::~FileSystemModel()
{
    // Abort all the pending threads
    pool.clear();
    qDebug() << "FileSystemModel::~FileSystemModel: Waiting for all threads to finish";
    pool.waitForDone();
    qDebug() << "FileSystemModel::~FileSystemModel: All threads finished";

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
    if (fileInfoRetriever->getScope() == FileInfoRetriever::List)
        return Columns::MaxColumns;

    return 1;
}

QVariant FileSystemModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()  && index.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
        switch (role) {
            case Qt::EditRole:
            case Qt::DisplayRole:
                switch (index.column()) {
                    case Columns::Name:
                        return QVariant(fileSystemItem->getDisplayName());
                    case Columns::Extension:
                        if (fileSystemItem->isDrive() || fileSystemItem->isFolder())
                            return QVariant();
                        return fileSystemItem->getExtension();
                    case Columns::Size:
                        if (fileSystemItem->isDrive() || fileSystemItem->isFolder())
                            return QVariant();
                        return QVariant(humanReadableSize(fileSystemItem->getSize()));
                    case Columns::Type:
                        return QVariant(fileSystemItem->getType());
                    case Columns::LastChangeTime:
                        return QVariant(fileSystemItem->getLastChangeTime().toMSecsSinceEpoch());
                    default:
                        return QVariant();
                }
            case Qt::DecorationRole:
                if (index.column() == Columns::Name) {
                    // Fetch the real icon the second time it is requested
                    if (fileSystemItem->hasFakeIcon()) {
                        QtConcurrent::run(const_cast<QThreadPool *>(&pool), const_cast<FileSystemModel *>(this), &FileSystemModel::getIcon, index);
                        fileSystemItem->setFakeIcon(false);
                    }

                    QIcon icon = fileSystemItem->getIcon();
                    if (icon.isNull()) {
                        // This is the first time the view requests the icon
                        // Set a fake icon (a default one)
                        if (fileSystemItem->isDrive())
                            icon = driveIcon;
                        else if (fileSystemItem->isFolder())
                            icon = folderIcon;
                        else
                            icon = fileIcon;

                        fileSystemItem->setIcon(icon);

                        // This icon is fake and needs a real one the next time it is requested
                        fileSystemItem->setFakeIcon(true);
                    }
                    return icon;
                }
                break;
            case Qt::TextAlignmentRole:
                switch (index.column()) {
                    case Columns::Size:
                    case Columns::LastChangeTime:
                        return QVariant(Qt::AlignRight | Qt::AlignVCenter);
                }
                break;
            case Qt::ForegroundRole:
                if (fileInfoRetriever->getScope() == FileInfoRetriever::List && fileSystemItem->isFolder()) {
                    return QVariant(QBrush(Qt::darkBlue));
                }
        }
    }
    return QVariant();
}

QVariant FileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation)

    switch (role) {
        case Qt::DisplayRole:
            switch (section) {
                case Columns::Name:
                    return QVariant(tr("Name"));
                case Columns::Extension:
                    return QVariant(tr("Ext"));
                case Columns::Size:
                    return QVariant(tr("Size"));
                case Columns::Type:
                    return QVariant(tr("Type"));
                case Columns::LastChangeTime:
                    return QVariant(tr("Date Modified"));
            }
            break;
        case Qt::TextAlignmentRole:
            switch (section) {
                case Columns::Size:
                case Columns::LastChangeTime:
                    return QVariant(Qt::AlignRight | Qt::AlignVCenter);
            }
            break;

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
    qDebug() << "FileSystemModel::sort requested";
    currentSortOrder = order;
    currentSortColumn = column;
    emit layoutAboutToBeChanged();
    root->sortChildren(column, order);
    emit layoutChanged();
}

/*!
 * \brief Returns the item flags for the given index.
 * \param index the QModelIndex index.
 * \return A Qt::ItemFlags object with the flags for the given index.
 *
 * This function will return only Qt::ItemIsEnabled if the index is not a filename, that is,
 * if it's not the first column.  This way the user can only select items in column 0.
 *
 * If the index is in column 0, it is indeed a filename and it can be editable and dragable.
 * Also if the index is a folder (or a drive in Windows) it can also support drops.
 */
Qt::ItemFlags FileSystemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags itemFlags = QAbstractItemModel::flags(index);

    if (index.isValid()) {

        // We don't want items to be selectable if they are not in the first column
        if (index.column() > 0)
            return Qt::ItemIsEnabled;

        itemFlags |= Qt::ItemIsEditable | Qt::ItemIsDragEnabled;

        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
        if (fileSystemItem->isFolder() || fileSystemItem->isDrive())
            itemFlags |= Qt::ItemIsDropEnabled;

    } else
        itemFlags |= Qt::ItemIsDropEnabled;

    return itemFlags;
}
/*!
 * \brief Returns the drop actions supported by this model
 * \return a Qt::DropActions object with the drop actions supported
 *
 * This function returns Qt::CopyAction | Qt::MoveAction | Qt::LinkAction.
 *
 * There's no need to implement supportedDragActions() since its default implementation calls
 * this function.
 */
Qt::DropActions FileSystemModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction | Qt::LinkAction;
}


QMimeData *FileSystemModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *data = new QMimeData();
    QList<QUrl> urls;
    for (QModelIndex index : indexes) {
        if (index.column() == 0 && index.internalPointer() != nullptr) {
            FileSystemItem *fileSystemItem = static_cast<FileSystemItem *>(index.internalPointer());
            urls.append(QUrl::fromLocalFile(fileSystemItem->getPath()));
        }
    }
    data->setUrls(urls);
    return data;
}

QStringList FileSystemModel::mimeTypes() const
{
    return QStringList(QLatin1String("text/uri-list"));
}


QModelIndex FileSystemModel::relativeIndex(QString path, QModelIndex parent)
{
    qDebug() << "FileSystemModel::relativeIndex" << path;
    if (parent.isValid() && parent.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem *>(parent.internalPointer());
        FileSystemItem *child = fileSystemItem->getChild(path);

        return createIndex(fileSystemItem->childRow(child), 0, child);

    }
    return QModelIndex();
}

void FileSystemModel::setRoot(const QString path)
{
    qDebug() << "FileSystemModel::setRoot" << fileInfoRetriever->getScope() << path;
    settingRoot.store(true);
    emit fetchStarted();
    beginResetModel();

    // Abort all the pending threads
    pool.clear();
    qDebug() << "FileSystemModel::setRoot" << fileInfoRetriever->getScope() << "Waiting for all threads to finish";
    pool.waitForDone();
    qDebug() << "FileSystemModel::setRoot" << fileInfoRetriever->getScope() << "All threads finished";

    FileSystemItem *deleteLater = root;
    root = fileInfoRetriever->getRoot(path);

    // Process all dataChanged events that could still modify the old root structure
    QApplication::processEvents();

    // Now we can delete the old root structure safely
    if (deleteLater != nullptr) {
        qDebug() << "FileSystemModel::setRoot" << fileInfoRetriever->getScope() << "freeing older root";
        delete deleteLater;
    }
}


FileSystemItem *FileSystemModel::getRoot()
{
    return root;
}

QChar FileSystemModel::separator()
{
    return QDir::separator();
}

void FileSystemModel::parentUpdated(FileSystemItem *parent)
{
    qDebug() << "FileSystemModel::parentUpdated" << parent->childrenCount() << " children";

    parent->sortChildren(currentSortColumn, currentSortOrder);

    if (fetchingMore.load()) {
        qDebug() << "FileSystemModel::parentUpdated endInsertRows";
        endInsertRows();
        fetchingMore.store(false);
    }

    if (settingRoot.load()) {
        qDebug() << "FileSystemModel::parentUpdated endResetModel";
        endResetModel();
        settingRoot.store(false);
    }

    emit fetchFinished();
}

void FileSystemModel::itemUpdated(FileSystemItem *item)
{
    // Send the dataChanged signal only if this item has been already updated by the icon
    // Otherwise it's not necessary since the getIcon will send a dataChanged signal for it
    if (item != nullptr && !item->getIcon().isNull() && !item->hasFakeIcon()) {

        FileSystemItem *parent = item->getParent();

        // This could be an old signal (different path)
        if (item->getParent()->getPath() == root->getPath()) {
            int row = parent->childRow(item);
            qDebug() << "FileSystemModel::itemUpdated" << item->getPath() << "row" << row;
            QModelIndex fromIndex = createIndex(row, Columns::Size, item);
            QModelIndex lastIndex = createIndex(row, Columns::MaxColumns, item);
            QVector<int> roles;
            roles.append(Qt::DisplayRole);
            emit dataChanged(fromIndex, lastIndex, roles);
        }
    }
}

void FileSystemModel::extendedInfoUpdated(FileSystemItem *parent)
{
    Q_UNUSED(parent)

    // If the model is currently sorted by one of the extended info columns, it obviously needs resorting
    if (currentSortColumn >= Columns::Type)
        sort(currentSortColumn, currentSortOrder);
}

void FileSystemModel::getIcon(const QModelIndex &index)
{
    if (index.isValid() && index.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
        QIcon icon = fileInfoRetriever->getIcon(fileSystemItem);
        if (!icon.isNull()) {
            fileSystemItem->setIcon(icon);
            emit dataChanged(index, index);
        }
    }
}

QString FileSystemModel::humanReadableSize(quint64 size) const
{
    if (size == std::numeric_limits<quint64>::max())
        return QString();

    int i;
    double sizeDouble = size;
    for (i = 0; sizeDouble >= 1024.0 ; i++)
        sizeDouble /= 1024.0;

    // Remove decimals for exact values (like 24.00) and for sizeDouble >= 100
    QString strSize;
    if ((sizeDouble >= 100.0) || ((sizeDouble - trunc(sizeDouble)) < 0.001))
        strSize = QString::number(qRound(sizeDouble));
    else
        strSize = QString::number(sizeDouble, 'f', 2);

    return QString("%1 %2%3 ").arg(strSize).arg(sizeUnits.at(i)).arg((i == 0 && size != 1) ? "s" : QString());
}

