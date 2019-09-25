#include <QFileIconProvider>
#include <QDebug>
#include <QString>

#include "unixfileinforetriever.h"
#include "filesystemitem.h"

UnixFileInfoRetriever::UnixFileInfoRetriever(QObject *parent) : FileInfoRetriever(parent)
{

}

void UnixFileInfoRetriever::getChildrenBackground(FileSystemItem *parent)
{
    QFileIconProvider iconProvider;
    qDebug() << "getChildrenBackground()";

    // If this is the root we need to retrieve the display name of it and its icon
    if (parent->getPath() == "/") {
        parent->setDisplayName("File System");
        parent->setHasSubFolders(true);
        parent->setIcon(iconProvider.icon(QFileIconProvider::Drive));
    }

    for(auto& path: fs::directory_iterator(parent->getPath().toStdWString(), fs::directory_options::skip_permission_denied)) {

        if (fs::is_directory(path)) {

            QString strPath = QString::fromStdWString(path.path().wstring());

#ifdef Q_OS_WIN
            // This is for debugging the Unix implementation from Windows
            strPath = strPath.replace('\\', '/');
#endif

            // Get the absolute path and create a FileSystemItem with it
            FileSystemItem *child = new FileSystemItem(strPath);

            // Get the display name
            child->setDisplayName(QString::fromStdWString(path.path().filename().wstring()));
            qDebug() << "Path " << child->getPath() << " isDrive " << child->isDrive();

            // Set basic attributes
            child->setHasSubFolders(hasSubFolders(path));

#ifdef Q_OS_WIN
            // This is for debugging the Unix implementation from Window
            strPath = "C:" + strPath;
#endif

            // Set icon using QFileInfo
            QFileInfo fileInfo = QFileInfo(strPath);
            child->setIcon(iconProvider.icon(fileInfo));
            parent->addChild(child);
        }
    }

    parent->setAllChildrenFetched(true);
    emit parentUpdated(parent);
}

bool UnixFileInfoRetriever::hasSubFolders(fs::path path)
{
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
