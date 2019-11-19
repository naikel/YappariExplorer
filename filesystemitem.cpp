#include <QApplication>
#include <QMimeDatabase>
#include <QCollator>
#include <QDebug>

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
#ifdef Q_OS_WIN
#define MIN_INDEX -1
#else
#define MIN_INDEX 0
#endif

    displayName = value;

    QMimeDatabase mimeDatabase;
    QString suffix = mimeDatabase.suffixForFileName(value);
    if (suffix.isEmpty()) {
        int index = getDisplayName().lastIndexOf('.');
        suffix = (index > MIN_INDEX) ? getDisplayName().mid(++index) : QString();
    }
    extension = suffix;
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

QList<FileSystemItem *> FileSystemItem::getChildren()
{
    return children.values();
}

int FileSystemItem::childrenCount()
{
    return children.size();
}

int FileSystemItem::childRow(FileSystemItem *child) {
    return indexedChildren.indexOf(child);
}

bool fileSystemItemCompare(FileSystemItem *i, FileSystemItem *j, int column, Qt::SortOrder order)
{
    QCollator collator;
    bool comparison;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);

    // Folders are first
    if (i->isFolder() && !j->isFolder())
        return true;

    // Files are second
    if (!i->isFolder() && !i->isDrive() && j->isDrive())
        return true;

    // Drives are third
    if (i->isDrive() && j->isDrive()) {

        comparison = (collator.compare(i->getPath(), j->getPath()) <= 0);

        return (order == Qt::AscendingOrder  && comparison) ||
               (order == Qt::DescendingOrder && !comparison);
    }

    QVariant left  = i->getData(column);
    QVariant right = j->getData(column);

    if (static_cast<QMetaType::Type>(left.type()) == QMetaType::QString)
        comparison = (collator.compare(left.toString(), right.toString()) <= 0);
    else
        comparison = (left <= right);

    // If both items are files or both are folders then direct comparison is allowed
    if ((!i->isFolder() && !j->isFolder()) || (i->isFolder() && j->isFolder())) {
        if (left == right)
            comparison = (collator.compare(i->getDisplayName(), j->getDisplayName()) < 0);

        return (order == Qt::AscendingOrder  && comparison) ||
               (order == Qt::DescendingOrder && !comparison);
    }

    return false;
}

void FileSystemItem::sortChildren(int column, Qt::SortOrder order)
{
    std::sort(indexedChildren.begin(), indexedChildren.end(), [column, order](FileSystemItem *i, FileSystemItem *j) { return fileSystemItemCompare(i, j, column, order); } );
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

bool FileSystemItem::isDrive() const
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
    if (folder)
        extension = QString();
}

bool FileSystemItem::isHidden() const
{
    return hidden;
}

void FileSystemItem::setHidden(bool value)
{
    hidden = value;
}

bool FileSystemItem::hasFakeIcon() const
{
    return fakeIcon;
}

void FileSystemItem::setFakeIcon(bool value)
{
    fakeIcon = value;
}

QString FileSystemItem::getType() const
{
    return type;
}

void FileSystemItem::setType(const QString &value)
{
    type = value;
}

quint64 FileSystemItem::getSize() const
{
    return size;
}

void FileSystemItem::setSize(const quint64 &value)
{
    size = value;
}

QString FileSystemItem::getExtension() const
{
    return extension;
}

QDateTime FileSystemItem::getCreationTime() const
{
    return creationTime;
}

void FileSystemItem::setCreationTime(const QDateTime &value)
{
    creationTime = value;
}

QDateTime FileSystemItem::getLastAccessTime() const
{
    return lastAccessTime;
}

void FileSystemItem::setLastAccessTime(const QDateTime &value)
{
    lastAccessTime = value;
}

QDateTime FileSystemItem::getLastChangeTime() const
{
    return lastChangeTime;
}

void FileSystemItem::setLastChangeTime(const QDateTime &value)
{
    lastChangeTime = value;
}

QVariant FileSystemItem::getData(int column)
{
    DataInfo dataInfo = static_cast<DataInfo>(column);

    switch (dataInfo) {
    case DataInfo::Name:
        return QVariant(getDisplayName());
        case DataInfo::Size:
            return QVariant(getSize());
        case DataInfo::Extension:
            return QVariant(getExtension());
        case DataInfo::Type:
            return QVariant(getType());
        case DataInfo::LastChangeTime:
            return QVariant(getLastChangeTime());
        default:
            return QVariant();
    }
}
