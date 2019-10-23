#include "filesystemitem.h"

FileSystemItem::FileSystemItem(QString path)
{
    this->path = path;
}

FileSystemItem::~FileSystemItem()
{
    qDeleteAll(children);
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

FileSystemItem *FileSystemItem::getChild(QString path)
{
    return children.value(path);
}

int FileSystemItem::childrenCount()
{
    return children.size();
}

int FileSystemItem::childRow(FileSystemItem *child) {
    return indexedChildren.indexOf(child);
}

bool fileSystemItemCompare(FileSystemItem *i, FileSystemItem *j)
{
    Qt::SortOrder order = i->getParent()->getCurrentOrder();

    // Folders are first
    if (i->isFolder() && !j->isFolder())
        return true;

    // Files are second
    if (!i->isFolder() && !i->isDrive() && j->isDrive())
        return true;

    // Drives are third
    if (i->isDrive() && j->isDrive())
        return (order == Qt::AscendingOrder  && i->getPath().toLower() < j->getPath().toLower()) ||
               (order == Qt::DescendingOrder && i->getPath().toLower() > j->getPath().toLower());

    // If both items are files or both are folders then direct comparison is allowed
    if ((i->isFolder() && j->isFolder()) || (!i->isFolder() && !j->isFolder()))
        return (order == Qt::AscendingOrder  && i->getDisplayName().toLower() < j->getDisplayName().toLower()) ||
               (order == Qt::DescendingOrder && i->getDisplayName().toLower() > j->getDisplayName().toLower());

    return false;
}

void FileSystemItem::sortChildren(Qt::SortOrder order)
{
    setCurrentOrder(order);
    std::sort(indexedChildren.begin(), indexedChildren.end(), fileSystemItemCompare);
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

bool FileSystemItem::isFolder() const
{
    return folder;
}

void FileSystemItem::setFolder(bool value)
{
    folder = value;
}

Qt::SortOrder FileSystemItem::getCurrentOrder() const
{
    return currentOrder;
}

void FileSystemItem::setCurrentOrder(const Qt::SortOrder &value)
{
    currentOrder = value;
}

bool FileSystemItem::isHidden() const
{
    return hidden;
}

void FileSystemItem::setHidden(bool value)
{
    hidden = value;
}
