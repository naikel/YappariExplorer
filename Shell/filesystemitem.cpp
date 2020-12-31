#include <QFileIconProvider>
#include <QApplication>
#include <QMimeDatabase>
#include <QCollator>
#include <QLocale>
#include <QDebug>

#ifdef PARALLEL
#include <parallel/algorithm>
#endif

#include "filesystemitem.h"

#include <unicode/coll.h>

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
    return children.values();
}

void FileSystemItem::removeChildren()
{
    qDeleteAll(indexedChildren);
    children.clear();
    indexedChildren.clear();
    setAllChildrenFetched(false);
    setHasSubFolders(false);
}

int FileSystemItem::childrenCount()
{
    return children.size();
}

int FileSystemItem::childRow(FileSystemItem *child) {
    return indexedChildren.indexOf(child);
}

bool fileSystemItemCompare(const QCollator& collator, FileSystemItem *i, FileSystemItem *j, int column, Qt::SortOrder order)
{
    bool comparison;

    // Folders are first
    if (i->isFolder() && !j->isFolder())
        return true;

    // Files are second
    if (!i->isFolder() && !i->isDrive() && j->isDrive())
        return true;

    // Drives are third
    if (i->isDrive() && j->isDrive()) {

        comparison = (collator.compare(i->getPath(), j->getPath()) < 0);

        return (order == Qt::AscendingOrder  && comparison) ||
               (order == Qt::DescendingOrder && !comparison);
    }

    QVariant left  = i->getData(column);
    QVariant right = j->getData(column);

    if (static_cast<QMetaType::Type>(left.type()) == QMetaType::QString)
        comparison = (collator.compare(left.toString(), right.toString()) < 0);
    else
        comparison = (left < right);

    // If both items are files or both are folders then direct comparison is allowed
    if ((!i->isFolder() && !j->isFolder()) || (i->isFolder() && j->isFolder())) {
        if (left == right) {
            comparison = (collator.compare(i->getDisplayName(), j->getDisplayName()) < 0);
        }

        return (order == Qt::AscendingOrder  && comparison) ||
               (order == Qt::DescendingOrder && !comparison);
    }

    return false;
}

bool fileSystemItemCompareicu(icu::Collator *coll, FileSystemItem *i, FileSystemItem *j, int column, Qt::SortOrder order)
{
    bool comparison;

    // Folders are first
    if (i->isFolder() && !j->isFolder())
        return true;

    // Files are second
    if (!i->isFolder() && !i->isDrive() && j->isDrive())
        return true;

    // Drives are third
    if (i->isDrive() && j->isDrive()) {

        i->getPath().utf16();
        comparison = (coll->compare(reinterpret_cast<const char16_t *>(i->getPath().utf16()), i->getPath().length(),
                                    reinterpret_cast<const char16_t *>(j->getPath().utf16()), j->getPath().length()) < 0);

        return (order == Qt::AscendingOrder  && comparison) ||
               (order == Qt::DescendingOrder && !comparison);
    }

    QVariant left  = i->getData(column);
    QVariant right = j->getData(column);

    if (static_cast<QMetaType::Type>(left.type()) == QMetaType::QString) {

        QString strLeft = left.toString();
        QString strRight = right.toString();

        comparison = (coll->compare(reinterpret_cast<const char16_t *>(strLeft.utf16()), strLeft.length(),
                                    reinterpret_cast<const char16_t *>(strRight.utf16()), strRight.length()) < 0);
    } else
        comparison = (left < right);

    // If both items are files or both are folders then direct comparison is allowed
    if ((!i->isFolder() && !j->isFolder()) || (i->isFolder() && j->isFolder())) {
        if (left == right) {
            comparison = (coll->compare(reinterpret_cast<const char16_t *>(i->getDisplayName().utf16()), i->getDisplayName().length(),
                                        reinterpret_cast<const char16_t *>(j->getDisplayName().utf16()), j->getDisplayName().length()) < 0);
        }

        return (order == Qt::AscendingOrder  && comparison) ||
               (order == Qt::DescendingOrder && !comparison);
    }

    return false;
}

void FileSystemItem::sortChildren(int column, Qt::SortOrder order)
{
    QTime start;
    start.start();
    qDebug() << "FileSystemItem::sortChildren Started sorting";

#ifndef ICU_COLLATOR
    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);

#ifdef PARALLEL
    __gnu_parallel::sort(indexedChildren.begin(), indexedChildren.end(), [&collator, column, order](FileSystemItem *i, FileSystemItem *j) { return fileSystemItemCompare(collator, i, j, column, order); },
                         __gnu_parallel::multiway_mergesort_tag());
#else
    std::sort(indexedChildren.begin(), indexedChildren.end(), [&collator, column, order](FileSystemItem *i, FileSystemItem *j) { return fileSystemItemCompare(collator, i, j, column, order); } );
#endif

#else
    UErrorCode status = U_ZERO_ERROR;
    icu::Collator *coll = icu::Collator::createInstance(icu::Locale::getDefault(), status);
    coll->setAttribute(UCOL_NUMERIC_COLLATION, UCOL_ON, status);
    std::sort(indexedChildren.begin(), indexedChildren.end(), [coll, column, order](FileSystemItem *i, FileSystemItem *j) { return fileSystemItemCompareicu(coll, i, j, column, order); } );
    delete coll;
#endif
    qDebug() << "FileSystemItem::sortChildren Finished in" << start.elapsed() << "milliseconds";
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

    item->setDisplayName(displayName);
    item->setIcon(icon);
    item->setSize(size);
    item->setType(type);
    item->setCreationTime(creationTime);
    item->setLastAccessTime(lastAccessTime);
    item->setLastChangeTime(lastChangeTime);

    item->setFolder(folder);
    item->setHidden(hidden);
    item->setHasSubFolders(hasSubFolders);
    item->setAllChildrenFetched(allChildrenFetched);
    item->setFakeIcon(fakeIcon);

    item->setParent(parent);

    return item;
}

quint16 FileSystemItem::getCapabilities() const
{
    return capabilities;
}

void FileSystemItem::setCapabilities(const quint16 &value)
{
    capabilities = value;
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
