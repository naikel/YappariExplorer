#ifndef WINSHELLACTIONS_H
#define WINSHELLACTIONS_H

#include "Shell/shellactions.h"

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
    void removeItemsBackground(QList<QUrl> srcUrls) override;

private:

    enum Operation {
        Copy,
        Move,
        Delete
    };

    QString getUniqueLinkName(QString linkTo, QString destDir);
    void performFileOperations(QList<QUrl> srcUrls, QString dstPath, Operation op);
};

#endif // WINSHELLACTIONS_H
