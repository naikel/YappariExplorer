/*
 * Copyright (C) 2019, 2020 Naikel Aparicio. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ''AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the author and should not be interpreted as representing
 * official policies, either expressed or implied, of the copyright holder.
 */

#include <QApplication>
#include <QPalette>
#include <QFileIconProvider>
#include <QtConcurrent/QtConcurrentRun>
#include <QTimer>
#include <QMimeData>
#include <QBrush>
#include <QDebug>
#include <QDir>
#include <QUrl>

#include <limits>
#include <cmath>

#include "once.h"
#include "FileSystemModel.h"

#ifdef Q_OS_WIN
#   include "Shell/Win/WinFileInfoRetriever.h"
#   include "Shell/Win/WinShellActions.h"
#   include "Shell/Win/WinDirectoryWatcher.h"
#   include "Shell/Win/WinDirectoryWatcherV2.h"
#   define PlatformInfoRetriever()                 WinFileInfoRetriever()
#   define PlatformShellActions()                  WinShellActions()
#   define PlatformDirectoryWatcher(parent)        WinDirectoryWatcherv2(parent)
#else
#   include "Shell/Unix/UnixFileInfoRetriever.h"
#   include "Shell/Unix/unixshellactions.h"
#   define PlatformInfoRetriever()     UnixFileInfoRetriever()
#   define PlatformShellActions()      UnixShellActions()
#endif

/*!
 * \brief The constructor.
 * \param parent The QObject parent.
 *
 * Constructs a new FileSystemModel object.
 *
 * The constructor creates the following objects:
 *
 * - A FileInfoRetriever object to get access to the local filesystem.
 * - A ShellActions object to execute actions like copy, move, delete, rename.
 * - A DirectoryWatcher object that notifies the model when the filesystem changes.
 *
 * This model can be of one of two scopes: a Tree model, or a List model.
 *
 * In the Tree model, the root of the model is the root of the tree.  This means the root is visible in the model
 * and the whole tree starts there.
 *
 * In the List model, the root is a virtual entity and it's not really visible. All indexes have an invalid parent.
 * The reason for this is to use any kind of view with this model like a QTreeView and it will still show the element
 * as a list.
 *
 * \sa FileInfoRetriever
 * \sa ShellActions
 * \sa DirectoryWatcher
 */
FileSystemModel::FileSystemModel(QObject *parent) : QAbstractItemModel(parent)
{
    // This is mandatory if you want the dataChanged signal to work when emitting it in a different thread.
    // It's because of the "roles" argument of the signal. The fingerprint of the method is as follows:
    // dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
    qRegisterMetaType<QVector<int>>("QVector<int>");

    fileInfoRetriever = new PlatformInfoRetriever();
    shellActions = new PlatformShellActions();

    // Get the default icons
    QFileIconProvider iconProvider;
    driveIcon = iconProvider.icon(QFileIconProvider::Drive);
    folderIcon = iconProvider.icon(QFileIconProvider::Folder);
    fileIcon = iconProvider.icon(QFileIconProvider::File);

    connect(fileInfoRetriever, &FileInfoRetriever::parentInfoUpdated, this, &FileSystemModel::parentInfoUpdated);
    connect(fileInfoRetriever, &FileInfoRetriever::parentChildrenUpdated, this, &FileSystemModel::parentChildrenUpdated);
    connect(fileInfoRetriever, &FileInfoRetriever::itemUpdated, this, &FileSystemModel::itemUpdated);
    connect(fileInfoRetriever, &FileInfoRetriever::iconUpdated, this, &FileSystemModel::iconUpdated);

    // Start new directory watcher to monitor directory changes
    watcher = new PlatformDirectoryWatcher(this);
    connect(watcher, &DirectoryWatcher::fileRename, this, &FileSystemModel::renamePath);
    connect(watcher, &DirectoryWatcher::fileModified, this, &FileSystemModel::refreshPath);
    connect(watcher, &DirectoryWatcher::fileAdded, this, &FileSystemModel::addPath);
    connect(watcher, &DirectoryWatcher::fileRemoved, this, &FileSystemModel::removePath);
    connect(watcher, &DirectoryWatcher::folderUpdated, this, &FileSystemModel::refreshFolder);

#ifdef Q_OS_WIN
    // WinDirectoryWatcher is faster for renames than WinDirectoryWatcherv2
    // Yes, I do this to show the new name 100ms faster to the user
    directoryWatcher = new WinDirectoryWatcher(this);
    connect(directoryWatcher, &DirectoryWatcher::fileRename, this, &FileSystemModel::renamePath);
    connect(directoryWatcher, &DirectoryWatcher::fileModified, this, &FileSystemModel::refreshPath);
    connect(directoryWatcher, &DirectoryWatcher::fileAdded, this, &FileSystemModel::addPath);
    connect(directoryWatcher, &DirectoryWatcher::fileRemoved, this, &FileSystemModel::removePath);
    connect(directoryWatcher, &DirectoryWatcher::folderUpdated, this, &FileSystemModel::refreshFolder);
#endif

    fileInfoRetriever->start();

    // Garbage collector
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &FileSystemModel::garbageCollector);
    timer->start(300'000);
}

