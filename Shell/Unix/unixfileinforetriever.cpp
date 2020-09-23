#include <QFileIconProvider>
#include <QMimeDatabase>
#include <QDateTime>
#include <QPainter>
#include <QDebug>
#include <QString>
#include <QTime>

#include <sys/stat.h>

#include "Shell/Unix/unixfileinforetriever.h"
#include "Shell/filesystemitem.h"

#ifdef Q_OS_WIN
// This is for debugging the Unix implementation from Windows
#define QString_fromStdString(path)             QString::fromStdWString(path.wstring())
#define QString_toStdString(str)                str.toStdWString()
#define QString_toCString(str)                  str.toStdString().c_str()
#else
// POSIX systems are already Unicode aware, and attempts to convert to wstring will end up in error anyway
#define QString_fromStdString(path)             QString::fromStdString(path.string())
#define QString_toStdString(str)                str.toStdString()
#define QString_toCString(str)                  str.toStdString().c_str()
#endif

UnixFileInfoRetriever::UnixFileInfoRetriever(QObject *parent) : FileInfoRetriever(parent)
{
}

bool UnixFileInfoRetriever::getParentInfo(FileSystemItem *parent)
{
    QFileIconProvider iconProvider;

    qDebug() << "UnixFileInfoRetriever::getParentInfo" << getScope() << "Parent path" << parent->getPath();

    // If this is the root we need to retrieve the display name of it and its icon
    if (parent->getPath() == "/") {
        parent->setDisplayName("File System");
        parent->setHasSubFolders(true);
    } else {
        std::error_code ec;
        fs::path path = QString_toStdString(parent->getPath());
        if (fs::exists(path), &ec) {
            parent->setDisplayName(QString_fromStdString(path.filename()));
            parent->setHasSubFolders(hasSubFolders(path));
        } else {
            QString errMessage = QString::fromStdString(ec.message());
            qint32 err = ec.value();

            qDebug() << "UnixFileInfoRetriever::getParentInfo" << getScope() << "Couldn't access" << parent->getPath() << "error_code" << err << "(" << errMessage << ")";
            emit parentUpdated(parent, err, errMessage);
            return false;
        }
    }
    parent->setIcon(getIcon(parent));
    qDebug() << "UnixFileInfoRetriever::getParentInfo" << getScope() << "Root name is " << parent->getDisplayName();

    return true;
}

void UnixFileInfoRetriever::getChildrenBackground(FileSystemItem *parent)
{
    qDebug() << "UnixFileInfoRetriever::getChildrenBackground " << getScope();

    // Sometimes the last folder is deleted between getParentInfo and this function
    // We need to double check it
    bool subFolders {};

    QString root = parent->getPath();

#ifdef Q_OS_WIN
    // This is for debugging the Unix implementation from Windows
    if (root == "/")
        root = "C:\\";
#endif

    for(auto& path: fs::directory_iterator(QString_toStdString(root), fs::directory_options::skip_permission_denied)) {

        if (!running.load())
            break;

        if (getScope() == FileInfoRetriever::List || fs::is_directory(path)) {

            QString strPath = QString_fromStdString(path.path());

#ifdef Q_OS_WIN
            // This is for debugging the Unix implementation from Windows
            strPath = strPath.replace('\\', '/');
#endif
            // Get the absolute path and create a FileSystemItem with it
            FileSystemItem *child = new FileSystemItem(strPath);

            // Get the display name
            child->setDisplayName(QString_fromStdString(path.path().filename()));
            qDebug() << "UnixFileInfoRetriever::getChildrenBackground" << getScope() << child->getPath();

            // Set basic attributes
            bool isDirectory = fs::is_directory(path);
            child->setFolder(isDirectory);
            if (isDirectory) {
                qDebug() << "UnixFileInfoRetriever::getChildrenBackground" << child->getPath() << "is a directory";
                if (!subFolders && isDirectory)
                    subFolders = true;
            }
            child->setHasSubFolders(isDirectory ? hasSubFolders(path) : false);
            child->setHidden(child->getDisplayName().at(0) == '.');

            struct stat buffer {};
            stat(QString_toCString(strPath), &buffer);
            child->setSize(static_cast<quint64>(buffer.st_size));
            child->setLastChangeTime(QDateTime::fromTime_t(static_cast<uint>(buffer.st_mtime)));
            child->setLastAccessTime(QDateTime::fromTime_t(static_cast<uint>(buffer.st_atime)));

            parent->addChild(child);
        }
    }

    parent->setHasSubFolders(subFolders);
    parent->setAllChildrenFetched(true);
    emit parentUpdated(parent, 0, QString());

    if (getScope() == FileInfoRetriever::List)
         getExtendedInfo(parent);

    running.store(false);
}

