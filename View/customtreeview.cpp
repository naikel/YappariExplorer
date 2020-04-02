#include <QApplication>
#include <QKeyEvent>
#include <QDebug>

#include "once.h"
#include "customtreeview.h"
#include "Model/filesystemmodel.h"

CustomTreeView::CustomTreeView(QWidget *parent) : QTreeView(parent)
{
    // Custom initialization
    setHeaderHidden(true);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setFrameShape(QFrame::NoFrame);

    // Animations don't really work when rows are inserted dynamically
    setAnimated(false);
}

void CustomTreeView::setModel(QAbstractItemModel *model)
{
    FileSystemModel *fileSystemModel = reinterpret_cast<FileSystemModel *>(model);
    Once::connect(fileSystemModel, &FileSystemModel::fetchFinished, this, &CustomTreeView::initialize);

    connect(fileSystemModel, &FileSystemModel::fetchStarted, this, &CustomTreeView::setBusyCursor);
    connect(fileSystemModel, &FileSystemModel::fetchFinished, this, &CustomTreeView::setNormalCursor);

    QTreeView::setModel(model);
}

void CustomTreeView::setRootIndex(const QModelIndex &index)
{
    QTreeView::setRootIndex(index);
}

QModelIndex CustomTreeView::selectedItem()
{
    if (selectionModel()->selectedIndexes().size() > 0) {
        QModelIndex index = selectionModel()->selectedIndexes().at(0);
        if (index.isValid())
            return index;
    }
    return QModelIndex();
}

void CustomTreeView::initialize()
{
    qDebug() << "CustomTreeView::initialize";

    FileSystemModel *fileSystemModel = reinterpret_cast<FileSystemModel *>(model());

    QModelIndex root = fileSystemModel->index(0, 0, QModelIndex());

    // Expand the root and select it
    expand(root);
    selectionModel()->select(root, QItemSelectionModel::SelectCurrent);
}

void CustomTreeView::setNormalCursor()
{
    setCursor(Qt::ArrowCursor);
}

void CustomTreeView::setBusyCursor()
{
    setCursor(Qt::BusyCursor);
}

void CustomTreeView::selectIndex(QModelIndex index)
{
    selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    scrollTo(index);
}

QModelIndex CustomTreeView::moveCursor(QAbstractItemView::CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    qDebug() << "CustomTreeView::moveCursor";
    QModelIndex newIndex = QTreeView::moveCursor(cursorAction, modifiers);
    emit clicked(newIndex);
    return newIndex;
}

void CustomTreeView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {

        case Qt::Key_Select:
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if (currentIndex().isValid()) {
                if (!isExpanded(currentIndex()))
                    expand(currentIndex());
                else
                    collapse(currentIndex());
            }
            event->accept();
            break;

        case Qt::Key_Back:
        case Qt::Key_Backspace:
            qDebug() << "Backspace!";
            event->ignore();
            break;

        default:
            QTreeView::keyPressEvent(event);
    }
}

void CustomTreeView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::BackButton)
        qDebug() << "Back button pressed";
    QTreeView::mousePressEvent(event);
}
