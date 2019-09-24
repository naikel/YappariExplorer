#include "filesystemitem.h"

FileSystemItem::FileSystemItem(QString path)
{
    this->path = path;
}

QString FileSystemItem::getDisplayName() const
{
    return displayName;
}

void FileSystemItem::setDisplayName(const QString &value)
{
    displayName = value;
}

void FileSystemItem::addChild(FileSystemItem *child)
{
    child->setParent(this);
    children.insert(child->path, child);

    for (int i = 0; i < indexedChildren.size() ; i++) {

        // If both children are drives or both are not drives then they can be compared
        if ((indexedChildren.at(i)->isDrive() && !child->isDrive()) ||
                (indexedChildren.at(i)->isDrive() && child->isDrive() && indexedChildren.at(i)->path.toLower() > child->path.toLower()) ||
                (!indexedChildren.at(i)->isDrive() && !child->isDrive() && indexedChildren.at(i)->displayName.toLower() > child->displayName.toLower())) {
            indexedChildren.insert(i, child);
            return;
        }
    }
    indexedChildren.append(child);
}

FileSystemItem *FileSystemItem::getParent() const
{
    return parent;
}

void FileSystemItem::setParent(FileSystemItem *value)
{
    parent = value;
}

FileSystemItem *FileSystemItem::getChildAt(int n)
{
    return indexedChildren.at(n);
}

int FileSystemItem::childrenCount()
{
    return children.size();
}

int FileSystemItem::childRow(FileSystemItem *child) {
    return indexedChildren.indexOf(child);
}

bool FileSystemItem::getHasSubFolders() const
{
    return hasSubFolders;
}

void FileSystemItem::setHasSubFolders(bool value)
{
    hasSubFolders = value;
}

bool FileSystemItem::areAllChildrenFetched() const
{
    return allChildrenFetched;
}

void FileSystemItem::setAllChildrenFetched(bool value)
{
    allChildrenFetched = value;
}

bool FileSystemItem::isDrive()
{
    return (path.length() == 3 && path.at(0).isLetter() && path.at(1) == ':' && path.at(2) == '\\');
}

QString FileSystemItem::getPath() const
{
    return path;
}

void FileSystemItem::setPath(const QString &value)
{
    path = value;
}

QIcon FileSystemItem::getIcon() const
{
    return icon;
}

void FileSystemItem::setIcon(const QIcon &value)
{
    icon = value;
}
