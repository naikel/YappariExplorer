#include <QFileIconProvider>
#include <QDebug>
#include <QString>

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
        root = "G:\\";
#endif

    for(auto& path: fs::directory_iterator(root.toStdWString(), fs::directory_options::skip_permission_denied)) {

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
            qDebug() << "Path " << child->getPath() << " isDrive " << child->isDrive();

            // Set basic attributes
            child->setFolder(fs::is_directory(path));
            child->setHasSubFolders(hasSubFolders(path));
            child->setHidden(child->getPath().at(0) == '.');

            parent->addChild(child);
        }
    }

    parent->sortChildren(Qt::AscendingOrder);
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

QIcon UnixFileInfoRetriever::getIcon(FileSystemItem *item) const
{
    QFileIconProvider iconProvider;
    QString strPath = item->getPath();

    qDebug() << "UnixFileInfoRetriever::getIcon " << getScope() << strPath;

    // Set icon using QFileInfo
    QFileInfo fileInfo = QFileInfo(strPath);
    return iconProvider.icon(fileInfo);
}