/*!
 * \brief The destructor.
 *
 * This function will wait for any pending tasks from the FileInfoRetriever object, and then destroy it, along with
 * the DirectoryWatcher and the ShellActions object.
 */
FileSystemModel::~FileSystemModel()
{
    fileInfoRetriever->quit();

    // Stop the previous directory watcher if valid
    if (watcher != nullptr) {
        disconnect(watcher, &DirectoryWatcher::fileRename, this, &FileSystemModel::renamePath);
        disconnect(watcher, &DirectoryWatcher::fileModified, this, &FileSystemModel::refreshPath);
        disconnect(watcher, &DirectoryWatcher::fileAdded, this, &FileSystemModel::addPath);
        disconnect(watcher, &DirectoryWatcher::fileRemoved, this, &FileSystemModel::removePath);
        disconnect(watcher, &DirectoryWatcher::folderUpdated, this, &FileSystemModel::refreshFolder);
        watcher->deleteLater();
        watcher = nullptr;

#ifdef Q_OS_WIN
        disconnect(directoryWatcher, &DirectoryWatcher::fileRename, this, &FileSystemModel::renamePath);
        disconnect(directoryWatcher, &DirectoryWatcher::fileModified, this, &FileSystemModel::refreshPath);
        disconnect(directoryWatcher, &DirectoryWatcher::fileAdded, this, &FileSystemModel::addPath);
        disconnect(directoryWatcher, &DirectoryWatcher::fileRemoved, this, &FileSystemModel::removePath);
        disconnect(directoryWatcher, &DirectoryWatcher::folderUpdated, this, &FileSystemModel::refreshFolder);
        directoryWatcher->deleteLater();
        directoryWatcher = nullptr;
#endif
    }

    if (root != nullptr)
        delete root;

    if (fileInfoRetriever != nullptr)
        delete fileInfoRetriever;

    if (shellActions != nullptr)
        delete shellActions;
}

/*!
 * \brief Returns the index in the model specified by the given \a row, \a column and \a parent index.
 * \param row row of the index.
 * \param column column of the index.
 * \param parent parent of the index.
 * \return a QModelIndex object with the \a row, \a column and \a parent index specified.
 *
 * If the scope of the model is a Tree, the parent is valid. Only the root item will have an invalid parent.
 *
 * If the scope of the model is a List, the parent must be not valid.
 *
 */
QModelIndex FileSystemModel::index(int row, int column, const QModelIndex &parent) const
{
    // qDebug() << "index " << row << " " << column;
    if (row < 0 || column < 0 || row >= rowCount(parent) || column >= columnCount(parent)) {
        qDebug() << "index " << row << "(" << rowCount(parent) << ")" << column <<  "(" << columnCount(parent) << ")" << reinterpret_cast<FileSystemItem *>(parent.internalPointer())->getPath();
        return QModelIndex();
    }

    // If parent is not valid it means TreeView is asking for the root
    if (!parent.isValid() && !root->getDisplayName().isEmpty())
        return createIndex(row, column, this->root);

    if (parent.isValid() && parent.internalPointer() != nullptr) {
        FileSystemItem *parentItem = getFileSystemItem(parent);

        return createIndex(row, column, parentItem->getChildAt(row));
    }

    return QModelIndex();
}

/*!
 * \brief Returns the parent of the specicied \a index.
 * \param index a QModelIndex object.
 * \return the parent of the \a index specified as a QModelIndex object.
 *
 * If the scope of the model is a List, this function will always return an invalid index.
 */
QModelIndex FileSystemModel::parent(const QModelIndex &index) const
{
    if (index.isValid() && index.internalPointer() != nullptr) {
        FileSystemItem *parent = getFileSystemItem(index)->getParent();
        if (parent != nullptr) {
            FileSystemItem *grandParent = parent->getParent();
            int row = (grandParent != nullptr) ? grandParent->childRow(parent) : 0;
            return createIndex(row, 0, parent);
        }
    }
    return QModelIndex();
}

/*!
 * \brief Returns the number of rows under the given \a parent.
 * \param parent a QModelIndex.
 * \return Number of children of the \a parent.
 *
 * If the scope of the model is a List, the parent is ignored. It will always return the
 * number of elements of the list.
 */
int FileSystemModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    if (parent.isValid() && parent.internalPointer() != nullptr) {

        FileSystemItem *item = getFileSystemItem(parent);

        // If the item is currently being fetched return 0 until it's done
        if (item->getLock())
            return 0;

        return item->childrenCount();
    }

    // For now root has only one row: "My PC".  We may add more later
    if (!parent.isValid())
        return 1;

    return 0;
}

