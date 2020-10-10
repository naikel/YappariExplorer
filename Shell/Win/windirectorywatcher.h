#ifndef WINDIRECTORYWATCHER_H
#define WINDIRECTORYWATCHER_H

#include <qt_windows.h>
#include <fileapi.h>

#include <QThread>

class WinDirectoryWatcher : public QThread
{
    Q_OBJECT

public:
    WinDirectoryWatcher(QString path, QObject *parent = nullptr);
    ~WinDirectoryWatcher();
    void run() override;
    void stop();

signals:
    void fileRename(QString oldFileName, QString newFileName);
    void fileModified(QString fileName);
    void fileAdded(QString fileName);
    void fileRemoved(QString fileName);

private:
    QString path        {};
    HANDLE handle       {};
    HANDLE event        {};
    bool stopRequested  {};
};

#endif // WINDIRECTORYWATCHER_H
