#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QTabWidget>
#include <QWindow>
#include <QTabBar>
#include <QDebug>

#include "once.h"
#include "View/customtabbar.h"

#ifdef Q_OS_WIN
#include "Shell/Win/wincontextmenu.h"
#define PlatformContextMenu(PARENT)     WinContextMenu(PARENT)
#else
#include "Shell/Unix/unixcontextmenu.h"
#define PlatformContextMenu(PARENT)     UnixContextMenu(PARENT)
#endif

#define APPLICATION_TITLE   "YappariExplorer"

#define TOP_TREEVIEW        0

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(":/icons/app2.png"));
    setWindowTitle(APPLICATION_TITLE);

    contextMenu = new PlatformContextMenu(this);
    // Resize tree views
    connect(ui->topTreeView, &CustomTreeView::resized, this, &MainWindow::resizeBottomTreeView);
    connect(ui->bottomTreeView, &CustomTreeView::resized, this, &MainWindow::resizeTopTreeView);

    // Initialize explorers
    ui->bottomExplorer->initialize(this, ui->bottomTreeView, ui->bottomTabWidget);
    ui->topExplorer->initialize(this, ui->topTreeView, ui->topTabWidget);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resizeBottomTreeView()
{
    QList<int> currentSizes = ui->topSplitter->sizes();
    if (currentSizes.at(0) != ui->bottomSplitter->sizes().at(0))
        ui->bottomSplitter->setSizes(currentSizes);
}

void MainWindow::resizeTopTreeView()
{
    QList<int> currentSizes = ui->bottomSplitter->sizes();
    if (currentSizes.at(0) != ui->topSplitter->sizes().at(0))
        ui->topSplitter->setSizes(currentSizes);
}

void MainWindow::changeTitle(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected)
    Q_UNUSED(deselected)

    QString title = APPLICATION_TITLE;
    QModelIndex index = selected.indexes().at(0);
    if (index.isValid()) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
        title = fileSystemItem->getDisplayName() + " - " + title;
    }
    setWindowTitle(title);
}

void MainWindow::showContextMenu(const QPoint &pos, const QList<FileSystemItem *> fileSystemItems, const ContextMenu::ContextViewAspect viewAspect)
{
    qDebug() << "MainWindow::showContextMenu";

    contextMenu->show(winId(), pos, fileSystemItems, viewAspect);
}

void MainWindow::defaultAction(const FileSystemItem *fileSystemItem)
{
    contextMenu->defaultAction(winId(), fileSystemItem);
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    if (contextMenu && contextMenu->handleNativeEvent(eventType, message, result))
        return true;

    return QMainWindow::nativeEvent(eventType, message, result);
}
