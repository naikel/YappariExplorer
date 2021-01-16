#ifndef PATHBAR_H
#define PATHBAR_H

#include <QPushButton>
#include <QFrame>

#include "Model/filesystemmodel.h"
#include "View/PathBarButton.h"

class PathBar : public QFrame
{
    Q_OBJECT
public:
    explicit PathBar(QWidget *parent = nullptr);

public slots:
    void setModel(FileSystemModel *model);
    void menuSelected(QAction *action);

signals:
    void rootChange(QString path);

private:
    FileSystemModel *model {};
    PathBarButton *backButton;
    PathBarButton *upButton;
    PathBarButton *nextButton;

    PathBarButton *createButton(QIcon icon, QString objectName);

private slots:
    void buttonClicked();
};

#endif // PATHBAR_H
