#include <QDebug>

#include "SortModel.h"

#include "Model/FileSystemModel.h"

SortModel::SortModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    UErrorCode status = U_ZERO_ERROR;
    coll = icu::Collator::createInstance(icu::Locale::getDefault(), status);
    coll->setAttribute(UCOL_NUMERIC_COLLATION, UCOL_ON, status);
}

SortModel::~SortModel()
{
    if (coll != nullptr)
        delete coll;
}

bool SortModel::willRecycle(const QModelIndex &index)
{
    FileSystemModel *model = reinterpret_cast<FileSystemModel *>(sourceModel());
    return model->willRecycle(mapToSource(index));
}

void SortModel::removeIndexes(QModelIndexList indexList, bool permanent)
{
    QModelIndexList sourceIndexList;
    for (QModelIndex index : indexList)
        sourceIndexList.append(mapToSource(index));
    FileSystemModel *model = reinterpret_cast<FileSystemModel *>(sourceModel());
    model->removeIndexes(sourceIndexList, permanent);
}

bool SortModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    bool comparison;
    bool result = (sortOrder() == Qt::AscendingOrder);

    FileSystemItem *i = reinterpret_cast<FileSystemItem *>(source_left.internalPointer());
    FileSystemItem *j = reinterpret_cast<FileSystemItem *>(source_right.internalPointer());

    // Folders are first
    if (i->isFolder() && !j->isFolder())
        return result;

    // Files are second
    if (!i->isFolder() && !i->isDrive() && j->isDrive())
        return result;

    // Drives are third
    if (i->isDrive() && j->isDrive()) {

        i->getPath().utf16();
        comparison = (coll->compare(reinterpret_cast<const char16_t *>(i->getPath().utf16()), i->getPath().length(),
                                    reinterpret_cast<const char16_t *>(j->getPath().utf16()), j->getPath().length()) < 0);

        return comparison;
    }

    int column = sortColumn();
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

        return comparison;
    }

    return false;
}

Qt::DropAction SortModel::defaultDropActionForIndex(QModelIndex index, const QMimeData *data, Qt::DropActions possibleActions)
{
    FileSystemModel *model = reinterpret_cast<FileSystemModel *>(sourceModel());
    return model->defaultDropActionForIndex(mapToSource(index), data, possibleActions);
}
