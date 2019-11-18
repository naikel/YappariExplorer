#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QItemSelection>
#include <QMainWindow>
#include <QModelIndex>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void expandAndSelectRelative(QString path);
    void expandAndSelectAbsolute(QString path);
    void collapseAndSelect(QModelIndex index);
    void changeTitle(const QItemSelection &selected, const QItemSelection &deselected);
    void newTabRequested();
    void tabChanged(int index);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
