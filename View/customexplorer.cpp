/*
 * Copyright (C) 2019, 2020 Naikel Aparicio. All rights reserved.
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

#include <QDebug>

#include "customexplorer.h"
#include "once.h"

CustomExplorer::CustomExplorer(QWidget *parent, Qt::WindowFlags f) : QFrame(parent, f)
{

}

void CustomExplorer::initialize(MainWindow *mainWindow, CustomTreeView *treeView, CustomTabWidget *tabWidget)
{
    this->mainWindow = mainWindow;
    this->treeView = treeView;
    this->tabWidget = tabWidget;

    FileSystemModel *fileSystemModel = new FileSystemModel(FileInfoRetriever::Tree, this);
    fileSystemModel->setRoot("/");
    treeView->setModel(fileSystemModel);

    // Handling of single selections
    connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged, mainWindow, &MainWindow::changeTitle);
    connect(treeView, &CustomTreeView::clicked, tabWidget, &CustomTabWidget::setViewIndex);

    // Auto expand & select on view select / Auto tree select and view change on collapse
    connect(tabWidget, &CustomTabWidget::rootChanged, this, &CustomExplorer::expandAndSelectRelative);
    connect(treeView, &CustomTreeView::collapsed, this, &CustomExplorer::collapseAndSelect);

    // Tab handling
    connect(tabWidget, &CustomTabWidget::newTabRequested, this, &CustomExplorer::newTabRequested);
    connect(tabWidget, &CustomTabWidget::currentChanged, this, &CustomExplorer::tabChanged);

    // Context Menu
    connect(tabWidget, &CustomTabWidget::contextMenuRequestedForItems, mainWindow, &MainWindow::showContextMenu);
    connect(treeView, &CustomTreeView::contextMenuRequestedForItems, mainWindow, &MainWindow::showContextMenu);

    // Actions
    connect(tabWidget, &CustomTabWidget::defaultActionRequestedForItem, mainWindow, &MainWindow::defaultAction);
}

/*!
 * \brief Expands current tree item and selects the subfolder item with the new path.
 * \param path a new path that is a subfolder of the current path.
 *
 * Expands the current tree item and looks for the item with the new path inside of it and selects it.
 *
 * For this to work the new path has to be a subfolder of the current path, in other words
 * it has to be a relative of the current path.
 *
 * If the subfolder items have not been fetched yet, this function triggers a new fetch in
 * the background and it's called again after it finishes.
 *
 *  \sa CustomExplorer::expandAndSelectAbsolute
 */
void CustomExplorer::expandAndSelectRelative(QString path)
{
    qDebug() << "CustomExplorer::expandAndSelectRelative" << path;
    FileSystemModel *fileSystemModel = reinterpret_cast<FileSystemModel *>(treeView->model());
    QModelIndex parent = treeView->selectedItem();
    if (parent.isValid()) {
        FileSystemItem *parentItem = static_cast<FileSystemItem*>(parent.internalPointer());
        qDebug() << "CustomExplorer::expandAndSelect parent" << parentItem->getPath();
        if (!parentItem->areAllChildrenFetched()) {

            // Call this function again after all items are fetched
            qDebug() << "CustomExplorer::expandAndSelectRelative will be called again after fetch";
            Once::connect(fileSystemModel, &FileSystemModel::fetchFinished, this, [this, path]() { this->expandAndSelectRelative(path); });

            // This will trigger the background fetching
            treeView->expand(parent);

        } else {

            qDebug() << "CustomExplorer::expandAndSelectRelative all children of" << parentItem->getPath() << "are fetched";

            if (!treeView->isExpanded(parent)) {
                treeView->expand(parent);
            }
            QModelIndex index = fileSystemModel->relativeIndex(path, parent);
            treeView->selectIndex(index);
        }
    }
}

/*!
 * \brief Expands the current tree and selects the item with the new path.
 * \param path a new path that can be anywhere.
 *
 * Expands the tree until the item with the new path is visible and selects it.
 *
 * The path can be anywhere.
 *
 * If the subfolder items have not been fetched yet, this function triggers a new fetch in
 * the background and it's called again after it finishes.  This means this function can be
 * called several times until all children that are needed have been fetched.
 *
 * This function is only used when the user selects a new tab and the tree view has to select
 * that tab's current folder.
 *
 * \sa CustomExplorer::expandAndSelectRelative
 */
