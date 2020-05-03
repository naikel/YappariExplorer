#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QAbstractItemModel>

#include "Shell/filesystemitem.h"
#include "Shell/fileinforetriever.h"

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

    // Model functions
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
    QStringList mimeTypes() const override;
    QModelIndex relativeIndex(QString path, QModelIndex parent);
    void setRoot(const QString path);
    FileSystemItem *getRoot();
    QChar separator();

public slots:
    void parentUpdated(FileSystemItem *parent);
    void itemUpdated(FileSystemItem *item);
    void extendedInfoUpdated(FileSystemItem *parent);

signals:
    void fetchStarted();
    void fetchFinished();

private:
    QAtomicInt fetchingMore;
    QAtomicInt settingRoot;

    FileSystemItem *root                    {nullptr};
    int currentSortColumn                   {0};
    Qt::SortOrder currentSortOrder          {Qt::AscendingOrder};
    FileInfoRetriever *fileInfoRetriever    {nullptr};
    QList<QString> sizeUnits                {"byte", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};

    // Default icons
    QIcon driveIcon, fileIcon, folderIcon;

    QThreadPool pool;

    void getIcon(const QModelIndex &index);

    QString humanReadableSize(quint64 size) const;
};

#endif // FILESYSTEMMODEL_H
