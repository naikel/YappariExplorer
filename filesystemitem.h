#ifndef FILESYSTEMITEM_H
#define FILESYSTEMITEM_H

#include <QHash>
#include <QIcon>
#include <QString>

class FileSystemItem
{
public:
    FileSystemItem(QString path);
    ~FileSystemItem();

    QString getDisplayName() const;
    void setDisplayName(const QString &value);
    void addChild(FileSystemItem *child);
    FileSystemItem *getParent() const;
    void setParent(FileSystemItem *value);
    FileSystemItem *getChildAt(int n);
    FileSystemItem *getChild(QString path);
    int childrenCount();
    int childRow(FileSystemItem *child);

    void sortChildren(Qt::SortOrder order);

    bool getHasSubFolders() const;
    void setHasSubFolders(bool value);

    bool areAllChildrenFetched() const;
    void setAllChildrenFetched(bool value);

    bool isDrive();

    QString getPath() const;
    void setPath(const QString &value);

    QIcon getIcon() const;
    void setIcon(const QIcon &value);

    bool isFolder() const;
    void setFolder(bool value);

    Qt::SortOrder getCurrentOrder() const;
    void setCurrentOrder(const Qt::SortOrder &value);

    bool isHidden() const;
    void setHidden(bool value);

    bool getFakeIcon() const;
    void setFakeIcon(bool value);

private:
    QString path                {};
    QString displayName         {};
    QIcon icon                  {};
    Qt::SortOrder currentOrder  {Qt::AscendingOrder};

    bool folder                 {false};
    bool hidden                 {false};
    bool hasSubFolders          {false};
    bool allChildrenFetched     {false};
    bool fakeIcon              {false};

    FileSystemItem *parent      {};
    QHash<QString, FileSystemItem *> children;
    QList<FileSystemItem *> indexedChildren;

};

#endif // FILESYSTEMITEM_H
