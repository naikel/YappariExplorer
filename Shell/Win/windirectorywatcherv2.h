#ifndef WINDIRECTORYWATCHERV2_H
#define WINDIRECTORYWATCHERV2_H

#include <qt_windows.h>
#include <fileapi.h>

#include <QMap>

#include "Shell/directorywatcher.h"

class WinDirectoryWatcherv2 : public DirectoryWatcher
{
    Q_OBJECT

public:
    WinDirectoryWatcherv2(QObject *parent = nullptr);
    ~WinDirectoryWatcherv2();

    void addPath(QString path) override;
    void removePath(QString path) override;
    bool handleNativeEvent(const QByteArray &eventType, void *message, long *result) override;

private:
    QMap<QString, int> watchedPaths;
    quint32 id;
};

#endif // WINDIRECTORYWATCHER_H
