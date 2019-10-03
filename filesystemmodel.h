#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QAbstractItemModel>

#include "filesystemitem.h"

#ifdef Q_OS_WIN
#include "winfileinforetriever.h"
#else
#include "unixfileinforetriever.h"
#endif

class FileSystemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    FileSystemModel(QObject *parent = nullptr);
    ~FileSystemModel();

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

public Q_SLOTS:
    void parentUpdated(FileSystemItem *parent);

signals:
    void fetchStarted();
    void fetchFinished();

private:
    FileSystemItem *root;

#ifdef Q_OS_WIN
    WinFileInfoRetriever fileInfoRetriever;
#else
    UnixFileInfoRetriever fileInfoRetriever;
#endif
};

#endif // FILESYSTEMMODEL_H
