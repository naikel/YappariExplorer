#include "customtabwidget.h"

#include <QHeaderView>
#include <QDebug>

#ifdef Q_OS_WIN
#   include <QTabBar>
#   include <QFont>
#   include <qt_windows.h>
#endif

#include "customtabbar.h"
#include "detailedview.h"

CustomTabWidget::CustomTabWidget(QWidget *parent) : QTabWidget(parent)
{
    QTabBar *customTabBar = new CustomTabBar(this);
    setTabBar(customTabBar);

#ifdef Q_OS_WIN

    // The tab bar doesn't have the default font on Windows 10
    // I believe this will be fixed in Qt 6 but in the meantime let's just get the default font and apply it to the tab bar
    // Most of the cases it should be Segoi UI 9 pts

    QFont font = customTabBar->font();

    NONCLIENTMETRICS ncm;
    ncm.cbSize = FIELD_OFFSET(NONCLIENTMETRICS, lfMessageFont) + sizeof(LOGFONT);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize , &ncm, 0);

    HDC defaultDC = GetDC(nullptr);
    int verticalDPI_In = GetDeviceCaps(defaultDC, LOGPIXELSY);
    ReleaseDC(nullptr, defaultDC);

    font.setFamily(QString::fromWCharArray(ncm.lfMenuFont.lfFaceName));
    font.setPointSize(static_cast<int>(qAbs(ncm.lfMenuFont.lfHeight) * 72.0 / qreal(verticalDPI_In)));
    customTabBar->setFont(font);

#endif

    DetailedView *detailedView = new DetailedView(this);
    FileSystemModel *fileSystemModel = new FileSystemModel(FileInfoRetriever::List, detailedView);
    fileSystemModel->setRoot("/");
    detailedView->setModel(fileSystemModel);
    connect(detailedView, &DetailedView::doubleClicked, this, &CustomTabWidget::doubleClicked);
    connect(fileSystemModel, &FileSystemModel::fetchFinished, this, &CustomTabWidget::updateTab);

    addTab(detailedView, fileSystemModel->getRoot()->getDisplayName());
    setTabIcon(0, fileSystemModel->getRoot()->getIcon());
    setCurrentIndex(0);

}

void CustomTabWidget::changeRootIndex(const QModelIndex &index)
{
    qDebug() << "CustomTabWidget::changeRootIndex";
    if (index.isValid() && index.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
        changeRootPath(fileSystemItem->getPath());
    }
}

void CustomTabWidget::changeRootPath(const QString path)
{
    qDebug() << "CustomTabWidget::changeRootPath new path " << path;
    DetailedView *detailedView = static_cast<DetailedView *>(currentWidget());
    FileSystemModel *fileSystemModel = static_cast<FileSystemModel *>(detailedView->model());
    if (!(fileSystemModel->getRoot()->getPath() == path))
        fileSystemModel->setRoot(path);
    else
        qDebug() << "CustomTabWidget::changeRootPath already in path " << path;
}

void CustomTabWidget::doubleClicked(const QModelIndex &index)
{
    qDebug() << "CustomTabWidget::doubleClicked";
    if (index.isValid() && index.internalPointer() != nullptr) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
        if (fileSystemItem->isDrive() || fileSystemItem->isFolder()) {
            QString path = fileSystemItem->getPath();

            // This will delete (free) the fileSystemItem and it's no longer valid
            changeRootIndex(index);

            emit rootChanged(path);
        }
    }
}

void CustomTabWidget::updateTab()
{
    qDebug() << "CustomTabWidget::updateTab";
    DetailedView *detailedView = static_cast<DetailedView *>(currentWidget());
    FileSystemModel *fileSystemModel = static_cast<FileSystemModel *>(detailedView->model());
    setTabText(0, fileSystemModel->getRoot()->getDisplayName());
    setTabIcon(0, fileSystemModel->getRoot()->getIcon());
    detailedView->scrollToTop();
    qDebug() << "CustomTabWidget::updateTab tab updated. " << fileSystemModel->getRoot()->childrenCount() << " items";
    qDebug() << "---------------------------------------------------------------------------------------------------";
}

void CustomTabWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    qDebug() << "CustomTabWidget::mouseDoubleClickEvent open new tab";

    QTabWidget::mouseDoubleClickEvent(event);
}
