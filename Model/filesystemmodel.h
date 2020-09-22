#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QAbstractItemModel>

#include "Shell/filesystemitem.h"
#include "Shell/fileinforetriever.h"
#include "Shell/shellactions.h"

#ifdef Q_OS_WIN
#   include "Shell/Win/windirectorywatcher.h"
#   define PlatformFileSystemWatcher() WinFileSystemWatcher()
#endif

class FileSystemModel : public QAbstractItemModel
{
    Q_OBJECT

public:

    enum Columns {
        Name,
        Extension,
        Size,
        Type,
        LastChangeTime,
        MaxColumns
    };

    FileSystemModel(FileInfoRetriever::Scope scope, QObject *parent = nullptr);
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
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDropActions() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    QStringList mimeTypes() const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    // Custom functions
    QString getDropPath(QModelIndex index) const;
    Qt::DropActions supportedDragActionsForIndexes(QModelIndexList indexes);
    Qt::DropAction defaultDropActionForIndex(QModelIndex index, const QMimeData *data, Qt::DropActions possibleActions);
    QModelIndex relativeIndex(QString path, QModelIndex parent);
    bool setRoot(const QString path);
    FileSystemItem *getRoot() const;
    bool removeAllRows(QModelIndex &parent);
    QChar separator() const;

    // Inline functions
    inline FileSystemItem *getFileSystemItem(QModelIndex index) const {
        return static_cast<FileSystemItem *>(index.internalPointer());
    }

public slots:
    void parentUpdated(FileSystemItem *parent, qint32 err, QString errMessage);
    void itemUpdated(FileSystemItem *item);
    void extendedInfoUpdated(FileSystemItem *parent);

signals:
    void fetchStarted();
    void fetchFinished();
    void fetchFailed(qint32 err, QString errMessage);

private:

    WinDirectoryWatcher *watcher            {};

    QAtomicInt fetchingMore;
    QAtomicInt settingRoot;

    FileSystemItem *root                    {nullptr};
    int currentSortColumn                   {0};
    Qt::SortOrder currentSortOrder          {Qt::AscendingOrder};
    FileInfoRetriever *fileInfoRetriever    {nullptr};
    ShellActions *shellActions              {nullptr};
    QList<QString> sizeUnits                {"byte", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};

    // Default icons
    QIcon driveIcon, fileIcon, folderIcon;

    QThreadPool pool;

    void getIcon(const QModelIndex &index);
    QString humanReadableSize(quint64 size) const;

private slots:
    void renameItem(QString oldFileName, QString newFileName);
};

#endif // FILESYSTEMMODEL_H
