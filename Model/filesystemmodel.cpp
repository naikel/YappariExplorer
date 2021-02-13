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
#include "filesystemmodel.h"

#ifdef Q_OS_WIN
#   include "Shell/Win/winfileinforetriever.h"
#   include "Shell/Win/winshellactions.h"
#   include "Shell/Win/windirectorywatcher.h"
#   include "Shell/Win/windirectorywatcherv2.h"
#   define PlatformInfoRetriever()                 WinFileInfoRetriever()
#   define PlatformShellActions()                  WinShellActions()
#   define PlatformDirectoryWatcher(parent)        WinDirectoryWatcherv2(parent)
#else
#   include "Shell/Unix/unixfileinforetriever.h"
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
FileSystemModel::FileSystemModel(FileInfoRetriever::Scope scope, QObject *parent) : QAbstractItemModel(parent)
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

    // Start the thread of the file retriever
    fileInfoRetriever->setScope(scope);

    connect(fileInfoRetriever, &FileInfoRetriever::parentUpdated, this, &FileSystemModel::parentUpdated);
    connect(fileInfoRetriever, &FileInfoRetriever::itemUpdated, this, &FileSystemModel::itemUpdated);
    connect(fileInfoRetriever, &FileInfoRetriever::extendedInfoUpdated, this, &FileSystemModel::extendedInfoUpdated);

    // Start new directory watcher to monitor directory changes
    watcher = new PlatformDirectoryWatcher(this);
    connect(watcher, &DirectoryWatcher::fileRename, this, &FileSystemModel::renamePath);
    connect(watcher, &DirectoryWatcher::fileModified, this, &FileSystemModel::refreshPath);
    connect(watcher, &DirectoryWatcher::fileAdded, this, &FileSystemModel::addPath);
    connect(watcher, &DirectoryWatcher::fileRemoved, this, &FileSystemModel::removePath);
    // TODO: Refresh by path here, checking if it's the root, or only refreshing the branch
    connect(watcher, &DirectoryWatcher::folderUpdated, this, &FileSystemModel::refresh);

#ifdef Q_OS_WIN
        // WinDirectoryWatcher is faster for renames than WinDirectoryWatcherv2
        // Yes, I do this to show the new name 100ms faster to the user
        directoryWatcher = new WinDirectoryWatcher(this);
        connect(directoryWatcher, &DirectoryWatcher::fileRename, this, &FileSystemModel::renamePath);
        connect(directoryWatcher, &DirectoryWatcher::fileModified, this, &FileSystemModel::refreshPath);
        connect(directoryWatcher, &DirectoryWatcher::fileAdded, this, &FileSystemModel::addPath);
        connect(directoryWatcher, &DirectoryWatcher::fileRemoved, this, &FileSystemModel::removePath);
        connect(directoryWatcher, &DirectoryWatcher::folderUpdated, this, &FileSystemModel::refresh);
#endif


    // Initialize the history
    history = new FileSystemHistory(this);
}

/*!
 * \brief The destructor.
 *
 * This function will wait for any pending tasks from the FileInfoRetriever object, and then destroy it, along with
 * the DirectoryWatcher and the ShellActions object.
 */