/*!
 * \brief Returns the number of columns for the model.
 * \param parent a QModelIndex. It is ignorex.
 * \return Number of columns for the model, depending on the scope.
 *
 * If the scope of the model is a Tree, only one column is returned, since the Tree model
 * only have names and nothing else.
 *
 * If the scope of the model is a List, then this function returns the number of columns
 * implemented.
 */
int FileSystemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return Columns::MaxColumns;

}

/*!
 * \brief Returns the data under the given \a role ffor the item specified by \a index
 * \param index a QModelIndex
 * \param role a Qt::ItemDataRole
 * \return data for \a index of the specified \a role
 *
 * If the role is Qt::DecorationRole this function will do the following:
 *
 * 1. The first time it is requested it will assign a "fake icon" to the index which is basically a default icon for it.
 * 2. The second time it will run the function FileSystemModel::getIcon in the background to request the real icon.
 *
 * This is because the views will ask for the icon of all items while constructing the view to measure each row size.  Then
 * the view will request the icon a second time to actually show it in the view, only if its index is visible.
 *
 * This way we will only request icons from the filesystem for indexes that are actually visible in the viewport.
 * We get better performance in very long file listings like C:\Windows\System32.
 *
 * If the role is Qt::DisplayRole, this function will customize the output for the Size column. The LastChangeSince column
 * is millisecs since epoch, so the model can sort by it easily.  A delegate for the LastChangeSince column is needed to
 * show actual human readable dates.
 *
 * If the role is Qt::ForegroundRole, it will show a different color for folders.
 *
 * If the role is Qt::TextAlignmentRole, it will align to the right the Size and Date columns.
 *
 */
QVariant FileSystemModel::data(const QModelIndex &index, int role) const
{
    // qDebug() << "data" << index.row() << " " << index.column();
    if (index.isValid()  && index.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = getFileSystemItem(index);
        Q_ASSERT(fileSystemItem);
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
                        fileSystemItem->setFakeIcon(false);
                        fileInfoRetriever->getIcon(fileSystemItem);
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
                if (/*fileInfoRetriever->getScope() == FileInfoRetriever::List && */fileSystemItem->isFolder()) {
                    return QVariant(QBrush(Qt::darkBlue));
                } else {
                    // This will make disabled columns look normal
                    QPalette palette = QGuiApplication::palette();
                    return QVariant(palette.text());
                }
                break;
            case FileSystemModel::PathRole:
                return QVariant(fileSystemItem->getPath());

            case FileSystemModel::ExtensionRole:
                return QVariant(fileSystemItem->getExtension());

            case FileSystemModel::FileRole:
                return !(fileSystemItem->isFolder() || fileSystemItem->isDrive());

            case FileSystemModel::FolderRole:
                return fileSystemItem->isFolder();

            case FileSystemModel::DriveRole:
                return fileSystemItem->isDrive();

            case FileSystemModel::HasSubFoldersRole:
                return fileSystemItem->getHasSubFolders();

            case FileSystemModel::AllChildrenFetchedRole:
                return fileSystemItem->areAllChildrenFetched();

            case FileSystemModel::CapabilitiesRole:
                return fileSystemItem->getCapabilities();

            case FileSystemModel::ErrorCodeRole:
                return fileSystemItem->getErrorCode();

            case FileSystemModel::ErrorMessageRole:
                return fileSystemItem->getErrorMessage();

            case FileSystemModel::RefCounterRole:
                return fileSystemItem->getRefCounter();

        }
    }
    return QVariant();
}

/*!
 * \brief Returns the data for the given \a role and \a section in the header with the specified \a orientation
 * \param section column number for horizontal headers or row number for vertical headers
 * \param orientation vertical or horizontal orientation as a Qt::Orientation
 * \param role a Qt::ItemDataRole
 *
 * This function just returns the name of the headers depending on what column is requested and its alignment.
 */
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
        FileSystemItem *fileSystemItem = getFileSystemItem(parent);
        //return fileSystemItem->getHasSubFolders();
        return !fileSystemItem->areAllChildrenFetched() || fileSystemItem->childrenCount() > 0;
    }
    return true;
}

bool FileSystemModel::canFetchMore(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = getFileSystemItem(parent);

        // We have to be sure the item is not currently fetching more (getLock() has to return false)
        if (!fileSystemItem->getLock() && !fileSystemItem->areAllChildrenFetched())
            return true;
    }

    return false;
}

/*!
 * \brief Fetches any available data for the items with the parent sepcified by the \a parent index.
 * \param parent a QModelIndex
 *
 * If the children items of \a parent hasn't been fetched already, this function will start a
 * background process to fetch them.
 *
 * If this model is already fetching items for another \a parent, that fetching process will be canceled
 * by the FileInfoRetriever object, and start this one.
 *
 * \sa FileInfoRetriever::getChildren
 *
 */
