#include <QFileIconProvider>
#include <QMimeDatabase>
#include <QDebug>
#include <QString>
#include <QTime>

#include "unixfileinforetriever.h"
#include "filesystemitem.h"

UnixFileInfoRetriever::UnixFileInfoRetriever(QObject *parent) : FileInfoRetriever(parent)
{
    qDebug() << QIcon::themeSearchPaths();
    //QIcon::setThemeName("HighContrast");
    QIcon::setThemeName("Adwaita");
}

void UnixFileInfoRetriever::getParentInfo(FileSystemItem *parent)
{
    QFileIconProvider iconProvider;

    qDebug() << "UnixFileInfoRetriever::getParentInfo " << getScope() << " Parent path " << parent->getPath();

    // If this is the root we need to retrieve the display name of it and its icon
    if (parent->getPath() == "/") {
        parent->setDisplayName("File System");
        parent->setHasSubFolders(true);
        parent->setIcon(iconProvider.icon(QFileIconProvider::Drive));

    } else {

        fs::path path = parent->getPath().toStdWString();
        parent->setDisplayName(QString::fromStdWString(path.filename().wstring()));
        parent->setHasSubFolders(hasSubFolders(path));

        // Set icon using QFileInfo
        QFileInfo fileInfo = QFileInfo(parent->getPath());
        parent->setIcon(iconProvider.icon(fileInfo));
    }
    qDebug() << "UnixFileInfoRetriever::getParentInfo " << getScope() << " Root name is " << parent->getDisplayName();

}

void UnixFileInfoRetriever::getChildrenBackground(FileSystemItem *parent)
{
    qDebug() << "UnixFileInfoRetriever::getChildrenBackground " << getScope();

    QString root = parent->getPath();

#ifdef Q_OS_WIN
    // This is for debugging the Unix implementation from Windows
    if (root == "/")
        root = "C:\\";
#endif

    for(auto& path: fs::directory_iterator(root.toStdWString(), fs::directory_options::skip_permission_denied)) {

        if (!running.load())
            break;

        if (getScope() == FileInfoRetriever::List || fs::is_directory(path)) {

            QString strPath = QString::fromStdWString(path.path().wstring());

#ifdef Q_OS_WIN
            // This is for debugging the Unix implementation from Windows
            strPath = strPath.replace('\\', '/');
#endif

            // Get the absolute path and create a FileSystemItem with it
            FileSystemItem *child = new FileSystemItem(strPath);

            // Get the display name
            child->setDisplayName(QString::fromStdWString(path.path().filename().wstring()));
            qDebug() << "UnixFileInfoRetriever::getChildrenBackground " << getScope() << child->getPath();

            // Set basic attributes
            child->setFolder(fs::is_directory(path));
            child->setHasSubFolders(hasSubFolders(path));
            child->setHidden(child->getPath().at(0) == '.');

            parent->addChild(child);
        }
    }

    parent->setAllChildrenFetched(true);
    emit parentUpdated(parent);

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

                fs::path path = fs::path(item->getPath().toStdWString().c_str());
                item->setSize(fs::file_size(path));
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
        for(auto& child: fs::directory_iterator(path)) {
            if (fs::is_directory(child))
                return true;
        }
    }
    catch (const std::exception& e) {
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
    QFileInfo fileInfo = QFileInfo(strPath);
    return iconProvider.icon(fileInfo);
}
