#ifndef WINDIRECTORYWATCHER_H
#define WINDIRECTORYWATCHER_H

#include <qt_windows.h>
#include <fileapi.h>

#include <QMap>

#include "Shell/DirectoryWatcher.h"
#include "Shell/Win/WinDirChangeNotifier.h"

class WinDirectoryWatcher : public DirectoryWatcher
{
    Q_OBJECT

public:
    WinDirectoryWatcher(QObject *parent = nullptr);
    ~WinDirectoryWatcher();

    void addPath(QString path) override;
    void removePath(QString path) override;
    bool handleNativeEvent(const QByteArray &eventType, void *message, long *result) override;

private:
    QMap<QString, int> watchedPaths;
    quint32 id;
    WinDirChangeNotifier *dirChangeNotifier;
};

#endif // WINDIRECTORYWATCHER_H