void FileSystemModel::fetchMore(const QModelIndex &parent)
{
    qDebug() << "FileSystemModel::fetchMore";
    if (parent.isValid() && parent.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = getFileSystemItem(parent);

        // We don't care if fetchingMore was already true since FileInfoRetriever will cancel the current fetch
        if (!fileSystemItem->getLock() && !fileSystemItem->areAllChildrenFetched()) {

            fileSystemItem->setLock(true);

            QVector<int> roles;
            roles.append(FileSystemModel::AllChildrenFetchedRole);
            emit dataChanged(parent, parent, roles);

            fileInfoRetriever->getChildren(fileSystemItem);
        }
    }
}

void FileSystemModel::removeIndexes(QModelIndexList indexList, bool permanent)
{
    qDebug() << "FileSystemModel::removeIndexes";
    QList<QUrl> urls;
    for (QModelIndex index : indexList) {
        if (index.isValid())
            urls.append(QUrl::fromLocalFile(getFileSystemItem(index)->getPath()));
    }

    shellActions->removeItems(urls, permanent);
}

void FileSystemModel::startWatch(const QModelIndex &parent, QString verb)
{
    FileSystemItem *parentItem = getFileSystemItem(parent);
    qDebug() << "FileSystemModel::startWatch" << verb << parentItem->getPath();

    watch = true;
    parentBeingWatched = parentItem;
    extensionBeingWatched = verb;
}

bool FileSystemModel::willRecycle(const QModelIndex &index)
{
    return fileInfoRetriever->willRecycle(getFileSystemItem(index));
}

void FileSystemModel::refreshFolder(QString path)
{
    refreshIndex(index(path));
}

void FileSystemModel::refreshIndex(QModelIndex index) {

    if (index.isValid()) {

        FileSystemItem *item = getFileSystemItem(index);

        // Do not refresh an index being fetched at this time
        if (!item->getLock()) {
            qDebug() << "FileSystemModel::refreshIndex";
            removeAllRows(index);
        }
    }
}

QIcon FileSystemModel::getFolderIcon() const
{
    return folderIcon;
}

void FileSystemModel::refreshPath(QString fileName)
{
    qDebug() << "FileSystemModel::refreshPath" << fileName;

    // Only the list view has other columns

    QModelIndex itemIndex = index(fileName);
    if (itemIndex.isValid() && itemIndex.internalPointer() != nullptr) {

        FileSystemItem *fileSystemItem = getFileSystemItem(itemIndex);
        if (fileSystemItem) {

            // This can fail and return false if the file is deleted immediately after a modification,
            // but we don't remove the file here because we will receive another notification that will handle that

            qDebug() << "FileSystemModel::refreshPath" << "item found" << fileName;
            fileInfoRetriever->refreshItem(fileSystemItem);
        } else {
            // Try to add it
            qDebug() << "FileSystemModel::refreshPath" << "item not found, trying to add" << fileName;
            addPath(fileName);
        }
    }
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
            return Qt::NoItemFlags; //Qt::ItemIsEnabled;

        itemFlags |= Qt::ItemIsEditable | Qt::ItemIsDragEnabled;

        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
        Q_ASSERT(fileSystemItem);
        if (fileSystemItem != nullptr && (fileSystemItem->isFolder() || fileSystemItem->isDrive()))
                itemFlags |= Qt::ItemIsDropEnabled;

    } else {
        // Enable drop to the background (empty areas)
        itemFlags |= Qt::ItemIsDropEnabled;
    }

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
    qDebug() << "FileSystemModel::mimeData";
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

bool FileSystemModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(row)
    Q_UNUSED(column)

    FileSystemItem *item = getFileSystemItem(parent);
    if (item != nullptr) {
        QString itemPath = item->getPath();

        if (!(item->getCapabilities() & FSI_DROP_TARGET))
            return false;

        if (action == Qt::MoveAction) {
            // We can't move data to the same folder
            QList<QUrl> urls = data->urls();

            if (urls.size() > 0) {

                if (item->isFolder())
                    itemPath += separator();

                QString path = urls[0].toLocalFile().replace('/', separator());
                int index = qMax(path.lastIndexOf(separator()), itemPath.lastIndexOf(separator()));

                if (path.left(index) == itemPath.left(index))
                    return false;
            }
        }

        return true;
    }

    return false;
}

bool FileSystemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(row)
    Q_UNUSED(column)
    qDebug() << "FileSystemModel::dropMimeData" << action;
    QList<QUrl> urls = data->urls();

    QString dstPath = getDropPath(parent);
    qDebug() << "FileSystemModel::dropMimeData Dropping" << urls << "to" << dstPath;

    switch(action) {
        case Qt::CopyAction:
            shellActions->copyItems(urls, dstPath);
            break;
        case Qt::MoveAction:
            shellActions->moveItems(urls, dstPath);

#ifdef Q_OS_WIN
            // In Windows moving from the File Explorer to this application and returning true makes the File Explorer
            // delete the source file before it is copied
            return false;
#endif
            break;
        case Qt::LinkAction:
            shellActions->linkItems(urls, dstPath);
            break;
        default:
            break;
    }

    return true;
}

