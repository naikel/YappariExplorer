#include <QGuiApplication>
#include <QResizeEvent>
#include <QWindow>
#include <QScreen>
#include <QLayout>
#include <QPainter>
#include <QDebug>

#include <qt_windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <winuser.h>

#include "WinFramelessWindow.h"

#define WM_NCUAHDRAWCAPTION     0x00AE
#define WM_NCUAHDRAWFRAME       0x00AF

#include <QPushButton>

WinFramelessWindow::WinFramelessWindow(QWidget *parent) : QMainWindow(parent)
{
    borderWidth = getPhysicalPixels(1);
    titleBarHeight = getPhysicalPixels(30);

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setObjectName("centralWidget");
    QVBoxLayout *vLayout = new QVBoxLayout(centralWidget);

    _contentWidget = new QWidget(this);
    QVBoxLayout *contentLayout = new QVBoxLayout(_contentWidget);
    contentLayout->setContentsMargins(borderWidth, borderWidth, borderWidth, borderWidth);

    titleBar = new TitleBar(titleBarHeight, centralWidget);

    connect(titleBar, &TitleBar::closeRequested, this, &WinFramelessWindow::close);
    connect(titleBar, &TitleBar::showMinimizedRequested, this, &WinFramelessWindow::showMinimized);
    connect(titleBar, &TitleBar::showMaximizedRequested, this, &WinFramelessWindow::restoreOrMaximize);

    vLayout->setSpacing(0);
    vLayout->addWidget(titleBar);
    vLayout->addWidget(_contentWidget);
    vLayout->setContentsMargins(0, 0, 0, 0);

    setCentralWidget(centralWidget);

    createWinId();
    windowId = winId();
}

qreal WinFramelessWindow::getPhysicalPixels(qreal virtualPixels)
{
    return virtualPixels * (logicalDpiX() / 96.0);
}

bool WinFramelessWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);

    MSG *msg = static_cast< MSG * >(message);

    if (msg->hwnd == nullptr)
        return false;

    switch (msg->message) {

        case WM_SIZE:
            if (msg->wParam == SIZE_MAXIMIZED || msg->wParam == SIZE_RESTORED) {
                maximized = (msg->wParam == SIZE_MAXIMIZED);
                titleBar->update(true, maximized);
            }
            break;

        case WM_ACTIVATE:
            titleBar->update(msg->wParam != WA_INACTIVE, maximized);
            break;

        // Calculate the nonclient area.
        // This is the area for the window's frame including the title bar
        case WM_NCCALCSIZE:
        {

            // WPARAM must be TRUE
            if (!msg->wParam) {
                *result = 0;
                return true;
            }

            auto& params = *reinterpret_cast<NCCALCSIZE_PARAMS *>(msg->lParam);
            RECT& clientRect = params.rgrc[0];

            // If the window is maximized we need to set the rectangle as the same size of the monitor
            WINDOWPLACEMENT placement;
            if (!::GetWindowPlacement(msg->hwnd, &placement)) {
                  return false;
            }
            if (placement.showCmd == SW_MAXIMIZE) {

                auto monitor = ::MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONULL);
                if (!monitor)
                    return false;
                MONITORINFO monitor_info {};
                monitor_info.cbSize = sizeof(monitor_info);
                if (!::GetMonitorInfoW(monitor, &monitor_info))
                    return false;

                clientRect = monitor_info.rcWork;

                *result = WVR_REDRAW;
                return true;
            }

            // This fixes the flickering, I don't know why but it does
            clientRect.bottom -= 1;

            // Redraw the window after the non client area changed
            *result = WVR_REDRAW;
            return true;
        }

        // This is the hit test in the nonclient area
        // We tell Windows if we are in a border or in the titlebar
        case WM_NCHITTEST:
        {
            RECT winrect;
            ::GetWindowRect(msg->hwnd, &winrect);

            long x = GET_X_LPARAM(msg->lParam);
            long y = GET_Y_LPARAM(msg->lParam);

            int resizeBorderWidth = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);

            bool left    = (x >= winrect.left   && x <= winrect.left   + resizeBorderWidth);
            bool right   = (x <= winrect.right  && x >= winrect.right  - resizeBorderWidth);
            bool top     = (y >= winrect.top    && y <= winrect.top    + resizeBorderWidth);
            bool bottom  = (y <= winrect.bottom && y >= winrect.bottom - resizeBorderWidth);
            bool caption = (y >= winrect.top    && y <= winrect.top    + titleBarHeight) &&
                           (x >= winrect.left   && x <= winrect.right);

            *result = HTCLIENT;
            if (left)  *result = top ? HTTOPLEFT : (bottom ? HTBOTTOMLEFT : HTLEFT);
            else if (right) *result = top ? HTTOPRIGHT : (bottom ? HTBOTTOMRIGHT : HTRIGHT);
            else if (top || bottom) *result = (top) ? HTTOP : HTBOTTOM;
            else if (caption && titleBar->isTitleBar(QPoint(x, y))) *result = HTCAPTION;

            return true;
        }

        // This blocks the painting of the Windows themed titlebar
        // We intercept the call and change lParam to -1 so DefWindowProc does not
        // repaint the nonclient area
        case WM_NCACTIVATE:
            *result = DefWindowProcW(msg->hwnd, msg->message, msg->wParam, -1);
            return true;

        // Windows is trying to paint the titlebar
        // Just overwrite our content to it, otherwise we would get the default non-themed titlebar
        case WM_NCPAINT:
            update();
            break;

        // These nonclient area messages are not documented
        // They are received to draw themed borders
        // We have to block them
        case WM_NCUAHDRAWFRAME:
        case WM_NCUAHDRAWCAPTION:

        // Also disable painting of the titlebar's icon or text
        case WM_SETICON:
        case WM_SETTEXT:
            *result = 0;
            return true;
    }

    return QMainWindow::nativeEvent(eventType, message, result);
}

void WinFramelessWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);

    WINDOWPLACEMENT placement;
    if (!::GetWindowPlacement(reinterpret_cast<HWND>(winId()), &placement)) {
          return;
    }
    if (placement.showCmd != SW_MAXIMIZE) {
        QPainter painter(this);
        painter.save();
        painter.setPen({Qt::darkGreen, borderWidth});
        painter.drawLine(0, 1, width(), 1);
        painter.drawLine(0, height() - borderWidth + 2, width(), height() - borderWidth + 2);
        painter.drawLine(1, 0, 1, height());
        painter.drawLine(width() - borderWidth + 1, 0, width() - borderWidth + 1, height());
        painter.restore();
    }
}

void WinFramelessWindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)

    WINDOWPLACEMENT placement;
    if (!::GetWindowPlacement(reinterpret_cast<HWND>(windowId), &placement)) {
          return;
    }

    if (placement.showCmd != SW_MAXIMIZE) {
        _contentWidget->layout()->setContentsMargins(borderWidth + 1, 0, borderWidth + 1, borderWidth);
    } else {
        _contentWidget->layout()->setContentsMargins(0, 0, 0, 0);
    }

    ::SetWindowPos(reinterpret_cast<HWND>(windowId), NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
}

QWidget *WinFramelessWindow::contentWidget() const
{
    return _contentWidget;
}

void WinFramelessWindow::setContentWidget(QWidget *value)
{
    _contentWidget = value;
}

int WinFramelessWindow::y()
{
    WINDOWPLACEMENT placement;
    if (!::GetWindowPlacement(reinterpret_cast<HWND>(winId()), &placement)) {
          return -1;
    }

    int y = placement.rcNormalPosition.top;

    return (y - titleBarHeight) + 1;
}

int WinFramelessWindow::x()
{
    WINDOWPLACEMENT placement;
    if (!::GetWindowPlacement(reinterpret_cast<HWND>(winId()), &placement)) {
          return -1;
    }

    int x = placement.rcNormalPosition.left;

    return x - 1;
}


bool WinFramelessWindow::isMaximized()
{
    return maximized;
}

void WinFramelessWindow::restoreOrMaximize()
{
    WINDOWPLACEMENT placement;
    if (::GetWindowPlacement(reinterpret_cast<HWND>(windowId), &placement)) {
        maximized = (placement.showCmd == SW_MAXIMIZE);
    }

    // Probably because of the many customizations we've done for this window
    // the Qt functions showNormaled() and showMaximized() behave really buggy
    // and sometimes fail so we use the Windows API directly.
    ::ShowWindow(reinterpret_cast<HWND>(windowId), maximized ? SW_RESTORE : SW_MAXIMIZE);
}
