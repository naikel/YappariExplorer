#ifndef FILESYSTEMHISTORY_H
#define FILESYSTEMHISTORY_H

#include <QList>

class FileSystemHistory : QObject
{
public:
    FileSystemHistory(QObject *parent = nullptr);

    bool isCursorAtTheEnd();
    void insert(QString path);

    bool canGoForward();
    bool canGoBack();

    QString getLastItem();
    QString getNextItem();

private:
    QList<QString> pathList;
    int cursor { -1 };

};

#endif // FILESYSTEMHISTORY_H
