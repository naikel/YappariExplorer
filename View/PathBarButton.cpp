#include <QMouseEvent>
#include <QDebug>
#include <QMenu>

#include "PathBarButton.h"

PathBarButton::PathBarButton(QWidget *parent) : QPushButton(parent)
{
    menuTimer.setInterval(500);
    connect(&menuTimer, &QTimer::timeout, this, &PathBarButton::showMenu);
}

void PathBarButton::mousePressEvent(QMouseEvent *e)
{
    if (active) {
        // Start timer
        if (e->button() == Qt::LeftButton && menu != nullptr)
            menuTimer.start();

        QPushButton::mousePressEvent(e);
    }
}

void PathBarButton::mouseReleaseEvent(QMouseEvent *e)
{
    menuTimer.stop();
    if (active) {
        if (e->button() == Qt::RightButton)
            showMenu();
        else if (!menuRequested)
            QPushButton::mouseReleaseEvent(e);
        else {
            menuRequested = false;
            setDown(false);
        }
    }
}

void PathBarButton::setActive(bool value)
{
    active = value;
}

void PathBarButton::setMenu(QMenu *menu)
{
    if (this->menu != nullptr)
        delete this->menu;

    this->menu = menu;
}

void PathBarButton::showMenu()
{
    menuTimer.stop();

    if (menu != nullptr) {
        QAction *action = menu->exec(QCursor::pos());
        if (action != nullptr)
            emit menuSelected(action);
    }

    setDown(false);
    update();
}
