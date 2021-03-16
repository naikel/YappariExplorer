#ifndef WINSHELLACTIONS_H
#define WINSHELLACTIONS_H

#include "Shell/ShellActions.h"

class WinShellActions : public ShellActions
{
    Q_OBJECT

public:
    WinShellActions(QObject *parent = nullptr);

protected:
    void renameItemBackground(QUrl srcUrl, QString newName) override;
    void copyItemsBackground(QList<QUrl> srcUrls, QString dstPath) override;
    void moveItemsBackground(QList<QUrl> srcUrls, QString dstPath) override;
    void linkItemsBackground(QList<QUrl> srcUrls, QString dstPath) override;
    void removeItemsBackground(QList<QUrl> srcUrls, bool permanent) override;

private:

    enum Operation {
        Copy,
        Move,
        Delete
    };

    QString getUniqueLinkName(QString linkTo, QString destDir);
    void performFileOperations(QList<QUrl> srcUrls, QString dstPath, Operation op, bool permament = false);
};

#endif // WINSHELLACTIONS_H