QStringList FileSystemModel::mimeTypes() const
{
    return QStringList(QLatin1String("text/uri-list"));
}

bool FileSystemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && index.column() == 0 && index.internalPointer() != nullptr) {

        FileSystemItem *item = getFileSystemItem(index);

        switch(role) {

            case Qt::EditRole: {
                QString newName = value.toString();
                if (item->getDisplayName() == newName)
                    return false;

                qDebug() << "FileSystemModel::setData rename" << item->getPath() << "to" << newName;
                QUrl url = QUrl::fromLocalFile(item->getPath());

                shellActions->renameItem(url, newName);
                return true;
            }

            // This is a refresh request
            case FileSystemModel::AllChildrenFetchedRole:
                if (!value.toBool()) {
                    refreshIndex(index);
                    return true;
                }
                break;

            case FileSystemModel::IncreaseRefCounterRole:
                item->incRefCounter();
                garbageMutex.lock();
                garbage.removeOne(item);
                garbageMutex.unlock();
                break;

            case FileSystemModel::DecreaseRefCounterRole:
                item->decRefCounter();
                if (item->getRefCounter() == 0) {
                    garbageMutex.lock();
                    garbage.append(item);
                    garbageMutex.unlock();
                }
                break;
        }
    }

    return false;
}

void FileSystemModel::setDefaultRoot()
{
    setRoot(fileInfoRetriever->getRootPath());
}

QModelIndex FileSystemModel::index(FileSystemItem *item) const
{
    // Eventually this should support other submodels
    if (item == root)
        return createIndex(0, 0, item);

    FileSystemItem *parent = item->getParent();

    if (parent == nullptr)
        return QModelIndex();

    return createIndex(parent->childRow(item), 0, item);
}

QModelIndex FileSystemModel::index(QString path) const
{
    QModelIndex parentIndex = parent(path);
    FileSystemItem *parentItem = getFileSystemItem(parentIndex);
    if (parentItem != nullptr) {
        FileSystemItem *itemIndex = parentItem->getChild(path);
        if (itemIndex != nullptr) {
            int row = parentItem->childRow(itemIndex);

            return index(row, 0, parentIndex);
        }

    }

    return QModelIndex();
}


/*!
 * \brief Returns the path of a drop in an index item
 * \param index a QModelIndex where the user is making the drop
 * \return a QString with the path of the drop
 *
 * This function returns the path of the QModelIndex index if this index has the drop feature enabled,
 * otherwise it will return the path of the background view.
 */
QString FileSystemModel::getDropPath(QModelIndex index) const {

    QString dstPath;

    if (!index.isValid() || !(index.flags() & Qt::ItemIsDropEnabled)) {
        dstPath = getRoot()->getPath();
    } else {
        if (index.isValid() && index.internalPointer() != nullptr)
            dstPath = getFileSystemItem(index)->getPath();
    }

    return dstPath;
}

/*!
 * \brief Returns the drag actions supported by this model for the specified indexes
 * \param indexes a list of QModelIndex objects
 * \return a Qt::DropActions object with the drop actions supported for the indexes
 *
 * This function returns the drag actions supported by the specified indexes.
 *
 * A drive cannot be moved or copied, but the user can create a link to it.
 */
Qt::DropActions FileSystemModel::supportedDragActionsForIndexes(QModelIndexList indexes)
{
#ifdef Q_OS_WIN
    for (QModelIndex index : indexes) {
        if (index.isValid() && index.flags() & Qt::ItemIsDragEnabled) {
            FileSystemItem *fileSystemItem = static_cast<FileSystemItem *>(index.internalPointer());
            if (fileSystemItem->isDrive())
                return Qt::LinkAction;
        }
    }
#endif

    return supportedDragActions();
}

/*!
 * \brief Returns a default drop action for the mime data when it's being dropped on an index
 * \param index a QModelIndex object where the mime data is being dropped on
 * \param data QMimeData with the MIME data being dragged and dropped on the index
 * \param possibleActions a Qt::DropActions object with the possible actions
 * \return a Qt::DropAction object with the default drop action
 *
 * In Windows, this function will choose a default MoveAction if some files are being dragged in the same drive.
 *
 * Otherwise it will choose a CopyAction as default if supported, or a MoveAction if CopyAction is not supported,
 * and finally a LinkAction if nothing else is supported. LinkActions are always supported.
 */
