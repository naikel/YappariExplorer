#include "customtabwidget.h"

#ifdef Q_OS_WIN
#include <QTabBar>
#include <QFont>
#include <qt_windows.h>
#endif

CustomTabWidget::CustomTabWidget(QWidget *parent) : QTabWidget(parent)
{

#ifdef Q_OS_WIN

    // The tab bar doesn't have the default font on Windows 10
    // I believe this will be fixed in Qt 6 but in the meantime let's just get the default font and
    // apply it to the tab bar
    // Most of the cases it should be Segoi UI 9 pts

    QFont font = tabBar()->font();

    NONCLIENTMETRICS ncm;
    ncm.cbSize = FIELD_OFFSET(NONCLIENTMETRICS, lfMessageFont) + sizeof(LOGFONT);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize , &ncm, 0);

    HDC defaultDC = GetDC(0);
    int verticalDPI_In = GetDeviceCaps(defaultDC, LOGPIXELSY);
    ReleaseDC(0, defaultDC);

    font.setFamily(QString::fromWCharArray(ncm.lfMenuFont.lfFaceName));
    font.setPointSize(qAbs(ncm.lfMenuFont.lfHeight) * 72.0 / qreal(verticalDPI_In));
    tabBar()->setFont(font);

#endif

}
