/*
 * Copyright (C) 2019, 2020, 2021 Naikel Aparicio. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ''AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the author and should not be interpreted as representing
 * official policies, either expressed or implied, of the copyright holder.
 */

#include <QApplication>
#include <QHeaderView>
#include <QJsonArray>
#include <QResizeEvent>
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

#else
#include "Shell/Unix/unixcontextmenu.h"
#define PlatformContextMenu(PARENT)     UnixContextMenu(PARENT)

#endif

AppWindow *AppWindow::appWindow                        {};

/*!
 * \brief The constructor.
 * \param parent The QObject parent.
 *
 * Constructs a new FileSystemModel object.
 *
 * This constructor sets up the window, creates the Context Menu object
 * and the Settings::settings object.
 *
 * \sa saveSettings()
 */
AppWindow::AppWindow(QWidget *parent) : MAINWINDOW(parent)
{
    appWindow = this;

#ifdef Q_OS_WIN
    nextId = WM_USER + 200;
#endif

    Settings::settings = new Settings();
    connect(QApplication::instance(), &QApplication::aboutToQuit, this, &AppWindow::saveSettings);

    // Application settings
    setWindowIcon(QIcon(":/icons/app2.png"));
    setWindowTitle(APPLICATION_TITLE);

    contextMenu = new PlatformContextMenu(this);

    windowId = winId();

    setupGui();
}

/*!
 * \brief The denstructor.
 *
 */
AppWindow::~AppWindow()
{
    qDebug() << "AppWindow::~AppWindow Destroyed";
}

/*!
 * \brief Registers the DirectoryWatcher \a watcher and returns the registration id.
 * \param watcher a DirectoryWatcher object to be registered.
 * \return an id as a quint32
 *
 * This registers a DirectoryWatcher with this window.
 *
 * Since Windows sends notifications through the main window, this window needs to know which DirectoryWatcher
 * is watching that object, and then sends the notification to that DirectoryWatcher.  For this the
 * DirectoryWatcher must be registered with this window first.
 *
 * \sa nativeEvent()
 */
quint32 AppWindow::registerWatcher(DirectoryWatcher *watcher)
{
    regMutex.lock();

    quint32 id = nextId++;
    watchers.insert(id, watcher);

    qDebug() << "AppWindow::registerWatcher watcher registered with id =" << id;

    regMutex.unlock();
    return id;
}

/*!
 * \brief Returns a pointer to this instance.
 * \return an AppWindow pointer.
 *
 * Returns a pointer to this AppWindow object.
 */
AppWindow *AppWindow::instance()
{
    return appWindow;
}

/*!
 * \brief Returns the content widget of this window.
 * \return a QWidget pointer.
 *
 * Returns a pointer to the content widget of this window.
 *
 * If this is a normal window, this is equivalent to the centralWidget() function.
 * If this is a frameless window under Windows, it will return the content widget that
 * defines the client area.
 *
 * \sa QMainWindow::centralWidget()
 * \sa WinFramelessWindow::contentWidget()
 */
QWidget *AppWindow::contentWidget()
{
    return CENTRALWIDGET;
}

/*!
 * \brief Shows the Context Menu in the point \a pos.
 * \param pos a QPoint indicating the top-left point where the menu will be shown.
 * \param fileSystemItems a QList of FileSystemItem pointers of the files selected.
 * \param viewAspect indicates if it's a background menu or a menu of selected items.
 * \param view the view as a QAbstractItemView object that requested the menu.
 *
 * Shows the Context Menu in the point \a pos.
 *
 * If \a viewAspect is ContextMenu::SelectedItems then a menu of the \a fileSystemItems
 * items will be shown, otherwise it is the background menu.
 *
 * \sa ContextMenu::show()
 */
void AppWindow::showContextMenu(const QPoint &pos, const QModelIndexList &indexList, const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view)
{
    qDebug() << "AppWindow::showContextMenu";

    contextMenu->show(winId(), pos, indexList, viewAspect, view);
}

