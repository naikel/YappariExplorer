#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QThreadPool>
#include <QAbstractItemModel>

#include "filesystemitem.h"
#include "fileinforetriever.h"

class FileSystemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
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

    QModelIndex relativeIndex(QString path, QModelIndex parent);
    void setRoot(const QString path);
    FileSystemItem *getRoot();

public slots:
    void parentUpdated(FileSystemItem *parent);

signals:
    void fetchStarted();
    void fetchFinished();

private:
    FileSystemItem *root {nullptr};

    QAtomicInt fetchingMore;
    QAtomicInt settingRoot;

    FileInfoRetriever *fileInfoRetriever {nullptr};

    // Default icons
    QIcon driveIcon, fileIcon, folderIcon;

    QThreadPool pool;

    void getIcon(const QModelIndex &index);

};

#endif // FILESYSTEMMODEL_H
