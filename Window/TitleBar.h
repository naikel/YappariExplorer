#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QPushButton>
#include <QTimer>


class TitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit TitleBar(qreal titleBarHeight, QWidget *parent = nullptr);

    bool isTitleBar(const QPoint &pos);

    void update(bool isActivated, bool isMaximized);

signals:
    void closeRequested();
    void showMaximizedRequested();
    void showMinimizedRequested();
    void showNormaledRequested();

public slots:
    void closeButtonClicked();
    void maximizeButtonClicked();
    void minimizeButtonClicked();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    qreal titleBarHeight {};
    QString buttonStyleSheet { "\
        #iconButton, #minimizeButton, #maximizeButton, #closeButton { \
         border-style: none; \
         background-color: transparent; \
        } \
        #minimizeButton:hover, #maximizeButton:hover { \
         background-color: #800fbd46; \
        } \
        #minimizeButton:pressed, #maximizeButton:pressed { \
         background-color: #80075520; \
        } \
        #closeButton:hover { \
         background-color: #e81123; \
        } \
        #closeButton:pressed { \
         background-color: #8c0a15; \
        } \
        "
    };
    QPushButton *maximizeButton, *minimizeButton, *closeButton;
    bool qtBug {};
    bool active {true};

    QPushButton *createButton(QIcon icon, QString objectName);

};

#endif // TITLEBAR_H
