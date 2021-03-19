#include <QSortFilterProxyModel>
#include <QHBoxLayout>
#include <QDebug>
#include <QMenu>

#include "PathBar.h"

#define BASESTYLESHEET      "QFrame { background: white; border-bottom: 1px solid #9fcdb3; } " \
                            "QPushButton { border-style: none; background-color: transparent; } "
#define BUTTONSTYLESHEET    "QPushButton:hover { background-color: #500fbd46; } QPushButton:pressed { background-color: #800fbd46; }"

PathBar::PathBar(QWidget *parent) : QFrame(parent)
{
    setMinimumHeight(50);
    setMaximumHeight(50);
    setFrameShape(QFrame::NoFrame);
    setFrameShadow(QFrame::Plain);
    setObjectName(QString::fromUtf8("pathBar"));
    setStyleSheet(BASESTYLESHEET);

    pathWidget = new PathWidget(this);

    connect(pathWidget, &PathWidget::selectedIndex, this, &PathBar::selectedIndex);

    QHBoxLayout *hLayout = new QHBoxLayout(this);
    setLayout(hLayout);

    backButton = createButton(QIcon(":/icons/back.svg"), "backButton");
    nextButton = createButton(QIcon(":/icons/next.svg"), "nextButton");
    upButton   = createButton(QIcon(":/icons/up.svg"),   "upButton");

    connect(backButton, &QPushButton::clicked, this, &PathBar::buttonClicked);
    connect(upButton, &QPushButton::clicked, this, &PathBar::buttonClicked);
    connect(nextButton, &QPushButton::clicked, this, &PathBar::buttonClicked);

    connect(backButton, &PathBarButton::menuSelected, this, &PathBar::menuSelected);
    connect(nextButton, &PathBarButton::menuSelected, this, &PathBar::menuSelected);

    hLayout->addWidget(backButton);
    hLayout->addWidget(nextButton);
    hLayout->addWidget(upButton);
    hLayout->addSpacerItem(new QSpacerItem(10, 50));
    hLayout->addWidget(pathWidget);
    hLayout->setSpacing(0);
    hLayout->setContentsMargins(5, 0, 5, 0);
}

void PathBar::selectTreeIndex(const QModelIndex& selectedIndex)
{
    pathWidget->selectIndex(selectedIndex);
}

void PathBar::setHistory(FileSystemHistory *history)
{
    this->history = history;

    QList<HistoryEntry *> historyList = history->getPathList();
    int cursor = history->getCursor();

    QMenu *backMenu = new QMenu(backButton);
    for (int i = cursor - 1; i >= 0; --i) {

        HistoryEntry *entry = historyList[i];
        QAction *action = backMenu->addAction(entry->icon, entry->displayName);
        action->setData(QVariant(i));
    }

    QMenu *nextMenu = new QMenu(nextButton);
    for (int i = cursor + 1; i < historyList.size(); i++) {

        HistoryEntry *entry = historyList[i];
        QAction *action = nextMenu->addAction(entry->icon, entry->displayName);
        action->setData(QVariant(i));
    }

    backButton->setIcon(history->canGoBack() ? QIcon(":/icons/back.svg") : QIcon(":/icons/back_inactive.svg"));
    backButton->setStyleSheet(history->canGoBack() ? BUTTONSTYLESHEET : QString());
    backButton->setActive(history->canGoBack());
    backButton->setMenu(backMenu);

    nextButton->setIcon(history->canGoForward() ? QIcon(":/icons/next.svg") : QIcon(":/icons/next_inactive.svg"));
    nextButton->setStyleSheet(history->canGoForward() ? BUTTONSTYLESHEET : QString());
    nextButton->setActive(history->canGoForward());
    nextButton->setMenu(nextMenu);

    upButton->setIcon(history->canGoUp() ? QIcon(":/icons/up.svg") : QIcon(":/icons/up_inactive.svg"));
    upButton->setStyleSheet(history->canGoUp() ? BUTTONSTYLESHEET : QString());
    upButton->setActive(history->canGoUp());
}

void PathBar::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "PathBar::mousePressEvent";

    QFrame::mousePressEvent(event);
}

void PathBar::menuSelected(QAction *action)
{
    if (action != nullptr) {

        QVariant actionData = action->data();
        if (actionData.isValid())
            emit pathChangeRequested(history->getItemAtPos(actionData.toInt()));
    }
}

void PathBar::selectedIndex(const QModelIndex &index)
{
    if (index.isValid()) {

        const QSortFilterProxyModel *treeModel = reinterpret_cast<const QSortFilterProxyModel *>(index.model());

        emit rootIndexChangeRequested(treeModel->mapToSource(index));
    }
}

PathBarButton *PathBar::createButton(QIcon icon, QString objectName)
{
    PathBarButton *button = new PathBarButton(this);
    button->setObjectName(objectName);
    button->setIcon(icon);

    QSize buttonSize(40, 40);
    button->setIconSize(QSize(20, 20));
    button->setMinimumSize(buttonSize);
    button->setMaximumSize(buttonSize);
    button->setFlat(true);

    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    button->setSizePolicy(sizePolicy);
    button->setFocusPolicy(Qt::NoFocus);
    return button;

}

QString PathBar::getDisplayName(const QModelIndex &index)
{
    QString path = index.data(FileSystemModel::PathRole).toString();

#ifdef Q_OS_WIN
    if ((path.size() > 3 && path.left(3) == "::{") || (!path.isNull() && path.length() == 3 && path.right(2) == ":\\"))
        return index.data(Qt::DisplayRole).toString();
#endif

    return path;
}

void PathBar::buttonClicked()
{
    PathBarButton *button = reinterpret_cast<PathBarButton *>(sender());

    if (button == nextButton && history->canGoForward())
        emit pathChangeRequested(history->getNextItem());
    else if (button == backButton && history->canGoBack()) {
        emit pathChangeRequested(history->getLastItem());
    } else if (button == upButton && history->canGoUp())
        emit pathChangeRequested(history->getParentItem());
}
