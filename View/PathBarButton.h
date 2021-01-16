#ifndef PATHBARBUTTON_H
#define PATHBARBUTTON_H

#include <QPushButton>
#include <QTimer>

class PathBarButton : public QPushButton
{
    Q_OBJECT

public:
    PathBarButton(QWidget *parent = nullptr);

    void setActive(bool value);
    void setMenu(QMenu *menu);
    void showMenu();

signals:
    void menuSelected(QAction *action);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    bool menuRequested {};
    bool active        {};
    QMenu *menu        {};

    QTimer menuTimer;
};

#endif // PATHBARBUTTON_H
