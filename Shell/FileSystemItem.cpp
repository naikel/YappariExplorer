#include <QFileIconProvider>
#include <QApplication>
#include <QMimeDatabase>
#include <QLocale>
#include <QDebug>

#include "FileSystemItem.h"

FileSystemItem::FileSystemItem(QString path)
{
    this->path = path;
}

FileSystemItem::~FileSystemItem()
{
    removeChildren();
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

    if (!isFolder()) {
        QMimeDatabase mimeDatabase;
        QString suffix;
        try {
            suffix = mimeDatabase.suffixForFileName(value);
        }  catch (const std::exception& e) {
            qDebug() << "FileSystemItem::setDisplayName exception" << e.what();
        }

        if (suffix.isEmpty()) {
            int index = getDisplayName().lastIndexOf('.');
            suffix = (index > MIN_INDEX) ? getDisplayName().mid(++index) : QString();
        }
        extension = suffix;
    }
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

void FileSystemItem::removeChild(QString path)
{
    // This does not delete child, caller has to delete it.
    FileSystemItem *item = getChild(path);
    if (item) {
        children.remove(path);
        indexedChildren.removeOne(item);
    }
}

QList<FileSystemItem *> FileSystemItem::getChildren()
{
    return indexedChildren;
}

void FileSystemItem::removeChildren()
{
    for (FileSystemItem *item : qAsConst(indexedChildren))
        if (item->hasSubFolders)
            item->removeChildren();

    qDeleteAll(indexedChildren);
    clear();
}

void FileSystemItem::updateChildPath(FileSystemItem *child, QString path)
{
    QString oldPath = child->getPath();
    child->setPath(path);

    children.remove(oldPath);
    children.insert(path, child);
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

bool FileSystemItem::isDrive() const
{
    return (!path.isNull() && path.length() == 3 && path.at(0).isLetter() && path.at(1) == ':' && path.at(2) == '\\');
}

bool FileSystemItem::isInADrive() const
{
    return (!path.isNull() && path.length() >= 3 && path.at(0).isLetter() && path.at(1) == ':' && path.at(2) == '\\');
}

bool FileSystemItem::isEqualTo(FileSystemItem *item) const
{
    if (path != item->getPath())
        return false;

    if (displayName != item->getDisplayName() || size != item->getSize() || capabilities != item->getCapabilities())
        return false;

    if (creationTime != item->getCreationTime() || lastAccessTime != item->getLastAccessTime() || lastChangeTime != item->lastChangeTime)
        return false;

    if (folder != item->isFolder() || hasSubFolders != item->getHasSubFolders() || hidden != item->isHidden())
        return false;

    return true;
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

FileSystemItem *FileSystemItem::clone()
{
    FileSystemItem *item = new FileSystemItem(path);

    cloneTo(item);

    item->setParent(parent);

    return item;
}

void FileSystemItem::cloneTo(FileSystemItem *destination)
{
    destination->setDisplayName(displayName);
    destination->setIcon(icon);
    destination->setSize(size);
    destination->setType(type);
    destination->setCreationTime(creationTime);
    destination->setLastAccessTime(lastAccessTime);
    destination->setLastChangeTime(lastChangeTime);
    destination->setCapabilities(capabilities);
    destination->setMediaType(mediaType);

    destination->setFolder(folder);
    destination->setHidden(hidden);
    destination->setHasSubFolders(hasSubFolders);
    destination->setAllChildrenFetched(allChildrenFetched);
    destination->setFakeIcon(fakeIcon);
}

quint16 FileSystemItem::getCapabilities() const
{
    return capabilities;
}

void FileSystemItem::setCapabilities(const quint16 &value)
{
    capabilities = value;
}

FileSystemItem::MediaType FileSystemItem::getMediaType() const
{
    return mediaType;
}

void FileSystemItem::setMediaType(const MediaType &value)
{
    mediaType = value;
}

void FileSystemItem::clear()
{
    indexedChildren.clear();
    children.clear();
    setAllChildrenFetched(false);
}

qint32 FileSystemItem::getErrorCode() const
{
    return errorCode;
}

void FileSystemItem::setErrorCode(const qint32 &value)
{
    errorCode = value;
}

QString FileSystemItem::getErrorMessage() const
{
    return errorMessage;
}

void FileSystemItem::setErrorMessage(const QString &value)
{
    errorMessage = value;
}

bool FileSystemItem::getLock() const
{
    return lock;
}

void FileSystemItem::setLock(bool value)
{
    lock = value;
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
