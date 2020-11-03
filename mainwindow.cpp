#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QTabWidget>
#include <QWindow>
#include <QDebug>

#include "View/customtabbar.h"

#ifdef Q_OS_WIN
#   include "Shell/Win/wincontextmenu.h"
#   define PlatformContextMenu(PARENT)     WinContextMenu(PARENT)

quint32 MainWindow::nextId { WM_USER + 200 };

#else
#   include "Shell/Unix/unixcontextmenu.h"
#   define PlatformContextMenu(PARENT)     UnixContextMenu(PARENT)

quint32 MainWindow::nextId {};

#endif

#define APPLICATION_TITLE   "YappariExplorer"

WId MainWindow::windowId                                {};
QMutex MainWindow::regMutex                             {};
QMap<quint32, DirectoryWatcher*> MainWindow::watchers   {};

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

    windowId = winId();
}

MainWindow::~MainWindow()
{
    delete ui;
}

WId MainWindow::getWinId()
{
    return windowId;
}

quint32 MainWindow::registerWatcher(DirectoryWatcher *watcher)
{
    regMutex.lock();

    quint32 id = nextId++;
    watchers.insert(id, watcher);

    regMutex.unlock();
    return id;
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

void MainWindow::updateTitle(FileSystemItem *item)
{
    QString title = APPLICATION_TITLE;
    if (item != nullptr) {
        title = item->getDisplayName() + " - " + title;
    }
    setWindowTitle(title);
}

void MainWindow::showContextMenu(const QPoint &pos, const QList<FileSystemItem *> fileSystemItems,
                                 const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view)
{
    qDebug() << "MainWindow::showContextMenu";

    contextMenu->show(winId(), pos, fileSystemItems, viewAspect, view);
}

void MainWindow::defaultAction(const FileSystemItem *fileSystemItem)
{
    qDebug() << "MainWindow::defaultAction";

    contextMenu->defaultAction(winId(), fileSystemItem);
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    quint32 id;

#ifdef Q_OS_WIN
    MSG *msg = static_cast< MSG * >(message);
    id = msg->message;
#endif

    if (watchers.keys().contains(id)) {
        DirectoryWatcher *watcher = watchers.value(id);
        return watcher->handleNativeEvent(eventType, message, result);
    }

    if (contextMenu && contextMenu->handleNativeEvent(eventType, message, result))
        return true;

    return QMainWindow::nativeEvent(eventType, message, result);
}
