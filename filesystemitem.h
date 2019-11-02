#ifndef FILESYSTEMITEM_H
#define FILESYSTEMITEM_H

#include <QHash>
#include <QIcon>
#include <QString>
#include <QVariant>

class FileSystemItem
{
public:

    enum DataInfo {
        Name,
        Extension,
        Size,
        Type,
        MaxColumns
    };

    FileSystemItem(QString path);
    ~FileSystemItem();

    QString getDisplayName() const;
    void setDisplayName(const QString &value);

    void addChild(FileSystemItem *child);
    FileSystemItem *getChildAt(int n);
    FileSystemItem *getChild(QString path);
    QList<FileSystemItem *> getChildren();

    int childrenCount();
    int childRow(FileSystemItem *child);

    void sortChildren(int column, Qt::SortOrder order);

    FileSystemItem *getParent() const;
    void setParent(FileSystemItem *value);

    bool getHasSubFolders() const;
    void setHasSubFolders(bool value);

    bool areAllChildrenFetched() const;
    void setAllChildrenFetched(bool value);

    bool isDrive();

    QVariant getData(int column);

    QString getPath() const;
    void setPath(const QString &value);

    QIcon getIcon() const;
    void setIcon(const QIcon &value);

    bool isFolder() const;
    void setFolder(bool value);

    bool isHidden() const;
    void setHidden(bool value);

    bool hasFakeIcon() const;
    void setFakeIcon(bool value);

    QString getType() const;
    void setType(const QString &value);

    qint64 getSize() const;
    void setSize(const qint64 &value);

    QString getExtension() const;

private:
    QString path                {};
    QString displayName         {};
    QString extension           {};
    QIcon icon                  {};
    qint64 size                 { -1 };
    QString type                {};

    bool folder                 {false};
    bool hidden                 {false};
    bool hasSubFolders          {false};
    bool allChildrenFetched     {false};
    bool fakeIcon               {false};

    FileSystemItem *parent      {};

    // Children
    QHash<QString, FileSystemItem *> children;
    QList<FileSystemItem *> indexedChildren;

};

#endif // FILESYSTEMITEM_H
