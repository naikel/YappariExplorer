#include <QApplication>
#include <QMimeData>
#include <QKeyEvent>
#include <QDebug>
#include <QDrag>

#include "basetreeview.h"

#include "once.h"
#include "Model/filesystemmodel.h"

/*!
 * \brief The constructor.
 * \param parent The QWidget parent.
 *
 * The constructor only enables the context menu.
 */
BaseTreeView::BaseTreeView(QWidget *parent) : QTreeView(parent)
{
    setAcceptDrops(true);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false);
    setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeView::customContextMenuRequested, this, &BaseTreeView::contextMenuRequested);
}

/*!
 * \brief Sets the model for the view.
 * \param model Model for the view.
 *
 * Sets the model for the view to present.  The model is a FileSystemModel.
 *
 * After the model has finished fetching the initial items the function BaseTreeView::initialize will be called
 * to finish initialization of the view.
 *
 * This function also configures the mouse cursor to show a busy icon when the model is fetching new items.
 *
 * \sa FileSystemModel::initialize
 */
void BaseTreeView::setModel(QAbstractItemModel *model)
{
    FileSystemModel *fileSystemModel = reinterpret_cast<FileSystemModel *>(model);
    Once::connect(fileSystemModel, &FileSystemModel::fetchFinished, this, &BaseTreeView::initialize);

    connect(fileSystemModel, &FileSystemModel::fetchStarted, this, &BaseTreeView::setBusyCursor);
    connect(fileSystemModel, &FileSystemModel::fetchFinished, this, &BaseTreeView::setNormalCursor);

    QTreeView::setModel(fileSystemModel);
}

/*!
 * \brief Initializes the view.
 *
 * Initializes the view. This function is called after the model has fetched the initial items.
 *
 * This function does nothing in this class.  Subclasses should reimplement this function and put here the
 * custom configuration of their view.
 */
void BaseTreeView::initialize()
{
    qDebug() << "BaseTreeView::initialize";
}

/*!
 * \brief Sets the cursor icon to normal.
 */
void BaseTreeView::setNormalCursor()
{
    setCursor(Qt::ArrowCursor);
}

/*!
 * \brief Sets the cursor icon to busy.
 */
void BaseTreeView::setBusyCursor()
{
    setCursor(Qt::BusyCursor);
}

/*!
 * \brief A key press event occurred.
 * \param event the QKeyEvent that represents the key event that was sent to the widget.
 *
 * This function handles the default key actions like Enter, Backspace, etc.
 */
void BaseTreeView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {

        case Qt::Key_Select:
        case Qt::Key_Enter:
        case Qt::Key_Return:
            selectEvent();
            event->accept();
            break;

        case Qt::Key_Back:
        case Qt::Key_Backspace:
            backEvent();
            event->accept();
            break;

        default:
            QTreeView::keyPressEvent(event);
    }
}

/*!
 * \brief A mouse button was clicked.
 * \param event the QMouseEvent that represents the mouse button that was sent to the widget.
 *
 * This function handles the default mouse button actions like Back, Forward, etc.
 */
void BaseTreeView::mousePressEvent(QMouseEvent *event)
{
    switch (event->button()) {
        case Qt::BackButton:
            backEvent();
            event->accept();
            break;
        default:
            QTreeView::mousePressEvent(event);
    }
}

void BaseTreeView::startDrag(Qt::DropActions supportedActions)
{
    Q_UNUSED(supportedActions)

    qDebug() << "BaseTreeView:startDrag";

    Qt::DropActions dropActions = getFileSystemModel()->supportedDragActionsForIndexes(selectedIndexes());
    QTreeView::startDrag(dropActions);
}

void BaseTreeView::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug() << "BaseTreeView::dragEnterEvent";

    // This will set the QAbstractItemView state to DraggingState and allows autoexpand of trees
    QTreeView::dragEnterEvent(event);

    if (event->mimeData()->hasFormat("text/uri-list")) {
        event->acceptProposedAction();
    } else
        event->ignore();
}

void BaseTreeView::dragMoveEvent(QDragMoveEvent *event)
{
    // Process default events (autoexpand tree on drag, keyboard modifiers and highlight of destination)
    QTreeView::dragMoveEvent(event);

    // If no keyboard modifiers are being pressed, select the default action for the drop target
    if (event->isAccepted() && !event->keyboardModifiers()) {
        event->setDropAction(getFileSystemModel()->defaultDropActionForIndex(indexAt(event->pos()), event->mimeData(), event->possibleActions()));
    }
}

void BaseTreeView::dropEvent(QDropEvent *event)
{
    qDebug() << "BaseTreeView::dropEvent" << event->dropAction();

    // If no keyboard modifiers are being pressed, select the default action for the drop target
    if (!event->keyboardModifiers()) {
        event->setDropAction(getFileSystemModel()->defaultDropActionForIndex(indexAt(event->pos()), event->mimeData(), event->possibleActions()));
    }
    QTreeView::dropEvent(event);
}

/*!
 * \brief Handles a Select/Enter/Return event.
 *
 * This function does nothing. Subclasses must implement this function.
 */
void BaseTreeView::selectEvent()
{
    qDebug() << "BaseTreeView::selectEvent";
}

/*!
 * \brief Handles a Back/Backspace event.
 *
 * This function does nothing. Subclasses must implement this function.
 */
void BaseTreeView::backEvent()
{
    qDebug() << "BaseTreeView::backEvent";
}

bool BaseTreeView::isDragging() const
{
    return state() == DraggingState;
}

/*!
 * \brief A context menu was requested.
 * \param pos the QPoint position where the context menu was requested, relative to the widget.
 *
 * This function determines if the context menu was requested on an item or in the background, and then
 * sends the BaseTreeView::contextMenuRequestedForItems signal.
 */
void BaseTreeView::contextMenuRequested(const QPoint &pos)
{
    QList<FileSystemItem *> fileSystemItems;
    ContextMenu::ContextViewAspect viewAspect;

    // Get current selected item if any
    QModelIndex index = indexAt(pos);
    if (index.isValid() && index.internalPointer() != nullptr) {
        viewAspect = ContextMenu::Selection;
        for (QModelIndex selectedIndex : selectedIndexes()) {
            if (selectedIndex.column() == 0 && selectedIndex.internalPointer() != nullptr) {
                FileSystemItem *fileSystemItem = getFileSystemModel()->getFileSystemItem(selectedIndex);
                fileSystemItems.append(fileSystemItem);
                qDebug() << "BaseTreeView::contextMenuRequested selected for " << fileSystemItem->getPath();
            }
        }
    } else {
        viewAspect = ContextMenu::Background;
        FileSystemItem *fileSystemItem = getFileSystemModel()->getRoot();
        fileSystemItems.append(fileSystemItem);
        qDebug() << "BaseTreeView::contextMenuRequested selected for the background on " << fileSystemItem->getPath();
    }

    emit contextMenuRequestedForItems(mapToGlobal(pos), fileSystemItems, viewAspect);
}
