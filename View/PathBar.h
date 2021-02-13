#ifndef PATHBAR_H
#define PATHBAR_H

#include <QPushButton>
#include <QFrame>

#include "Model/filesystemmodel.h"
#include "View/PathBarButton.h"
#include "View/PathWidget.h"

class PathBar : public QFrame
{
    Q_OBJECT
public:
    explicit PathBar(QWidget *parent = nullptr);

public slots:
    void setTabModel(FileSystemModel *tabModel);
    void setTreeModel(FileSystemModel *treeModel);
    void selectTreeIndex(QModelIndex& selectedIndex);
    void menuSelected(QAction *action);
    void selectedIndex(const QModelIndex &index);

signals:
    void rootChange(QString path);

private:
    FileSystemModel *tabModel       {};
    FileSystemModel *treeModel      {};
    PathBarButton *backButton       {};
    PathBarButton *upButton         {};
    PathBarButton *nextButton       {};
    PathWidget *pathWidget          {};

    PathBarButton *createButton(QIcon icon, QString objectName);

private slots:
    void buttonClicked();



};

#endif // PATHBAR_H