FileSystemModel::~FileSystemModel()
{
    // Abort all the pending threads
    pool.clear();
    qDebug() << "FileSystemModel::~FileSystemModel: Waiting for all threads to finish";
    pool.waitForDone();
    qDebug() << "FileSystemModel::~FileSystemModel: All threads finished";

    // Stop the previous directory watcher if valid
    if (watcher != nullptr) {
        disconnect(watcher, &DirectoryWatcher::fileRename, this, &FileSystemModel::renamePath);
        disconnect(watcher, &DirectoryWatcher::fileModified, this, &FileSystemModel::refreshPath);
        disconnect(watcher, &DirectoryWatcher::fileAdded, this, &FileSystemModel::addPath);
        disconnect(watcher, &DirectoryWatcher::fileRemoved, this, &FileSystemModel::removePath);
        disconnect(watcher, &DirectoryWatcher::folderUpdated, this, &FileSystemModel::refresh);
        watcher->deleteLater();
        watcher = nullptr;

#ifdef Q_OS_WIN
        disconnect(directoryWatcher, &DirectoryWatcher::fileRename, this, &FileSystemModel::renamePath);
        disconnect(directoryWatcher, &DirectoryWatcher::fileModified, this, &FileSystemModel::refreshPath);
        disconnect(directoryWatcher, &DirectoryWatcher::fileAdded, this, &FileSystemModel::addPath);
        disconnect(directoryWatcher, &DirectoryWatcher::fileRemoved, this, &FileSystemModel::removePath);
        disconnect(directoryWatcher, &DirectoryWatcher::folderUpdated, this, &FileSystemModel::refresh);
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
    // qDebug() << "index " << fileInfoRetriever->getScope() << row << " " << column;
    if (row < 0 || column < 0 || row >= rowCount(parent) || column >= columnCount(parent))
        return QModelIndex();

    // If parent is not valid it means TreeView is asking for the root
    // or it is a ListView
    if (!parent.isValid() && !root->getDisplayName().isEmpty())
        return createIndex(row, column, (fileInfoRetriever->getScope() == FileInfoRetriever::List) ? root->getChildAt(row) : this->root);

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
    // List: The parent of a top level index should be invalid
    if (fileInfoRetriever->getScope() == FileInfoRetriever::List)
        return QModelIndex();

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

    if (fileInfoRetriever->getScope() == FileInfoRetriever::List)
        return root->childrenCount();

    if (parent.isValid() && parent.internalPointer() != nullptr)
        return getFileSystemItem(parent)->childrenCount();

    return 1;
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
    if (fileInfoRetriever->getScope() == FileInfoRetriever::List)
        return Columns::MaxColumns;

    return 1;
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
    // qDebug() << "data" << fileInfoRetriever->getScope() << index.row() << " " << index.column();
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
                } else {
                    // This will make disabled columns look normal
                    QPalette palette = QGuiApplication::palette();
                    return QVariant(palette.text());
                }
                break;
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
        return fileSystemItem->getHasSubFolders();
    }
    return true;
}

bool FileSystemModel::canFetchMore(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.internalPointer() != nullptr) {
       FileSystemItem *fileSystemItem = getFileSystemItem(parent);
        if (fileSystemItem->getHasSubFolders() && !fileSystemItem->areAllChildrenFetched())
            return true;
    }

    return false;
}

