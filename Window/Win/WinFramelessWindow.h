#ifndef WINFRAMELESSWINDOW_H
#define WINFRAMELESSWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>

#include "Window/TitleBar.h"

class WinFramelessWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit WinFramelessWindow(QWidget *parent = nullptr);

    qreal getPhysicalPixels(qreal virtualPixels);

    QWidget *contentWidget() const;
    void setContentWidget(QWidget *value);
    int y();
    int x();
    bool isMaximized();

protected:
    virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

signals:

private:
    WId windowId;
    qreal borderWidth;
    qreal titleBarHeight;
    qreal pixelRatio        { 1.0 };
    bool maximized          {};

    QWidget *_contentWidget;
    TitleBar *titleBar;

    void restoreOrMaximize();
};

#endif // WINFRAMELESSWINDOW_H
