#include <QApplication>
#include <QHeaderView>
#include <QDebug>
#include <QWindow>
#include <QScreen>
#include <QVBoxLayout>

#include "AppWindow.h"

#include "View/CustomExplorer.h"

#define APPLICATION_TITLE   "Yappari Explorer"

#ifdef Q_OS_WIN

#include "Shell/Win/wincontextmenu.h"
#define PlatformContextMenu(PARENT)     WinContextMenu(PARENT)

quint32 AppWindow::nextId { WM_USER + 200 };

#else
#include "Shell/Unix/unixcontextmenu.h"
#define PlatformContextMenu(PARENT)     UnixContextMenu(PARENT)

quint32 AppWindow::nextId {};

#endif

WId AppWindow::windowId                                {};
QMutex AppWindow::regMutex                             {};
QMap<quint32, DirectoryWatcher*> AppWindow::watchers   {};

AppWindow::AppWindow(QWidget *parent) : MAINWINDOW(parent)
{
    Settings::settings = new Settings();
    connect(QApplication::instance(), &QApplication::aboutToQuit, this, &AppWindow::saveSettings);

    setupGui();

    // Application settings
    setWindowIcon(QIcon(":/icons/app2.png"));
    setWindowTitle(APPLICATION_TITLE);

    contextMenu = new PlatformContextMenu(this);

    windowId = winId();
}

AppWindow::~AppWindow()
{
    qDebug() << "AppWindow::~AppWindow Destroyed";
}

WId AppWindow::getWinId()
{
    return windowId;
}

quint32 AppWindow::registerWatcher(DirectoryWatcher *watcher)
{
    regMutex.lock();

    quint32 id = nextId++;
    watchers.insert(id, watcher);

    qDebug() << "AppWindow::registerWatcher watcher registered with id =" << id;

    regMutex.unlock();
    return id;
}

QWidget *AppWindow::contentWidget()
{
    return CENTRALWIDGET;
}

void AppWindow::showContextMenu(const QPoint &pos, const QList<FileSystemItem *> fileSystemItems, const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view)
{
    qDebug() << "AppWindow::showContextMenu";

    contextMenu->show(winId(), pos, fileSystemItems, viewAspect, view);
}

void AppWindow::defaultAction(const FileSystemItem *fileSystemItem)
{
    qDebug() << "AppWindow::defaultAction";

    contextMenu->defaultAction(winId(), fileSystemItem);
}

void AppWindow::updateTitle(FileSystemItem *item)
{
#ifdef WIN32_FRAMELESS
    QString title = APPLICATION_TITLE;
    if (item != nullptr) {
        title = item->getDisplayName() + " - " + title;
    }
    setWindowTitle(title);
#else
    Q_UNUSED(item)
#endif
}

bool AppWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    quint32 id {};

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

    return MAINWINDOW::nativeEvent(eventType, message, result);
}

void AppWindow::setupGui()
{
#ifndef WIN32_FRAMELESS
    QWidget *cWidget = new QWidget(this);
    cWidget->setObjectName(QString::fromUtf8("centralwidget"));
    QVBoxLayout *verticalLayout = new QVBoxLayout(cWidget);
    verticalLayout->setSpacing(0);
    verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    setCentralWidget(cWidget);
#endif

    QWidget *w;
    if (nExplorers > 1) {
        QSplitter *splitter = new QSplitter(contentWidget());
        splitter->setObjectName(QString::fromUtf8("splitter"));
        splitter->setOrientation(verticalMode ? Qt::Vertical : Qt::Horizontal);
        splitter->setStyleSheet("QSplitter::handle { background: #9fcdb3;  } QSplitter::handle:vertical { height: 1px; }");
        w = splitter;
    } else
        w = contentWidget();

    // Create explorers
    CustomExplorer *cExplorer {};
    for (int i = 0; i < nExplorers; i++) {
        cExplorer = new CustomExplorer(i, w);
        cExplorer->setObjectName(QString::fromUtf8("customExplorer") + QString::number(i));

        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(cExplorer->sizePolicy().hasHeightForWidth());

        cExplorer->setSizePolicy(sizePolicy);
        cExplorer->setFrameShape(QFrame::NoFrame);
        cExplorer->setFrameShadow(QFrame::Raised);
        cExplorer->setLineWidth(0);

        QSplitter *cSplitter = cExplorer->getSplitter();
        splitters.append(cExplorer->getSplitter());
        explorers.append(cExplorer);

        if (verticalMode)
            connect(cExplorer->getTreeView(), &CustomTreeView::resized, this, [this, cSplitter]() { this->resizeOtherSplitter(cSplitter); } );

        cExplorer->initialize(this);
    }

    if (nExplorers == 1) {
        contentWidget()->layout()->addWidget(cExplorer);
    } else {
       QSplitter *splitter = dynamic_cast<QSplitter *>(w);
       splitter->addWidget(cExplorer);
       contentWidget()->layout()->addWidget(splitter);
    }

    // Default main window settings
    setMinimumWidth(getPhysicalPixels(250));

    qDebug() << "AppWindow::setupGui about to restore windows geometry" << this->windowHandle()->screen()->name();

    QString screenName = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_SCREEN).toString();

    for (QScreen *screen : QApplication::screens())
        if (screen->name() == screenName)
            this->windowHandle()->setScreen(screen);

    int x = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_X).toInt();
    int y = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_Y).toInt();
    int height = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_HEIGHT).toInt();
    int width = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_WIDTH).toInt();

    if (x >= 0 && y >= 0)
        move(x, y);
    else {
#ifdef WIN32_FRAMELESS
        // Center the window in the screen
        QRect ag = this->windowHandle()->screen()->availableGeometry();
        QRect wg({}, frameSize().boundedTo(ag.size()));
        move(ag.center() - wg.center());
#endif
    }

    if (height >= 0 && width >= 0)
        resize(width, height);
    else
        resize(getPhysicalPixels(1024), getPhysicalPixels(768));

    if (Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_MAXIMIZED).toBool())
        showMaximized();

    qDebug() << "AppWindow::setupGui windows geometry restored";
}

void AppWindow::saveSettings()
{
    Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_X, x());
    Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_Y, y());
    Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_WIDTH, width());
    Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_HEIGHT, height());
    Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_MAXIMIZED, isMaximized());
    Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_SCREEN, this->windowHandle()->screen()->name());

    Settings::settings->save();
}

void AppWindow::resizeOtherSplitter(QSplitter *splitter)
{
    if (nExplorers > 1) {

        QSplitter *otherSplitter {};
        for (QSplitter *s : splitters)
            if (splitter != s)
                otherSplitter = s;

        if (otherSplitter != nullptr) {
            QList<int> currentSizes = splitter->sizes();
            if (currentSizes.at(0) != otherSplitter->sizes().at(0))
                otherSplitter->setSizes(currentSizes);
        }
    }
}