Qt::DropAction FileSystemModel::defaultDropActionForIndex(QModelIndex index, const QMimeData *data, Qt::DropActions possibleActions)
{
    FileSystemItem *item {};

    if (!index.isValid() || !(index.flags() & Qt::ItemIsDropEnabled)) {
        item = getRoot();
    } else {
        if (index.isValid() && index.internalPointer() != nullptr)
            item = getFileSystemItem(index);
    }

    if (item != nullptr) {
        QString dstPath = item->getPath();
        if (item->isFolder())
            dstPath += separator();

        QList<QUrl> urls = data->urls();

        qDebug() << "Checking if " << urls << "can be dropped to " << dstPath;

        if (urls.size() > 0) {

            QString path = urls[0].toLocalFile().replace('/', separator());
            int idx = qMax(path.lastIndexOf(separator()), dstPath.lastIndexOf(separator()));

            if (path.left(idx) == dstPath.left(idx))
                return Qt::IgnoreAction;
        }

#ifdef Q_OS_WIN
        if (possibleActions & Qt::MoveAction) {

            // All items being dragged must be from the same drive
            QString drive;
            for (QUrl url : data->urls()) {
                QString path = url.toLocalFile();
                if (drive.isEmpty()) {
                    drive = path.left(3);
                } else if (drive != path.left(3)) {
                    drive = QString();
                    break;
                }
            }

            if (drive.right(1) == '/')
                drive = drive.left(2) + separator();

            QString dstPath = getDropPath(index);

            if (!drive.isEmpty() && dstPath.left(3) == drive) {
                // Default action for files in the same drive is MoveAction
                return Qt::MoveAction;
            }
        }
#endif

    }

    if (possibleActions & Qt::CopyAction)
        return Qt::CopyAction;

    if (possibleActions & Qt::MoveAction)
        return Qt::MoveAction;

    return Qt::LinkAction;
}


QModelIndex FileSystemModel::relativeIndex(QString path, QModelIndex parent)
{
    qDebug() << "FileSystemModel::relativeIndex" << path;
    if (parent.isValid() && parent.internalPointer() != nullptr) {
        FileSystemItem *item = getFileSystemItem(parent);
        FileSystemItem *child = item->getChild(path);

        return createIndex(item->childRow(child), 0, child);

    }
    return QModelIndex();
}

void FileSystemModel::setRoot(const QString path)
{
    qDebug() << "FileSystemModel::setRoot" << path;

    beginResetModel();

    FileSystemItem *deleteLater = root;
    root = new FileSystemItem(path);
    root->setParent(nullptr);

    fileInfoRetriever->getInfo(root);
    qDebug() << "FileSystemModel::setRoot root is ready" << path;

    // Now we can delete the old root structure safely
    if (deleteLater != nullptr) {
        qDebug() << "FileSystemModel::setRoot" << "freeing older root";
        watcher->removePath(deleteLater->getPath());
#ifdef Q_OS_WIN
        directoryWatcher->removePath(deleteLater->getPath());
#endif
        // Process all dataChanged events that could still modify the old root structure
        QApplication::processEvents();

        // Finally delete the old root
        delete deleteLater;
    }
}


FileSystemItem *FileSystemModel::getRoot() const
{
    return root;
}

bool FileSystemModel::removeAllRows(const QModelIndex &parent)
{
    FileSystemItem *item = getFileSystemItem(parent);
    if (item != nullptr) {
        int count = rowCount(parent);

        qDebug() << "FileSystemModel::removeAllRows removing" << count << "rows" << item->getLock();
        if (count > 0) {

            beginRemoveRows(parent, 0, count - 1);
            item->removeChildren();
            endRemoveRows();

            return true;
        }
    }

    return false;
}

QChar FileSystemModel::separator() const
{
    return QDir::separator();
}

QModelIndex FileSystemModel::parent(QString path) const
{
    qDebug() << "FileSystemModel::parent" << path;

    FileSystemItem *parentItem = root;

    if (path == fileInfoRetriever->getRootPath()) {
        qDebug() << "FileSystemModel::parent" << path << "this is root";
        return QModelIndex();
    }

    // Remove the last entry
    QString parentPath = path;

#ifdef Q_OS_WIN
    // If this is a drive
    if (path.length() == 3 && path[0].isLetter() && path[1] == ':' && path[2] == '\\') {
        qDebug() << "FileSystemModel::parent" << parentPath << "is a drive and parent is root";
        return createIndex(0, 0, root);
    }
#endif
    if (path.right(1) == separator())
        parentPath = path.left(path.length() - 1);

    int index = parentPath.lastIndexOf(separator());
    if (index < 0) {
        qDebug() << "FileSystemModel::parent" << parentPath << "seems to be root";
        return createIndex(0, 0, root);
    }

    parentPath = parentPath.left(index);
    QStringList pathList = parentPath.split(separator());
    QString absolutePath = QString();
    FileSystemItem *grandParentItem = nullptr;

    if (parentPath != root->getPath()) {

        for (auto folder : pathList) {
            if (!folder.isEmpty()) {

                if (absolutePath.isEmpty()) {

#ifdef Q_OS_WIN
                    if (folder.length() == 2 && folder[0].isLetter() && folder[1] == ':')
                        absolutePath = folder + separator();
                    else
                        absolutePath = folder;
#else
                    absolutePath = separator() + folder;
#endif

                } else if (absolutePath.at(absolutePath.length() - 1) != separator())
                    absolutePath += separator() + folder;
                else
                    absolutePath += folder;

                grandParentItem = parentItem;
                parentItem = parentItem->getChild(absolutePath);

                if (parentItem == nullptr) {
                    qDebug() << "FileSystemModel::parent" << "couldn't find child" << absolutePath;
                    return QModelIndex();
                }

            }
        }
    } else {
        return createIndex(0, 0, root);
    }

    if (grandParentItem == nullptr)
        return QModelIndex();

    qDebug() << "FileSystemModel::parent" << path << "is" << parentItem->getPath();
    return createIndex(grandParentItem->childRow(parentItem), 0, parentItem);
}

