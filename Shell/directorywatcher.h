#ifndef DIRECTORYWATCHER_H
#define DIRECTORYWATCHER_H

#include <QObject>

class DirectoryWatcher : public QObject
{
    Q_OBJECT
public:
    DirectoryWatcher(QObject *parent = nullptr);

    virtual void addPath(QString path);
    virtual void removePath(QString path);
    virtual bool handleNativeEvent(const QByteArray &eventType, void *message, long *result);

signals:
    void fileRename(QString oldFileName, QString newFileName);
    void fileModified(QString fileName);
    void fileAdded(QString fileName);
    void fileRemoved(QString fileName);
};

#endif // DIRECTORYWATCHER_H
