#ifndef WINSHELLACTIONS_H
#define WINSHELLACTIONS_H

#include "Shell/shellactions.h"

class WinShellActions : public ShellActions
{
    Q_OBJECT

public:
    WinShellActions(QObject *parent = nullptr);
};

#endif // WINSHELLACTIONS_H
