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

#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QAbstractItemModel>
#include <QMutex>

#include "Shell/FileSystemItem.h"
#include "Shell/FileInfoRetriever.h"
#include "Shell/ShellActions.h"
#include "Shell/DirectoryWatcher.h"

/*!
 * \brief FileSystemModel class.
 *
 * This is the filesystem model.
 *
 * This FileSystemModel has the following features:
 *
 * - Provides functions to rename and remove files and directories. \sa removeIndexes \sa setData
 *
 * - Smart icon handling. Icons are fetch on demand.
 *
 * - It uses a DirectoryWatcher to add or remove files and folders that are new to or removed from the filesystem.
 *
 */
class FileSystemModel : public QAbstractItemModel
{
    Q_OBJECT

public:

    /*!
     * \brief The Columns enum
     *
     * This enum is used to identify each column of the model.
     *
     * So far only 5 columns have been implemented.
     */
    enum Columns {
        Name,
        Extension,
        Size,
        Type,
        LastChangeTime,
        MaxColumns
    };

    enum Role {
        PathRole = Qt::UserRole,
        ExtensionRole,
        FileRole,
        DriveRole,
        FolderRole,
        HasSubFoldersRole,
        AllChildrenFetchedRole,
        CapabilitiesRole,
        ShouldEditRole,
        ErrorCodeRole,
        ErrorMessageRole,
        IncreaseRefCounterRole,
        DecreaseRefCounterRole,
        RefCounterRole
    };

    FileSystemModel(QObject *parent = nullptr);
    ~FileSystemModel() override;

    // Model reimplemented functions
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // Drag n' Drop
    Qt::DropActions supportedDropActions() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    QStringList mimeTypes() const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    // Custom functions
    void setDefaultRoot();
    QModelIndex index (FileSystemItem *item) const;
    QModelIndex index (QString path) const;
    QString getDropPath(QModelIndex index) const;
    Qt::DropActions supportedDragActionsForIndexes(QModelIndexList indexes);
    Qt::DropAction defaultDropActionForIndex(QModelIndex index, const QMimeData *data, Qt::DropActions possibleActions);
    QModelIndex relativeIndex(QString path, QModelIndex parent);
    void setRoot(const QString path);
    FileSystemItem *getRoot() const;
    bool removeAllRows(const QModelIndex &parent);
    QChar separator() const;
    QModelIndex parent(QString path) const;
    void removeIndexes(QModelIndexList indexList, bool permanent);
    void startWatch(const QModelIndex &parent, QString verb);
    bool willRecycle(const QModelIndex &index);
    QIcon getFolderIcon() const;

    // Inline functions
    inline FileSystemItem *getFileSystemItem(QModelIndex index) const {
        return static_cast<FileSystemItem *>(index.internalPointer());
    }

public slots:
    void refreshFolder(QString path);
    void refreshIndex(QModelIndex index);

private:

    FileInfoRetriever *fileInfoRetriever    {};
    DirectoryWatcher *watcher               {};
    bool watch                              {};
    FileSystemItem *parentBeingWatched      {};
    QString extensionBeingWatched           {};
    QList<FileSystemItem *> garbage         {};

    FileSystemItem *root                    {};
    ShellActions *shellActions              {};
    QList<QString> sizeUnits                {"byte", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};

    // Default icons
    QIcon driveIcon, fileIcon, folderIcon;

    QMutex garbageMutex;
    QMutex addMutex;

    QString humanReadableSize(quint64 size) const;

private slots:

    // These slots are called by the FileInfoRetriever object
    void parentInfoUpdated(FileSystemItem *parent);
    void parentChildrenUpdated(FileSystemItem *parent);
    void itemUpdated(FileSystemItem *item);
    void iconUpdated(FileSystemItem *item);

    // These slots are called by the DirectoryWatcher object
    void renamePath(QString oldFileName, QString newFileName);
    void refreshPath(QString fileName);
    void addPath(QString fileName);
    void removePath(QString fileName);

    // Other slots
    void garbageCollector();
};

#endif // FILESYSTEMMODEL_H
