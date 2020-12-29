#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QLabel>
#include <QDebug>

#include "TitleBar.h"

TitleBar::TitleBar(qreal titleBarHeight, QWidget *parent) : QWidget(parent)
{
    this->titleBarHeight = titleBarHeight;
    setMinimumSize(QSize(0, titleBarHeight));
    setMaximumSize(QSize(16777215, titleBarHeight));
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
    setSizePolicy(sizePolicy);

    setObjectName(QString::fromUtf8("titleBar"));
    setStyleSheet(buttonStyleSheet);

    maximizeButton = createButton(QIcon(":/icons/button_maximize_white.svg"), "maximizeButton");
    minimizeButton = createButton(QIcon(":/icons/button_minimize_white.svg"), "minimizeButton");
    closeButton    = createButton(QIcon(":/icons/button_close_white.svg"), "closeButton");

    connect(maximizeButton, &QPushButton::clicked, this, &TitleBar::maximizeButtonClicked);
    connect(minimizeButton, &QPushButton::clicked, this, &TitleBar::minimizeButtonClicked);
    connect(closeButton, &QPushButton::clicked, this, &TitleBar::closeButtonClicked);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addStretch(1);
    layout->addWidget(minimizeButton);
    layout->addWidget(maximizeButton);
    layout->addWidget(closeButton);

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
}

void TitleBar::update(bool isActivated, bool isMaximized)
{
    // There's this VERY NASTY bug where the buttons keep their hover style after being clicked.
    // What we do is set an empty style sheet and restore it again when the user moves the cursor
    // again over the titlebar (non client area)
    setStyleSheet(QString());
    qtBug = true;

    closeButton->setIcon(isActivated ? QIcon(":/icons/button_close_white.svg") : QIcon(":/icons/button_close_white_inactive.svg"));
    maximizeButton->setIcon(isMaximized ? (isActivated ? QIcon(":/icons/button_restore_white.svg") : QIcon(":/icons/button_restore_white_inactive.svg"))
                                        : (isActivated ? QIcon(":/icons/button_maximize_white.svg") : QIcon(":/icons/button_maximize_white_inactive.svg")));
    minimizeButton->setIcon(isActivated ? QIcon(":/icons/button_minimize_white.svg") : QIcon(":/icons/button_minimize_white_inactive.svg"));
    active = isActivated;
}

// This function is called whenever the cursor is over the non client area, including the
// system buttons (minimize, maximize, close)
// We take this opportunity to restore the style sheet if we had replaced it
bool TitleBar::isTitleBar(const QPoint &pos)
{
    // @see TitleBar::update
    if (qtBug) {
        setStyleSheet(buttonStyleSheet);
        qtBug = false;
    }
    return (childAt(mapFromGlobal(pos)) == nullptr);
}


void TitleBar::closeButtonClicked()
{
    emit closeRequested();
}

void TitleBar::maximizeButtonClicked()
{
    emit showMaximizedRequested();
}

void TitleBar::minimizeButtonClicked()
{
    emit showMinimizedRequested();
}

void TitleBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.save();
    painter.fillRect(0, 0, width(), titleBarHeight, QBrush(Qt::darkGreen));

    QFont font = QFont("Segoe UI", 10);
    QFontMetrics fm(font);
    QString titleText = "Yappari Explorer";
    QSize titleSize = fm.size(0, titleText);
    painter.setFont(font);
    painter.setPen(active ? Qt::white : QColor(159, 205, 179)); // #9fcdb3
    QRect titleRect = QRect(
                        (width() / 2) - (titleSize.width() / 2),
                        (height() / 2) - (titleSize.height() / 2),
                        (width() / 2) + (titleSize.width() / 2),
                        (height() / 2) + (titleSize.height() / 2)
                      );
    painter.drawText(titleRect, titleText);
    painter.restore();

}

QPushButton *TitleBar::createButton(QIcon icon, QString objectName)
{
    QPushButton *button = new QPushButton(this);
    button->setObjectName(objectName);
    button->setIcon(icon);

    QSizeF systemButtonSizeF = { titleBarHeight * 1.2, titleBarHeight};
    QSize systemButtonSize = systemButtonSizeF.toSize();
    button->setIconSize(systemButtonSize);
    button->setMinimumSize(systemButtonSize);
    button->setMaximumSize(systemButtonSize);
    button->setFlat(true);

    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    button->setSizePolicy(sizePolicy);
    button->setFocusPolicy(Qt::NoFocus);
    return button;

}
