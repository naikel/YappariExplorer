#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QTabWidget>
#include <QTabBar>
#include <QDebug>

#include "once.h"
#include "filesystemmodel.h"
#include "customtabbar.h"

#define APPLICATION_TITLE   "YappariExplorer"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(":/icons/app2.png"));
    setWindowTitle(APPLICATION_TITLE);

    FileSystemModel *fileSystemModel = new FileSystemModel(FileInfoRetriever::Tree, this);
    fileSystemModel->setRoot("/");

    ui->treeView->setModel(fileSystemModel);

    // Handling of single selections
    connect(ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::changeTitle);
    connect(ui->treeView, &CustomTreeView::clicked, ui->tabWidget, &CustomTabWidget::setViewIndex);

    // Auto expand & select on view select / Auto tree select and view change on collapse
    connect(ui->tabWidget, &CustomTabWidget::rootChanged, this, &MainWindow::expandAndSelectRelative);
    connect(ui->treeView, &CustomTreeView::collapsed, this, &MainWindow::collapseAndSelect);

    // Tab handling
    connect(ui->tabWidget, &CustomTabWidget::newTabRequested, this, &MainWindow::newTabRequested);
    connect(ui->tabWidget, &CustomTabWidget::currentChanged, this, &MainWindow::tabChanged);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::expandAndSelectRelative(QString path)
{
    qDebug() << "MainWindow::expandAndSelectRelative" << path;
    FileSystemModel *fileSystemModel = reinterpret_cast<FileSystemModel *>(ui->treeView->model());
    QModelIndex parent = ui->treeView->selectedItem();
    if (parent.isValid()) {
        FileSystemItem *parentItem = static_cast<FileSystemItem*>(parent.internalPointer());
        qDebug() << "MainWindow::expandAndSelect parent" << parentItem->getPath();
        if (!parentItem->areAllChildrenFetched()) {

            // Call this function again after all items are fetched
            qDebug() << "MainWindow::expandAndSelectRelative will be called again after fetch";
            Once::connect(fileSystemModel, &FileSystemModel::fetchFinished, this, [this, path]() { this->expandAndSelectRelative(path); });

            // This will trigger the background fetching
            ui->treeView->expand(parent);

        } else {

            qDebug() << "MainWindow::expandAndSelectRelative all children of" << parentItem->getPath() << "are fetched";

            if (!ui->treeView->isExpanded(parent)) {
                ui->treeView->expand(parent);
            }
            QModelIndex index = fileSystemModel->relativeIndex(path, parent);
            ui->treeView->selectIndex(index);
        }
    }
}

void MainWindow::expandAndSelectAbsolute(QString path)
{
    qDebug() << "MainWindow::expandAndSelectAbsolute" << path;

    FileSystemModel *treeViewModel = reinterpret_cast<FileSystemModel *>(ui->treeView->model());

    FileSystemItem *parent = treeViewModel->getRoot();
    FileSystemItem *child {};

    QModelIndex parentIndex = treeViewModel->index(0, 0, QModelIndex());
    QModelIndex childIndex {};

    if (path == "/") {
        ui->treeView->selectIndex(parentIndex);
        return;
    }

    QStringList pathList = path.split(treeViewModel->separator());
    QString absolutePath = QString();

    for (auto folder : pathList) {
        if (!folder.isEmpty()) {

            // We need to check that the parent has all children fetched and it's expanded
            if (!parent->areAllChildrenFetched()) {

                qDebug() << "MainWindow::expandAndSelectAbsolute will be called again after fetch";
                Once::connect(treeViewModel, &FileSystemModel::fetchFinished, this, [this, path]() { this->expandAndSelectAbsolute(path); });

                // This will trigger the background fetching
                ui->treeView->expand(parentIndex);
                return;

            } else {
                if (!ui->treeView->isExpanded(parentIndex)) {
                    ui->treeView->expand(parentIndex);
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
    ui->treeView->selectIndex(childIndex);
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
                ui->treeView->selectIndex(index);

                // Probably we would have to tell the TreeView's FileSystemModel here to forget all the index's children
                // That way they will get reloaded from disk next time the user selects it
            }
        }
    }
}

void MainWindow::changeTitle(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected)
    Q_UNUSED(deselected)

    QString title = APPLICATION_TITLE;
    QModelIndex index = ui->treeView->selectedItem();
    if (index.isValid()) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
        title = fileSystemItem->getDisplayName() + " - " + title;
    }
    setWindowTitle(title);
}

void MainWindow::newTabRequested()
{
    QModelIndex index = ui->treeView->selectedItem();
    if (index.isValid()) {
        FileSystemItem *fileSystemItem = static_cast<FileSystemItem*>(index.internalPointer());
        ui->tabWidget->addNewTab(fileSystemItem->getPath());
    }

}

void MainWindow::tabChanged(int index)
{
    qDebug() << "MainWindow::tabChanged to index" << index;
    DetailedView *detailedView = static_cast<DetailedView *>(ui->tabWidget->currentWidget());
    FileSystemModel *viewModel = static_cast<FileSystemModel *>(detailedView->model());
    QString viewModelCurrentPath = viewModel->getRoot()->getPath();
    expandAndSelectAbsolute(viewModelCurrentPath);
}