/*!
 * \brief Executes the default action for the FileSystemItem object \afileSystemItem
 * \param fileSystemItem a FileSystemItem pointer.
 *
 * Executes the default action for a FileSystemItem object.
 *
 * \sa ContextMenu::defaultActionForIndex()
 */
void AppWindow::defaultActionForIndex(const QModelIndex &index)
{
    qDebug() << "AppWindow::defaultAction";

    contextMenu->defaultActionForIndex(winId(), index);
}

/*!
 * \brief Changes the window title in the title bar with the display name of a FileSystemItem object.
 * \param a FileSystemItem pointer.
 *
 * This function changes the title of this window in the title bar with the display name of a
 * FileSystemItem object and the application name.
 *
 * This only works if this window is not a Frameless Window.
 */
void AppWindow::updateTitle(const QModelIndex &index)
{
#ifndef WIN32_FRAMELESS
    QString title = APPLICATION_TITLE;
    if (index.isValid()) {
        title = index.data(Qt::DisplayRole).toString() + " - " + title;
    }
    setWindowTitle(title);
#else
    Q_UNUSED(index)
#endif
}

/*!
 * \brief Handles a native event sent to this window.
 * \param eventType a QByteArray that identifies the type of the event.
 * \param message a void pointer to a native message.
 * \param result a long pointer that will contain the result of the handling.
 * \return true if the event was handled by this function.
 *
 * This function checks if the event is a notify event or if it's a context menu event
 * under Windows.
 *
 * If it's a notify event then if will look from the DirectoryWatcher objects registered
 * to this window which one this event belongs to and send it to the respective object.
 *
 * If it's a context menu event then it sends the event to the ContextMenu object.
 */
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

/*!
 * \brief Handles a resize event.
 * \param event a QResizeEvent pointer.
 *
 * This event handler stores the new size of the window in case that the user decides
 * to maximize the window and then restore it, to restore to the original size.
 */
void AppWindow::resizeEvent(QResizeEvent *event)
{
    MAINWINDOW::resizeEvent(event);

    if (!isMaximized() && !event->size().isEmpty())
        prevSize = event->size();
}

/*!
 * \brief Gets the winId.
 * \return the WId of this window.
 *
 * This function allows different threads to get the WId of this window. If other threads
 * call AppWindow::instance()->winId() they would crash with the error:
 *
 * QCoreApplication::sendEvent: "Cannot send events to objects owned by a different thread."
 */
WId AppWindow::getWindowId() const
{
    return windowId;
}

/*!
 * \brief Sets up the GUI.
 *
 * This function sets up the whole window. It creates the CustomExplorer objects,
 * the main QSplitter if needed, and restores everything like it was in the
 * previous session.
 */