void FileSystemModel::parentInfoUpdated(FileSystemItem *parent)
{
    qDebug() << "FileSystemModel::parentInfoUpdated endResetModel";
    endResetModel();

    if (!parent->getErrorCode()) {
#ifdef Q_OS_WIN
        if (parent->getMediaType() == FileSystemItem::Fixed)
            directoryWatcher->addPath(parent->getPath());
        else
            watcher->addPath(parent->getPath());
#else
        watcher->addPath(parent->getPath());
#endif
    }

    QVector<int> roles;
    if (!parent->getErrorCode()) {
        roles.append(Qt::DisplayRole);
        roles.append(Qt::DecorationRole);
    } else {
        roles.append(FileSystemModel::ErrorCodeRole);
        roles.append(FileSystemModel::ErrorMessageRole);
    }

    QModelIndex parentIndex = index(parent);
    emit dataChanged(parentIndex, parentIndex, roles);
}

void FileSystemModel::parentChildrenUpdated(FileSystemItem *parent)
{
    QModelIndex parentIndex = index(parent);

    qDebug() << "FileSystemModel::parentInfoUpdated" << parent->childrenCount() << "children. Error code:" << parent->getErrorCode();

    // This only happens if the user cancelled the fetch by selecting another index
    if (parent->getErrorCode() == -1) {
        parent->removeChildren();
        parent->setLock(false);
        return;
    }

    int count = parent->childrenCount();

    if (count > 0) {
        beginInsertRows(parentIndex, 0, count - 1);
        parent->setLock(false);
        endInsertRows();
    } else
        parent->setLock(false);

    if (!parent->getErrorCode()) {
#ifndef Q_OS_WIN
        if (parent->getMediaType() == FileSystemItem::Fixed)
            directoryWatcher->addPath(parent->getPath());
        else
            watcher->addPath(parent->getPath());
#else
        watcher->addPath(parent->getPath());
#endif
    }

    QVector<int> roles;
    if (!parent->getErrorCode()) {
        roles.append(Qt::DisplayRole);
        roles.append(FileSystemModel::AllChildrenFetchedRole);
    } else {
        roles.append(FileSystemModel::ErrorCodeRole);
        roles.append(FileSystemModel::ErrorMessageRole);
    }

    emit dataChanged(parentIndex, parentIndex, roles);
}

void FileSystemModel::itemUpdated(FileSystemItem *item)
{
    if (item != nullptr) {

        FileSystemItem *parent = item->getParent();

        if (parent != nullptr) {
            int row = parent->childRow(item);
            if (row >= 0) {
                QModelIndex fromIndex = createIndex(row, 0, item);
                QModelIndex lastIndex = createIndex(row, Columns::MaxColumns - 1, item);
                QVector<int> roles;
                roles.append(Qt::DisplayRole);
                //roles.append(Qt::DecorationRole);
                emit dataChanged(fromIndex, lastIndex, roles);
            }
        }
    }
}

