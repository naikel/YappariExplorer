#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QTabWidget>
#include <QTabBar>
#include <QDebug>

#include "once.h"
#include "filesystemmodel.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Yappari Explorer");

    FileSystemModel *fileSystemModel = new FileSystemModel(FileInfoRetriever::Tree, this);
    fileSystemModel->setRoot("/");

    ui->treeView->setModel(fileSystemModel);

    connect(ui->treeView, &CustomTreeView::clicked, ui->tabWidget, &CustomTabWidget::changeRootIndex);
    connect(ui->tabWidget, &CustomTabWidget::rootChanged, this, &MainWindow::expandAndSelect);
    connect(ui->treeView, &CustomTreeView::collapsed, this, &MainWindow::collapseAndSelect);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::expandAndSelect(QString path)
{
    qDebug() << "MainWindow::expandAndSelect";
    FileSystemModel *fileSystemModel = reinterpret_cast<FileSystemModel *>(ui->treeView->model());
    QModelIndex parent = ui->treeView->selectedItem();
    if (parent.isValid()) {
        FileSystemItem *parentItem = static_cast<FileSystemItem*>(parent.internalPointer());
        if (!parentItem->areAllChildrenFetched()) {

            // Call this function again after all items are fetched
            qDebug() << "MainWindow::expandAndSelect will be called again after fetch";
            Once::connect(fileSystemModel, &FileSystemModel::fetchFinished, this, [this, path]() { this->expandAndSelect(path); });

            // This will trigger the background fetching
            ui->treeView->expand(parent);

        } else {
            if (!ui->treeView->isExpanded(parent))
                ui->treeView->expand(parent);
            QModelIndex index = fileSystemModel->relativeIndex(path, parent);
            ui->treeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
            ui->treeView->scrollTo(index);
        }
    }
}

void MainWindow::collapseAndSelect(QModelIndex index)
{
    qDebug() << "MainWindow::collapseAndSelect";

    // Only change the selection if we're hiding an item
    if (index.isValid() && ui->treeView->selectedItem().isValid())
    {
        for (QModelIndex i = ui->treeView->selectedItem().parent(); i.isValid() ; i = i.parent())
        {
            if (i.internalPointer() == index.internalPointer()) {
                FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
                ui->tabWidget->changeRootPath(fileSystemItem->getPath());
                ui->treeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);

                // Probably we would have to tell the TreeView's FileSystemModel here to forget all the index's children
                // That way they will get reloaded from disk next time the user selects it
            }
        }
    }
}