void AppWindow::setupGui()
{
#ifndef WIN32_FRAMELESS
    QWidget *cWidget = new QWidget(this);
    cWidget->setObjectName(QString::fromUtf8("centralwidget"));
    QVBoxLayout *verticalLayout = new QVBoxLayout(cWidget);
    verticalLayout->setSpacing(0);
    verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
    verticalLayout->setContentsMargins(0, 0, 0, 0);

#ifdef Q_OS_WIN
    QFrame *line = new QFrame(cWidget);
    QSizePolicy linePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    line->setSizePolicy(linePolicy);
    line->setMinimumSize(QSize(1, 1));
    line->setObjectName(QString::fromUtf8("topLine"));
    line->setStyleSheet("#topLine { background: #9fcdb3;  }");
    verticalLayout->addWidget(line);
#endif

    setCentralWidget(cWidget);
#endif

    QWidget *w;
    nExplorers = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_EXPLORERS).toInt();
    if (nExplorers > 1) {
        QSplitter *splitter = new QSplitter(contentWidget());
        splitter->setObjectName(QString::fromUtf8("splitter"));
        splitter->setOrientation(verticalMode ? Qt::Vertical : Qt::Horizontal);
        splitter->setStyleSheet("QSplitter::handle { background: #9fcdb3;  } QSplitter::handle:vertical { height: 1px; }");
        verticalLayout->addWidget(w);
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
    }

    if (nExplorers == 1) {
        contentWidget()->layout()->addWidget(cExplorer);
    } else {
       QSplitter *splitter = dynamic_cast<QSplitter *>(w);
       splitter->addWidget(cExplorer);
       contentWidget()->layout()->addWidget(splitter);

       QJsonArray sizes = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_SPLITTER_SIZES).toArray();
       if (!sizes.isEmpty()) {
           QList<int> intSizes;
           for (QJsonValue size : sizes)
               intSizes.append(size.toInt());

           splitter->setSizes(intSizes);
       }
    }

    // Default main window settings
    setMinimumWidth(getPhysicalPixels(350));

    QWindow *window = this->windowHandle();

    if (window != nullptr) {
        qDebug() << "AppWindow::setupGui about to restore windows geometry" << window->screen()->name();

        QString screenName = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_SCREEN).toString();

        for (QScreen *screen : QApplication::screens())
            if (screen->name() == screenName)
                window->setScreen(screen);
    }

    int x = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_X).toInt();
    int y = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_Y).toInt();
    int height = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_HEIGHT).toInt();
    int width = Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_WIDTH).toInt();

    prevSize = QSize(width, height);

    if (height >= 0 && width >= 0)
        resize(width, height);
    else
        resize(getPhysicalPixels(1024), getPhysicalPixels(768));

    if (x >= 0)
        move(x, y);
    else {
#ifdef WIN32_FRAMELESS
        // Center the window in the screen
        QRect ag = this->windowHandle()->screen()->availableGeometry();
        QRect wg({}, frameSize().boundedTo(ag.size()));
        move(ag.center() - wg.center());
#endif
    }

    if (Settings::settings->readGlobalSetting(SETTINGS_GLOBAL_MAXIMIZED).toBool()) {
        showMaximized();
    }


    qDebug() << "AppWindow::setupGui windows geometry restored";
}

/*!
 * \brief Saves current settings.
 *
 * This function saves the state of every object in the window and deletes
 * the Settings::settings object.
 *
 * \sa AppWindow::AppWindow()
 */
void AppWindow::saveSettings()
{
    for (const CustomExplorer *cExplorer : explorers)
       cExplorer->saveSettings();

    Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_X, x());
    Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_Y, y());
    Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_SCREEN, this->windowHandle()->screen()->name());

    Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_MAXIMIZED, isMaximized());
    if (isMaximized()) {
        Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_WIDTH, prevSize.width());
        Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_HEIGHT, prevSize.height());
    } else {
        Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_WIDTH, width());
        Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_HEIGHT, height());
    }

    Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_EXPLORERS, nExplorers);
    if (nExplorers > 1) {

        QLayout *layout = contentWidget()->layout();
        for (int i = 0; i < layout->count(); i++) {
            QWidget *w = layout->itemAt(i)->widget();

            if (QString(w->metaObject()->className()) == "QSplitter") {

                QSplitter *splitter = dynamic_cast<QSplitter *>(w);
                QJsonArray sizes;
                for (int size : splitter->sizes())
                    sizes.append(size);
                Settings::settings->saveGlobalSetting(SETTINGS_GLOBAL_SPLITTER_SIZES, sizes);
            }
        }
    }

    Settings::settings->save();

    delete Settings::settings;
}

/*!
 * \brief Resizes the splitter of the other CustomExplorer
 *
 * If a vertical splitter from a CustomExplorer object is resized, this function is called
 * so the vertical splitter from the other CustomExplorer has the same size.
 *
 * This function only has meaning if the number of explorers is greater than one.
 *
 */
void AppWindow::resizeOtherSplitter(QSplitter *splitter)
{
    if (nExplorers > 1) {

        for (QSplitter *otherSplitter : splitters)
            if (splitter != otherSplitter) {
                QList<int> currentSizes = splitter->sizes();
                if (currentSizes.at(0) != otherSplitter->sizes().at(0))
                    otherSplitter->setSizes(currentSizes);
                break;
            }
    }
}