void FileSystemModel::iconUpdated(FileSystemItem *item)
{
    if (item != nullptr) {

        FileSystemItem *parent = item->getParent();

        if (parent != nullptr) {
            int row = parent->childRow(item);
            if (row >= 0) {
                QModelIndex fromIndex = createIndex(row, 0, item);
                QModelIndex lastIndex = createIndex(row, 0, item);
                QVector<int> roles;
                roles.append(Qt::DecorationRole);
                emit dataChanged(fromIndex, lastIndex, roles);
            }
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

void FileSystemModel::renamePath(QString oldFileName, QString newFileName)
{
    if (oldFileName == newFileName)
        return;

    qDebug() << "FileSystemModel::renamePath" << oldFileName << newFileName;

    FileSystemItem *parentItem;

    QPersistentModelIndex parentIndex = parent(oldFileName);

    if (!parentIndex.isValid())
        return;

    parentItem = getFileSystemItem(parentIndex);

    FileSystemItem *fileSystemItem = parentItem->getChild(oldFileName);
    if (fileSystemItem) {

        QModelIndex itemIndex = index(fileSystemItem);

        parentItem->updateChildPath(fileSystemItem, newFileName);

        // TODO: Change this to refreshItem() + icon request, no need to send signals
        // Sets the display name, type and icon if it's needed to be retrieved again
        fileInfoRetriever->setDisplayNameOf(fileSystemItem);

        QVector<int> roles;
        roles.append(Qt::DisplayRole);
        QModelIndex bottomRight = index(itemIndex.row(), FileSystemModel::MaxColumns - 1, itemIndex.parent());
        emit dataChanged(itemIndex, bottomRight, roles);

    } else {

        qDebug() << "FileSystemModel::renamePath" << "couldn't find index for" << oldFileName << "in" << root->getPath() << " - Trying addPath";
        addPath(newFileName);
    }
}

void FileSystemModel::addPath(QString fileName)
{
    qDebug() << "FileSystemModel::addPath" << fileName;

    FileSystemItem *parentItem;

    QPersistentModelIndex parentIndex = parent(fileName);

    if (!parentIndex.isValid()) {
        qDebug() << "FileSystemModel::addPath" << "couldn't find a valid parent for" << fileName;
        return;
    }

    parentItem = getFileSystemItem(parentIndex);

    FileSystemItem *fileSystemItem = parentItem->getChild(fileName);
    if (fileSystemItem != nullptr) {
        qDebug() << "FileSystemModel::addPath" << "we already have this path";
        return;
    }

    fileSystemItem = new FileSystemItem(fileName);

    // TODO: Delete this, refreshItem() already sets the displayName
    fileInfoRetriever->setDisplayNameOf(fileSystemItem);

    bool result = fileInfoRetriever->refreshItem(fileSystemItem);

    // Skip items that failed (do not exist anymore)
    if (!result) {
        qDebug() << "FileSystemModel::addPath" << "skipping file" << fileName;

        // This is safe since refreshItem won't emit a signal if result was false
        delete fileSystemItem;

        return;
    }

    int row = parentItem->childrenCount();
    beginInsertRows(parentIndex, row, row);
    parentItem->addChild(fileSystemItem);
    endInsertRows();

    qDebug() << "FileSystemModel::addPath" << "row inserted for" << fileName;

    // If we were waiting for an item like this, tell the view the user has to set a new name for it
    if (watch && parentItem == parentBeingWatched &&
            ((extensionBeingWatched == "NewFolder" && fileSystemItem->isFolder()) ||
                   extensionBeingWatched.right(extensionBeingWatched.size() - 1) == fileSystemItem->getExtension())) {

        QModelIndex itemIndex = createIndex(row, 0, fileSystemItem);
        QVector<int> roles;
        roles.append(FileSystemModel::ShouldEditRole);
        emit dataChanged(itemIndex, itemIndex, roles);
        watch = false;
    }
}

void FileSystemModel::removePath(QString fileName)
{
    qDebug() << "FileSystemModel::removePath" << fileName;

    FileSystemItem *parentItem;

    // QPersistentModelIndex parentIndex = parent(fileName);
    QModelIndex parentIndex = parent(fileName);

    if (!parentIndex.isValid())
        return;

    parentItem = getFileSystemItem(parentIndex);

    FileSystemItem *fileSystemItem = parentItem->getChild(fileName);
    if (fileSystemItem) {
        int row = parentItem->childRow(fileSystemItem);


        qDebug() << "parentIndex" << parentIndex << parentIndex.data(FileSystemModel::PathRole);
        qDebug() << "parentItem" << parentItem->getPath();

        beginRemoveRows(parentIndex, row, row);

        parentItem->removeChild(fileName);

        endRemoveRows();

        if (fileSystemItem->isFolder()) {

            garbageMutex.lock();
            garbage.removeAll(fileSystemItem);
            garbageMutex.unlock();

            // TODO: Remove all subfolders from the watchers as well not only this one
            watcher->removePath(fileSystemItem->getPath());
#ifdef Q_OS_WIN
            directoryWatcher->removePath(fileSystemItem->getPath());
#endif
        }

        delete fileSystemItem;

        qDebug() << "FileSystemModel::removePath" << fileName << "removed sucessfully. row =" << row;
    }
}

void FileSystemModel::garbageCollector()
{
    qDebug() << "FileSystemModel::garbageCollector started";

    garbageMutex.lock();
    for (FileSystemItem *item : garbage) {

        if (item->getRefCounter() == 0) {
            watcher->removePath(item->getPath());

#ifdef Q_OS_WIN
            directoryWatcher->removePath(item->getPath());
#endif
            removeAllRows(index(item));

        }
    }

    garbage.clear();
    garbageMutex.unlock();
    qDebug() << "FileSystemModel::garbageCollector finished";
}

