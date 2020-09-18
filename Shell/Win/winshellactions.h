#ifndef WINSHELLACTIONS_H
#define WINSHELLACTIONS_H

#include "Shell/shellactions.h"

class WinShellActions : public ShellActions
{
    Q_OBJECT

public:
    WinShellActions(QObject *parent = nullptr);

protected:
    void copyItemsBackground(QList<QUrl> srcUrls, QString dstPath) override;
    void moveItemsBackground(QList<QUrl> srcUrls, QString dstPath) override;
    void linkItemsBackground(QList<QUrl> srcUrls, QString dstPath) override;

private:
    QString getUniqueLinkName(QString linkTo, QString destDir);
    void performFileOperations(QList<QUrl> srcUrls, QString dstPath, bool copy);
};

#endif // WINSHELLACTIONS_H
