#ifndef SHELLACTIONS_H
#define SHELLACTIONS_H

#include <QObject>

class ShellActions : public QObject
{
    Q_OBJECT

public:
    ShellActions(QObject *parent = nullptr);
};

#endif // SHELLACTIONS_H
