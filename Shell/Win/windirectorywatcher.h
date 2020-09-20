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

private:
    QString path        {};
    HANDLE handle       {};
    HANDLE event        {};
    bool stopRequested  {};
};

#endif // WINDIRECTORYWATCHER_H
