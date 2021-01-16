#include <QHBoxLayout>
#include <QMenu>

#include "PathBar.h"

#define BASESTYLESHEET      "QFrame { background: white; border-bottom: 1px solid #9fcdb3; } " \
                            "QPushButton { border-style: none; background-color: transparent; } "
#define BUTTONSTYLESHEET    "QPushButton:hover { background-color: #800fbd46; } QPushButton:pressed { background-color: #80075520; }"

#ifdef Q_OS_WIN
#define getText(entry) ((entry->path.size() > 3 && entry->path.left(3) == "::{") || (!entry->path.isNull() && entry->path.length() == 3 && entry->path.right(2) == ":\\")) ? entry->displayName : entry->path
#else
#define getText(entry) entry->path
#endif

PathBar::PathBar(QWidget *parent) : QFrame(parent)
{
    setMinimumHeight(40);
    setMaximumHeight(40);
    setFrameShape(QFrame::NoFrame);
    setFrameShadow(QFrame::Plain);
    setObjectName(QString::fromUtf8("pathBar"));
    setStyleSheet(BASESTYLESHEET);

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
    hLayout->setSpacing(0);
    hLayout->addStretch(1);
    hLayout->setContentsMargins(0, 0, 0, 0);
}

void PathBar::setModel(FileSystemModel *model)
{
    int cursor;

    this->model = model;

    QList<HistoryEntry *> &history = model->getHistory(&cursor);

    QMenu *backMenu = new QMenu(backButton);
    for (int i = cursor - 1; i >= 0; --i) {

        HistoryEntry *entry = history.at(i);
        QAction *action = backMenu->addAction(entry->icon, getText(entry));
        action->setData(QVariant(i));
    }

    QMenu *nextMenu = new QMenu(nextButton);
    for (int i = cursor + 1; i < history.size(); i++) {

        HistoryEntry *entry = history.at(i);
        QAction *action = nextMenu->addAction(entry->icon, getText(entry));
        action->setData(QVariant(i));
    }

    backButton->setIcon(model->canGoBack() ? QIcon(":/icons/back.svg") : QIcon(":/icons/back_inactive.svg"));
    backButton->setStyleSheet(model->canGoBack() ? BUTTONSTYLESHEET : QString());
    backButton->setActive(model->canGoBack());
    backButton->setMenu(backMenu);

    nextButton->setIcon(model->canGoForward() ? QIcon(":/icons/next.svg") : QIcon(":/icons/next_inactive.svg"));
    nextButton->setStyleSheet(model->canGoForward() ? BUTTONSTYLESHEET : QString());
    nextButton->setActive(model->canGoForward());
    nextButton->setMenu(nextMenu);

    upButton->setIcon(model->canGoUp() ? QIcon(":/icons/up.svg") : QIcon(":/icons/up_inactive.svg"));
    upButton->setStyleSheet(model->canGoUp() ? BUTTONSTYLESHEET : QString());
    upButton->setActive(model->canGoUp());

}

void PathBar::menuSelected(QAction *action)
{
    if (action != nullptr) {
        int pos;

        QVariant actionData = action->data();
        if (actionData.isValid()) {
            pos = actionData.toInt();
            model->goToPos(pos);
            emit rootChange(model->getRoot()->getPath());
        }
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

void PathBar::buttonClicked()
{
    if (model == nullptr)
        return;

    PathBarButton *button = reinterpret_cast<PathBarButton *>(sender());

    if (button == nextButton && model->canGoForward())
        model->goForward();
    else if (button == backButton && model->canGoBack()) {
        model->goBack();
    } else if (button == upButton && model->canGoUp())
        model->goUp();
    else
        return;

    emit rootChange(model->getRoot()->getPath());
}
