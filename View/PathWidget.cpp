#include <QStylePainter>
#include <QApplication>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QLabel>
#include <QMenu>
#include <QDebug>
#include <QStyle>
#include <QColor>

#include "once.h"

#include "View/PathWidget.h"


PathWidget::PathWidget(QWidget *parent) : QFrame(parent)
{
    setMinimumHeight(40);
    setMaximumHeight(40);
    QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setSizePolicy(sizePolicy);
    setFrameShape(QFrame::Box);
    setFrameShadow(QFrame::Plain);
    setStyleSheet("#pathWidget { background: white; border: 1px solid #dfdfdf; } QLabel { border: 0px; }");
    setObjectName(QString::fromUtf8("pathWidget"));
}

void PathWidget::setModel(FileSystemModel *model)
{
    this->model = model;
}

void PathWidget::selectIndex(QModelIndex index)
{
    // Delete previous buttons if any
    for (PathWidgetButton *button : qAsConst(buttons))
        delete button;

    buttons.clear();
    delete layout();

    QHBoxLayout *layout = new QHBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(5, 0, 5, 0);

    QLabel *label {};
    if (index.isValid()) {
        QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
        label = new QLabel(this);
        label->setPixmap(icon.pixmap(QSize(32, 32)));
        label->setMinimumWidth(35);
        label->setMinimumHeight(35);

        while (index.isValid()) {
            PathWidgetButton *button = new PathWidgetButton(index, this);
            connect(button->getPathButton(), &CustomButton::clicked, this, [this, index]() { this->clickedOnIndex(index); });
            if (button->getMenuButton() != nullptr)
                connect(button->getMenuButton(), &CustomButton::menuIndexSelected, this, &PathWidget::clickedOnIndex);
            buttons.append(button);
            layout->insertWidget(0, button);
            index = index.parent();
        }

        if (label != nullptr)
            layout->insertWidget(0, label);
    }

    layout->addStretch(1);
    layout->setSpacing(0);
}

void PathWidget::clickedOnIndex(const QModelIndex &index)
{
    emit selectedIndex(index);
}

PathWidgetButton::PathWidgetButton(QModelIndex &index, QWidget *parent) : QWidget(parent)
{
    this->index = index;

    setStyleSheet("QPushButton { padding-right: 5px; padding-left: 5px; border-width: 1px; border-style: solid; background-color: transparent; } "
                  "QPushButton:hover { background-color: #500fbd46; border-color: #a00fbd46; } QPushButton:pressed { background-color: #800fbd46; } "
                  "QPushButton::menu-indicator { image: url(:/icons/next.svg); width: 11px; position: relative; top: -5px; right: 6px; } "
                  "QPushButton::menu-indicator:pressed { image: url(:/icons/down.svg); } ");

    pathButton = new CustomButton(index.data(Qt::DisplayRole).toString(), this);
    pathButton->setStyleSheet("QPushButton:pressed { background-color: #800fbd46; } ");
    pathButton->setIndex(index);

    if (index.data(FileSystemModel::HasSubFoldersRole).toBool()) {
        menuButton = new CustomButton(this);
        menuButton->setIndex(index);
        menuButton->setMenu(new QMenu(menuButton));
        menuButton->setMaximumWidth(25);
        menuButton->setStyleSheet("QPushButton { border-width: 1px 1px 1px 0px; } QPushButton:pressed { background-color: #800fbd46; }");
        menuButton->setSibling(pathButton);
        pathButton->setSibling(menuButton);
    }

    QHBoxLayout *layout = new QHBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(pathButton);
    if (menuButton != nullptr)
        layout->addWidget(menuButton);
    layout->setSpacing(0);
}

CustomButton *PathWidgetButton::getPathButton() const
{
    return pathButton;
}

CustomButton *PathWidgetButton::getMenuButton() const
{
    return menuButton;
}

CustomButton::CustomButton(QString text, QWidget *parent) : QPushButton(text, parent)
{
    setMinimumHeight(38);
}

CustomButton::CustomButton(QWidget *parent) : QPushButton(parent)
{
    setMinimumHeight(38);
}

void CustomButton::setSibling(CustomButton *sibling)
{
    this->sibling = sibling;
}

void CustomButton::setIndex(QModelIndex &index)
{
    this->index = index;
}

void CustomButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QStylePainter p(this);
    QStyleOptionButton option;
    initStyleOption(&option);

    if (sibling != nullptr && (sibling->underMouse() || sibling->isDown()))
        option.state |= QStyle::State_MouseOver;

    if (oldState != option.state) {
        oldState = option.state;
        if (sibling != nullptr)
            sibling->update();
    }

    p.drawControl(QStyle::CE_PushButton, option);

}

void CustomButton::mousePressEvent(QMouseEvent *e)
{
    // Check if we have a menu
    if (menu() != nullptr && e->button() == Qt::LeftButton) {

        QAbstractItemModel *model = const_cast<QAbstractItemModel *>(index.model());

        if (!model->canFetchMore(index)) {
            populateMenu();
            if (menu()->isEmpty()) {
                hide();
                return;
            }
        } else {
            model->fetchMore(index);
            setDown(true);
            fetchingMore = true;
            connect(model, &QAbstractItemModel::dataChanged, this, &CustomButton::checkIfFetchFinished);
            return;
        }
    }

    fetchingMore = false;
    QPushButton::mousePressEvent(e);
}

void CustomButton::mouseReleaseEvent(QMouseEvent *e)
{
    if (!fetchingMore)
        QPushButton::mouseReleaseEvent(e);

}

void CustomButton::populateMenu()
{
    const QAbstractItemModel *model = index.model();
    QMenu *cMenu = menu();
    cMenu->clear();
    for (int i = 0 ; i < model->rowCount(index) ; i++) {
        const QModelIndex child = model->index(i, 0, index);

        if (!child.data(FileSystemModel::FileRole).toBool()) {
            QAction *action = cMenu->addAction(child.data(Qt::DecorationRole).value<QIcon>(), child.data(Qt::DisplayRole).toString());
            if (action != nullptr)
                connect(action, &QAction::triggered, this, [this, child]() { this->actionTriggered(child); });
        }
    }
    cMenu->setStyleSheet("QMenu { menu-scrollable: 1; }");
}

void CustomButton::actionTriggered(const QModelIndex &index)
{
    emit menuIndexSelected(index);
}

void CustomButton::checkIfFetchFinished(const QModelIndex &topLeft, const QModelIndex &, const QVector<int> &roles)
{
    if (topLeft == index && (roles.contains(FileSystemModel::AllChildrenFetchedRole) || roles.contains(FileSystemModel::ErrorCodeRole))) {

        bool fetchFinished = topLeft.data(FileSystemModel::AllChildrenFetchedRole).toBool();

        if (roles.contains(FileSystemModel::ErrorCodeRole) || fetchFinished)
            disconnect(index.model(), &QAbstractItemModel::dataChanged, this, &CustomButton::checkIfFetchFinished);

        fetchingMore = false;

        if (fetchFinished) {
            populateMenu();
            if (menu()->isEmpty()) {
                hide();
            } else
                showMenu();
        }
    }
}
