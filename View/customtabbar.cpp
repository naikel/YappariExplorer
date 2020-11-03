#include <QApplication>
#include <QStyleOptionTab>
#include <QMouseEvent>
#include <QDebug>
#include <QLabel>
#include <QIcon>

#ifdef Q_OS_WIN
#   include <QTabBar>
#   include <QFont>
#   include <qt_windows.h>
#endif

#include "customtabbar.h"
#include "customtabbarstyle.h"

CustomTabBar::CustomTabBar(QWidget *parent) : QTabBar(parent)
{

#ifdef Q_OS_WIN

    // The tab bar doesn't have the default font on Windows 10
    // I believe this will be fixed in Qt 6 but in the meantime let's just get the default font and apply it to the tab bar
    // Most of the cases it should be Segoe UI 9 pts

    NONCLIENTMETRICS ncm;
    ncm.cbSize = FIELD_OFFSET(NONCLIENTMETRICS, lfMessageFont) + sizeof(LOGFONT);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize , &ncm, 0);

    HDC defaultDC = GetDC(nullptr);
    int verticalDPI_In = GetDeviceCaps(defaultDC, LOGPIXELSY);
    ReleaseDC(nullptr, defaultDC);

    QFont f = font();
    f.setFamily(QString::fromWCharArray(ncm.lfMenuFont.lfFaceName));
    f.setPointSize(static_cast<int>(qAbs(ncm.lfMenuFont.lfHeight) * 72.0 / qreal(verticalDPI_In)));
    setFont(f);

#endif

    CustomTabBarStyle *style = new CustomTabBarStyle();
    setStyle(style);
    setExpanding(false);
    setDrawBase(false);
    setMovable(true);
    addTab(QIcon(":/icons/addtab.png"), QString());

    connect(this, &CustomTabBar::tabBarClicked, this, &CustomTabBar::tabClicked);
}

int CustomTabBar::getPaddingForIndex(int index) const
{
    QFontMetrics fm = fontMetrics();
    int space = fm.size(0, " ").width();

#ifdef Q_OS_WIN
    // If the tab text is empty it means this is the "add new tab" tab, we want that tab even smaller
    return tabText(index).isEmpty() ? space*7 : space*3;
#else
    return tabText(index).isEmpty() ? space*2 : 0;
#endif

}

void CustomTabBar::tabClicked(int index)
{
    if (index == count()-1) {
        qDebug() << "CustomTabBar::tabClicked: new tab requested";
        emit newTabClicked();
    }
}

QSize CustomTabBar::minimumTabSizeHint(int index) const
{
    QSize size = QTabBar::minimumTabSizeHint(index);
    return QSize(size.width()-getPaddingForIndex(index), size.height());
}

QSize CustomTabBar::tabSizeHint(int index) const
{
    QSize size = QTabBar::tabSizeHint(index);
    return QSize(size.width()-getPaddingForIndex(index), size.height());
}

void CustomTabBar::mousePressEvent(QMouseEvent *event)
{
    dragStartPosition = event->pos();
    dragIndex = tabAt(event->pos());

    QTabBar::mousePressEvent(event);
}

void CustomTabBar::mouseMoveEvent(QMouseEvent *event)
{
    if (!dragInProgress && dragIndex != -1) {
        if ((event->pos() - dragStartPosition).manhattanLength() > QApplication::startDragDistance()) {
            dragInProgress = true;
            qDebug() << "RECT" << tabSizeHint(dragIndex);
            removeTab(count() - 1);
        }
    }

    QTabBar::mouseMoveEvent(event);
}

void CustomTabBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (dragInProgress) {
        dragInProgress = false;
        addTab(QIcon(":/icons/addtab.png"), QString());
    }

    QTabBar::mouseReleaseEvent(event);
}

int CustomTabBar::getDragIndex() const
{
    return dragIndex;
}

QSize CustomTabBar::hintSizeOfTab(int index)
{
    return tabSizeHint(index);
}

bool CustomTabBar::isDragging() const
{
    return dragInProgress;
}