void FileSystemModel::fetchMore(const QModelIndex &parent)
{
    if (parent.isValid() && parent.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = getFileSystemItem(parent);
        if (fileSystemItem->getHasSubFolders() && !fileSystemItem->areAllChildrenFetched() && !fetchingMore.load()) {

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
    sort(column, order, QModelIndex());
}

void FileSystemModel::sort(int column, Qt::SortOrder order, QModelIndex parentIndex)
{
    qDebug() << "FileSystemModel::sort requested";
    currentSortOrder = order;
    currentSortColumn = column;

    QList<QPersistentModelIndex> parents;
    if (parentIndex.isValid())
        parents.append(parentIndex);

    emit layoutAboutToBeChanged(parents, QAbstractItemModel::VerticalSortHint);

    // We need to preserve the current persistent index list so we can change it later accordingly
    // The persistent index list keeps track of moving indexes, so if the user has an index selected
    // in the view, updating the list will reflect the selection change in the view.
    // Otherwise if the persistent index list is not updated the selection will stay in the same row
    // even if the item selected is in another row after the sorting.

    QModelIndexList oldList = persistentIndexList();
    QVector<QPair<FileSystemItem *, int>> oldNodes;
    for (QModelIndex &index : oldList) {
        QPair<FileSystemItem *, int> pair(static_cast<FileSystemItem*>(index.internalPointer()), index.column());
        oldNodes.append(pair);
    }

    // Sort the model
    FileSystemItem *parentItem = (parentIndex.isValid()) ? getFileSystemItem(parentIndex) : root;
    parentItem->sortChildren(column, order);

    // Create the new persistent index list which is basically the same list than before but with the
    // row updated in each entry
    QModelIndexList newList;
    const int numOldNodes = oldNodes.size();
    newList.reserve(numOldNodes);
    for (int i = 0; i < numOldNodes; ++i) {
        const QPair<FileSystemItem *, int> &oldNode = oldNodes.at(i);
        FileSystemItem *parentItem = oldNode.first->getParent();
        QModelIndex newIndex = createIndex((parentItem == nullptr) ? 0 : parentItem->childRow(oldNode.first), oldNode.second, oldNode.first);
        newList.append(newIndex);
    }

    // Report the model changes
    changePersistentIndexList(oldList, newList);
    emit layoutChanged();
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

void FileSystemModel::startWatch(FileSystemItem *parent, QString verb)
{
    watch = true;
    parentBeingWatched = parent;
    extensionBeingWatched = verb;
}

bool FileSystemModel::willRecycle(FileSystemItem *item)
{
    return fileInfoRetriever->willRecycle(item);
}

void FileSystemModel::goForward()
{
    if (canGoForward()) {
        QString path = history->getNextItem();
        setRoot(path);
    }
}

void FileSystemModel::goBack()
{
    if (canGoBack()) {
        QString path = history->getLastItem();
        setRoot(path);
    }
}


void FileSystemModel::goToPos(int pos)
{
    QString path = history->getItemAtPos(pos);
    if (!path.isNull())
        setRoot(path);
}

void FileSystemModel::goUp()
{
    if (canGoUp()) {
        const QString& currentPath = getRoot()->getPath();
        int index = currentPath.lastIndexOf(separator());
        if (index > 1) {

#ifdef Q_OS_WIN
            // If the parent folder is a drive we need to complete it
            if (index == 2 && currentPath.at(0).isLetter() && currentPath.at(1) == ':' && currentPath.at(2) == '\\') {

                if (currentPath.size() == 3) {
                    setRoot(fileInfoRetriever->getRootPath());
                    return;
                }
                index ++;
            }
#endif

            setRoot(currentPath.left(index));
        }
    }
}

bool FileSystemModel::canGoForward()
{
    return history->canGoForward();
}

bool FileSystemModel::canGoBack()
{
    return history->canGoBack();
}

bool FileSystemModel::canGoUp()
{
    return !(fileInfoRetriever->getRootPath() == getRoot()->getPath());
}

QList<HistoryEntry *> &FileSystemModel::getHistory(int *cursor) const
{
    *cursor = history->getCursor();
    return history->getPathList();
}

void FileSystemModel::refresh(QString path)
{
    if (path == root->getPath() && !settingRoot.load() && !fetchingMore.load()) {
        qDebug() << "FileSystemModel::refresh" << fileInfoRetriever->getScope();
        fetchingMore.store(true);
        emit fetchStarted();

        // Abort all the pending threads
        pool.clear();
        qDebug() << "FileSystemModel::refresh" << fileInfoRetriever->getScope() << "Waiting for all threads to finish";
        pool.waitForDone();
        qDebug() << "FileSystemModel::refresh" << fileInfoRetriever->getScope() << "All threads finished";

        // Process all dataChanged events that could still modify the old root structure
        QApplication::processEvents();

        removeAllRows(index(root));

        fileInfoRetriever->getInfo(root);
        qDebug() << "FileSystemModel::refresh root is ready" << root->getPath();

        // Now let's tell the views brand new rows are coming
        beginInsertRows(index(root), 0, 0);
    }
}

void FileSystemModel::refreshIndex(QModelIndex index) {

    if (index.isValid()) {
        qDebug() << "FileSystemModel::refresh" << fileInfoRetriever->getScope();
        if (removeAllRows(index)) {
            FileSystemItem *item = getFileSystemItem(index);
            item->setHasSubFolders(true);
        }
    }
}

void FileSystemModel::refreshPath(QString fileName)
{
    qDebug() << "FileSystemModel::refreshPath" << fileInfoRetriever->getScope() << fileName;

    // Only the list view has other columns
    if (fileInfoRetriever->getScope() == FileInfoRetriever::List) {

        if (fileName == root->getPath()) {
            // TODO: Sometimes new folders are added here but a refresh is too much
            return;
        }

        FileSystemItem *fileSystemItem = root->getChild(fileName);
        if (fileSystemItem) {

            // This can fail and return false if the file is deleted immediately after a modification,
            // but we don't remove the file here because we will receive another notification that will handle that

             qDebug() << "FileSystemModel::refreshPath" << fileInfoRetriever->getScope() << "item found" << fileName;
            fileInfoRetriever->refreshItem(fileSystemItem);
        } else {
            // Try to add it
            qDebug() << "FileSystemModel::refreshPath" << fileInfoRetriever->getScope() << "item not found, trying to add" << fileName;
            addPath(fileName);
        }
    }
}

void FileSystemModel::freeChildren(const QModelIndex &parent)
{
    qDebug() << "FileSystemModel::freeChildren";

    // TODO: There should use a better "cache" than this

    // Try again later if there are still some getIcon() calls pending
    if (pool.activeThreadCount() > 0) {
        QTimer::singleShot(5000, this, [this, parent]() { this->freeChildren(parent); });
        return;
    }

    if (parent.isValid()) {

        FileSystemItem *item = getFileSystemItem(parent);
        watcher->removePath(item->getPath());

#ifdef Q_OS_WIN
        directoryWatcher->removePath(item->getPath());
#endif

        // This will notify the view the rows are no longer valid
        removeAllRows(parent);

        // Set back the hasSubFolders flag so they can be retrieved again later
        item->setHasSubFolders(true);
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

    FileSystemItem *item = (fileInfoRetriever->getScope() == FileInfoRetriever::Scope::List && !parent.isValid()) ? root : getFileSystemItem(parent);
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
    // TODO: Not everything is a file

    if (role == Qt::EditRole && index.isValid() && index.column() == 0 && index.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem *>(index.internalPointer());

        QString newName = value.toString();
        if (fileSystemItem->getDisplayName() == newName)
            return false;

        qDebug() << "FileSystemModel::setData rename" << fileSystemItem->getPath() << "to" << newName;
        QUrl url = QUrl::fromLocalFile(fileSystemItem->getPath());

        shellActions->renameItem(url, newName);
        return true;
    }

    return false;
}

void FileSystemModel::setDefaultRoot()
{
    setRoot(fileInfoRetriever->getRootPath());
}

QModelIndex FileSystemModel::index(FileSystemItem *item) const
{
    FileSystemItem *parent = item->getParent();

    // ListView items don't have a parent
    if (parent == nullptr)
        return QModelIndex();

    return createIndex(parent->childRow(item), 0, item);
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

bool FileSystemModel::setRoot(const QString path)
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
    root = new FileSystemItem(path);
    root->setParent(nullptr);

    bool result = fileInfoRetriever->getInfo(root);
    qDebug() << "FileSystemModel::setRoot root is ready" << path;

    // Tell the views to invalidate everything before we delete the old data
    endResetModel();

    // Process all dataChanged events that could still modify the old root structure
    QApplication::processEvents();

    // Now let's tell the views brand new data is coming
    beginResetModel();

    // Now we can delete the old root structure safely
    if (deleteLater != nullptr) {
        qDebug() << "FileSystemModel::setRoot" << fileInfoRetriever->getScope() << "freeing older root";
        watcher->removePath(deleteLater->getPath());
#ifdef Q_OS_WIN
        directoryWatcher->removePath(deleteLater->getPath());
#endif
        // Process all dataChanged events that could still modify the old root structure
        QApplication::processEvents();

        // Finally delete the old root
        delete deleteLater;
    }

    history->insert(root);

    return result;
}


FileSystemItem *FileSystemModel::getRoot() const
{
    return root;
}

bool FileSystemModel::removeAllRows(const QModelIndex &parent)
{
    FileSystemItem *item {};
    if (fileInfoRetriever->getScope() == FileInfoRetriever::Tree && parent.isValid() && parent.internalPointer() != nullptr)
        item = getFileSystemItem(parent);
    else if (fileInfoRetriever->getScope() == FileInfoRetriever::List)
        item = root;

    if (item != nullptr) {
        int count = rowCount(parent);

        qDebug() << "FileSystemModel::removeAllRows removing" << count << "rows";
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
    qDebug() << "FileSystemModel::parent" << fileInfoRetriever->getScope() << path;

    if (path == fileInfoRetriever->getRootPath()) {
        qDebug() << "FileSystemModel::parent" << fileInfoRetriever->getScope() << path << "this is root";
        return QModelIndex();
    }

    // Remove the last entry
    QString parentPath = path;
    if (path.right(1) == separator())
        parentPath = path.left(path.length() - 1);

    int index = parentPath.lastIndexOf(separator());
    if (index < 0) {
        qDebug() << "FileSystemModel::parent" << fileInfoRetriever->getScope() << path << "seems to be root";
        return QModelIndex();
    }

    parentPath = parentPath.left(index);
    QStringList pathList = parentPath.split(separator());
    QString absolutePath = QString();
    FileSystemItem *grandParentItem = nullptr;
    FileSystemItem *parentItem = root;

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
                    qDebug() << "FileSystemModel::parent" << fileInfoRetriever->getScope() << "couldn't find child" << absolutePath;
                    return QModelIndex();
                }

            }
        }
    } else {
        return createIndex(0, 0, root);
    }

    if (grandParentItem == nullptr)
        return QModelIndex();

    return createIndex(grandParentItem->childRow(parentItem), 0, parentItem);
}

void FileSystemModel::parentUpdated(FileSystemItem *parent, qint32 err, QString errMessage)
{
    if (!err) {
        qDebug() << "FileSystemModel::parentUpdated" << parent->childrenCount() << " children";

        // TODO sort might block the GUI, what about sorting in the background?
        parent->sortChildren(currentSortColumn, currentSortOrder);
    }

    if (fetchingMore.load()) {
        qDebug() << "FileSystemModel::parentUpdated endInsertRows";
        endInsertRows();
        fetchingMore.store(false);
#ifdef Q_OS_WIN
        if (parent->getMediaType() == FileSystemItem::Fixed)
            directoryWatcher->addPath(parent->getPath());
        else
            watcher->addPath(parent->getPath());
#else
        watcher->addPath(parent->getPath());
#endif
    }

    if (settingRoot.load()) {
        qDebug() << "FileSystemModel::parentUpdated endResetModel";
        endResetModel();
        settingRoot.store(false);
#ifdef Q_OS_WIN
        if (parent->getMediaType() == FileSystemItem::Fixed)
            directoryWatcher->addPath(parent->getPath());
        else
            watcher->addPath(parent->getPath());
#else
        watcher->addPath(parent->getPath());
#endif

    }

    if (!err) {
        emit fetchFinished();
    } else {
        emit fetchFailed(err, errMessage);
    }
}

void FileSystemModel::itemUpdated(FileSystemItem *item)
{
    // Send the dataChanged signal only if this item has been already updated by the icon
    // Otherwise it's not necessary since the getIcon will send a dataChanged signal for it
    if (item != nullptr && !item->getIcon().isNull() && !item->hasFakeIcon()) {

        FileSystemItem *parent = item->getParent();

        // This could be an old signal (different path)
        if (parent != nullptr && item->getParent()->getPath() == root->getPath()) {
            int row = parent->childRow(item);
            if (row >= 0) {
                QModelIndex fromIndex = createIndex(row, Columns::Size, item);
                QModelIndex lastIndex = createIndex(row, Columns::MaxColumns, item);
                QVector<int> roles;
                roles.append(Qt::DisplayRole);
                roles.append(Qt::DecorationRole);
                emit dataChanged(fromIndex, lastIndex, roles);
            }
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
    // Index here is a clone.
    // Why? Because this method can be executed after the index is no longer valid

    if (index.isValid() && index.internalPointer() != nullptr) {

        FileSystemItem *fileSystemItem = getFileSystemItem(index);

        QIcon icon = fileInfoRetriever->getIcon(fileSystemItem);
        if (!icon.isNull()) {

            fileSystemItem->setIcon(icon);

            // This signal has very bad performance
            // The view will queue these and bundle them as efficient as possible
            QVector<int> roles;
            roles.append(Qt::DecorationRole);
            emit dataChanged(index, index, roles);
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
    qDebug() << "FileSystemModel::renamePath" << fileInfoRetriever->getScope() << oldFileName << newFileName;

    FileSystemItem *parentItem;

    QPersistentModelIndex parentIndex;
    if (fileInfoRetriever->getScope() == FileInfoRetriever::List) {
        parentIndex = QModelIndex();
        parentItem = root;
    } else {

         parentIndex = parent(oldFileName);

        if (!parentIndex.isValid())
            return;

        parentItem = getFileSystemItem(parentIndex);
    }

    FileSystemItem *fileSystemItem = parentItem->getChild(oldFileName);
    if (fileSystemItem) {

        fileSystemItem->setPath(newFileName);

        // Sets the display name, type and icon if it's needed to be retrieved again
        fileInfoRetriever->setDisplayNameOf(fileSystemItem);

        emit displayNameChanged(oldFileName, fileSystemItem);

        parentItem->removeChild(oldFileName);
        parentItem->addChild(fileSystemItem);
        sort(currentSortColumn, currentSortOrder, parentIndex);

    } else {

        if (fileInfoRetriever->getScope() == FileInfoRetriever::Scope::List) {

            // Check the item is for us, we might have changed the root already
            int i = oldFileName.lastIndexOf(separator());
            if (i > 0 && oldFileName.left(i) != root->getPath())
                return;
        }


        qDebug() << "FileSystemModel::renamePath" << fileInfoRetriever->getScope() << "couldn't find index for" << oldFileName << "in" << root->getPath() << " - Trying addPath";
        addPath(newFileName);
    }
}

void FileSystemModel::addPath(QString fileName)
{
    qDebug() << "FileSystemModel::addPath" << fileInfoRetriever->getScope() << fileName;

    FileSystemItem *parentItem;

    QPersistentModelIndex parentIndex;
    if (fileInfoRetriever->getScope() == FileInfoRetriever::List) {
        parentIndex = QModelIndex();
        parentItem = root;
    } else {

         parentIndex = parent(fileName);

        if (!parentIndex.isValid()) {
            qDebug() << "FileSystemModel::addPath" << fileInfoRetriever->getScope() << "couldn't find a valid parent for" << fileName;
            return;
        }

        parentItem = getFileSystemItem(parentIndex);
    }

    FileSystemItem *fileSystemItem = parentItem->getChild(fileName);
    if (fileSystemItem != nullptr) {
        qDebug() << "FileSystemModel::addPath" << fileInfoRetriever->getScope() << "we already have this path";
        return;
    }

    fileSystemItem = new FileSystemItem(fileName);
    fileInfoRetriever->setDisplayNameOf(fileSystemItem);

    bool result = fileInfoRetriever->refreshItem(fileSystemItem);

    // Skip items that failed (do not exist anymore) or files in TreeView (only folder/drives go there)
    if (!result ||
            (fileInfoRetriever->getScope() == FileInfoRetriever::Scope::Tree &&
            !fileSystemItem->isDrive() && !fileSystemItem->isFolder())) {
        qDebug() << "FileSystemModel::addPath" << fileInfoRetriever->getScope() << "skipping file" << fileName;
        delete fileSystemItem;
        return;
    }

    parentItem->addChild(fileSystemItem);
    parentItem->sortChildren(currentSortColumn, currentSortOrder);

    int row = parentItem->childRow(fileSystemItem);
    beginInsertRows(parentIndex, row, row);
    endInsertRows();

    qDebug() << "FileSystemModel::addPath" << fileInfoRetriever->getScope() << "row inserted for" << fileName;

    // If we were waiting for an item like this, tell the view the user has to set a new name for it
    if (watch && ((extensionBeingWatched == "NewFolder" && fileSystemItem->isFolder()) ||
                   extensionBeingWatched.right(extensionBeingWatched.size() - 1) == fileSystemItem->getExtension())) {

        QModelIndex itemIndex = createIndex(row, 0, fileSystemItem);
        emit shouldEdit(itemIndex);
        watch = false;
    }
}

void FileSystemModel::removePath(QString fileName)
{
    qDebug() << "FileSystemModel::removePath" << fileInfoRetriever->getScope() << fileName;

    FileSystemItem *parentItem;

    QPersistentModelIndex parentIndex;
    if (fileInfoRetriever->getScope() == FileInfoRetriever::List) {
        parentIndex = QModelIndex();
        parentItem = root;
    } else {

         parentIndex = parent(fileName);

        if (!parentIndex.isValid())
            return;

        parentItem = getFileSystemItem(parentIndex);
    }

    FileSystemItem *fileSystemItem = parentItem->getChild(fileName);
    if (fileSystemItem) {
        int row = parentItem->childRow(fileSystemItem);
        beginRemoveRows(parentIndex, row, row);
        parentItem->removeChild(fileName);
        delete fileSystemItem;
        endRemoveRows();
    }
}

