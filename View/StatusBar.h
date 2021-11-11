#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>

class StatusBar : public QStatusBar
{
    Q_OBJECT
public:
    StatusBar(QWidget *parent = nullptr);
};

#endif // STATUSBAR_H
