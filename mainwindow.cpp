#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "filesystemmodel.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Yappari Explorer");

    FileSystemModel *fileSystemModel = new FileSystemModel(this);

    ui->treeView->setModel(fileSystemModel);
}

MainWindow::~MainWindow()
{
    delete ui;
}