void CustomExplorer::expandAndSelectAbsolute(QString path)
{
    qDebug() << "CustomExplorer::expandAndSelectAbsolute" << path;

    FileSystemModel *treeViewModel = reinterpret_cast<FileSystemModel *>(treeView->model());

    FileSystemItem *parent = treeViewModel->getRoot();
    FileSystemItem *child {};

    QModelIndex parentIndex = treeViewModel->index(0, 0, QModelIndex());
    QModelIndex childIndex {};

    if (path == "/") {
        treeView->selectIndex(parentIndex);
        return;
    }

    QStringList pathList = path.split(treeViewModel->separator());
    QString absolutePath = QString();

    for (auto folder : pathList) {
        if (!folder.isEmpty()) {

            // We need to check that the parent has all children fetched and it's expanded
            if (!parent->areAllChildrenFetched()) {

                qDebug() << "CustomExplorer::expandAndSelectAbsolute will be called again after fetch";
                Once::connect(treeViewModel, &FileSystemModel::fetchFinished, this, [this, path]() { this->expandAndSelectAbsolute(path); });

                // This will trigger the background fetching
                treeView->expand(parentIndex);
                return;

            } else {
                if (!treeView->isExpanded(parentIndex)) {
                    treeView->expand(parentIndex);
                }

                // Parent has all children fetched and expanded at this point

                if (absolutePath.isEmpty())
                    absolutePath = folder + treeViewModel->separator();
                else if (absolutePath.at(absolutePath.length()-1) != treeViewModel->separator())
                    absolutePath += treeViewModel->separator() + folder;
                else
                    absolutePath += folder;

                child = parent->getChild(absolutePath);
                childIndex = treeViewModel->index(parent->childRow(child), 0, parentIndex);

                // Prepare for next iteration
                parent = child;
                parentIndex = childIndex;
            }
        }
    }
    treeView->selectIndex(childIndex);
}

/*!
 * \brief Collapse the tree and optionally selects the collapsed item.
 * \param index item that is going to be collapsed.
 *
 * Collapse the tree and selects the collapsed item.  The collapsed item is only selected if the current
 * selected item is not visible anymore.
 */
void CustomExplorer::collapseAndSelect(QModelIndex index)
{
    qDebug() << "CustomExplorer::collapseAndSelect";

    // Only change the selection if we're hiding an item
    if (index.isValid() && treeView->selectedItem().isValid())
    {
        for (QModelIndex i = treeView->selectedItem().parent(); i.isValid() ; i = i.parent())
        {
            if (i.internalPointer() == index.internalPointer()) {
                FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
                tabWidget->changeRootPath(fileSystemItem->getPath());
                treeView->selectIndex(index);

                // ToDo: Probably we would have to tell the TreeView's FileSystemModel here to forget all the index's children
                // That way they will get reloaded from disk next time the user selects it
            }
        }
    }
}

/*!
 * \brief Creates a new tab after a user's request.
 *
 * Creates a new tab after a request was received usually because the user clicked on the add new tab icon.
 */
void CustomExplorer::newTabRequested()
{
    QModelIndex index = treeView->selectedItem();
    if (index.isValid()) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
        tabWidget->addNewTab(fileSystemItem->getPath());
    }

}

/*!
 * \brief Selects the new folder after the user changed to a different tab
 * \param index index of the newly tab selected
 *
 * Tells the Tree View to select the folder of the tab selected.  This function will call
 * CustomExplorer::expandAndSelectAbsolute to look for the folder in the tree and select it.
 *
 * \sa CustomExplorer::expandAndSelectAbsolute
 */
void CustomExplorer::tabChanged(int index)
{
    qDebug() << "MainWindow::tabChanged to index" << index;
    DetailedView *detailedView = static_cast<DetailedView *>(tabWidget->currentWidget());
    FileSystemModel *viewModel = static_cast<FileSystemModel *>(detailedView->model());
    QString viewModelCurrentPath = viewModel->getRoot()->getPath();
    expandAndSelectAbsolute(viewModelCurrentPath);
}
