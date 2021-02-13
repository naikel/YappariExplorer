#ifndef PATHWIDGET_H
#define PATHWIDGET_H

#include <QStyleOptionButton>
#include <QPushButton>
#include <QFrame>
#include <QWidget>

#include "Model/filesystemmodel.h"

class CustomButton : public QPushButton
{
    Q_OBJECT

public:
    CustomButton(QString text,  QWidget *parent = nullptr);
    CustomButton(QWidget *parent = nullptr);

    void setSibling(CustomButton *sibling);
    void setIndex(QModelIndex &index);

signals:
    void menuIndexSelected(const QModelIndex &index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;


private:
    CustomButton *sibling               {};
    QStyle::State oldState              {};
    QModelIndex index                   {};
    bool fetchingMore                   {};

private slots:
    void populateMenu();
    void fetchFinished();
    void actionTriggered(const QModelIndex &index);

};

class PathWidgetButton : public QWidget
{
    Q_OBJECT

public:
    PathWidgetButton(QModelIndex &index, QWidget *parent = nullptr);

    CustomButton *getPathButton() const;
    CustomButton *getMenuButton() const;

private:
    CustomButton *pathButton            {};
    CustomButton *menuButton            {};
    QModelIndex index                   {};
};

class PathWidget : public QFrame
{
    Q_OBJECT

public:
    PathWidget(QWidget *parent = nullptr);

    void setModel(FileSystemModel *model);
    void selectIndex(QModelIndex index);

signals:
    void selectedIndex(const QModelIndex &index);

private:
    FileSystemModel *model              {};
    QList<PathWidgetButton *> buttons   {};

private slots:
    void clickedOnIndex(const QModelIndex &index);

};

#endif // PATHWIDGET_H
