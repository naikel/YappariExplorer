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
    int childrenCount();
    int childRow(FileSystemItem *child);

    bool getHasSubFolders() const;
    void setHasSubFolders(bool value);

    bool areAllChildrenFetched() const;
    void setAllChildrenFetched(bool value);

    bool isDrive();

    QString getPath() const;
    void setPath(const QString &value);

    QIcon getIcon() const;
    void setIcon(const QIcon &value);

private:
    QString path                {};
    QString displayName         {};
    QIcon icon;

    bool hasSubFolders          {false};
    bool allChildrenFetched     {false};

    FileSystemItem *parent      {};
    QHash<QString, FileSystemItem *> children;
    QList<FileSystemItem *> indexedChildren;
};

#endif // FILESYSTEMITEM_H