QString toTitleCase(QString str)
{
    QString result;
    QStringList strList = str.split(' ');
    for (auto word : strList) {
        if (!result.isEmpty())
            result += ' ';
        result += word.replace(0, 1, word.at(0).toUpper());
    }
    return result;
}

void UnixFileInfoRetriever::getExtendedInfo(FileSystemItem *parent)
{
    qDebug() << "UnixFileInfoRetriever::getExtendedInfo " << getScope() << " Parent path " << parent->getPath();

    QTime start;
    start.start();

    QMimeDatabase mimeDatabase;

    if (parent != nullptr && parent->areAllChildrenFetched()) {

        for (auto item : parent->getChildren()) {

            if (!running.load())
                break;

            if (!item->isFolder()) {

                QList<QMimeType> mimeList = mimeDatabase.mimeTypesForFileName(item->getDisplayName());
                if (mimeList.size() > 0)
                    item->setType(toTitleCase(mimeList.at(0).comment()));
                else {
                    QString strType;
                    if (!item->getExtension().isEmpty())
                        strType = item->getExtension().toUpper() + ' ';

                    item->setType(strType + tr("File"));
                }
            }

            emit itemUpdated(item);
        }

        if (!running.load()) {
            qDebug() << "UnixFileInfoRetriever::getExtendedInfo " << getScope() << "Parent path " << parent->getPath() << " aborted!";
            return;
        }

        // We need to signal the model that all the children need to be re-sorted
        emit (extendedInfoUpdated(parent));
    }

    qDebug() << "UnixFileInfoRetriever::getExtendedInfo Finished in" << start.elapsed() << "milliseconds";
}

bool UnixFileInfoRetriever::hasSubFolders(fs::path path)
{
    if (!fs::is_directory(path))
        return false;

    try {
        for (auto& child: fs::directory_iterator(path)) {
            if (fs::is_directory(child))
                return true;
        }
    } catch (const std::exception& e) {
        qDebug() << "Exception while trying to read folder: " << e.what();
    }

    return false;
}

QIcon UnixFileInfoRetriever::getIcon(FileSystemItem *item) const
{
    QFileIconProvider iconProvider;
    QString strPath = item->getPath();

    qDebug() << "UnixFileInfoRetriever::getIcon " << getScope() << strPath;

    // Set icon using QFileInfo
    QIcon icon;
    if (strPath == "/")
        icon = iconProvider.icon(QFileIconProvider::Drive);
    else {
        QFileInfo fileInfo = QFileInfo(strPath);
        icon = iconProvider.icon(fileInfo);
    }

#ifdef Q_OS_WIN
    // This is for debugging the Unix implementation from Windows
    return icon;
#else
    // This probably would be part of the configuration
    // TODO: Check QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
    QPixmap pixmap = icon.pixmap(16);

    if (item->isHidden()) {

        // The ghosted icon for hidden files is not provided by the OS
        // Here's an alpha blend implementation using Qt objects only

        if (!pixmap.isNull()) {

            QImage image = pixmap.toImage();
            QPainter paint(&pixmap);
            paint.setCompositionMode(QPainter::CompositionMode_Clear);
            paint.setOpacity(0.5);
            paint.drawImage(0, 0, image);
        }
    }

    return QIcon(pixmap);
#endif
}

