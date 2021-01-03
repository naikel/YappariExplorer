#ifndef PATHBAR_H
#define PATHBAR_H

#include <QPushButton>
#include <QFrame>

#include "Model/filesystemmodel.h"

class PathBar : public QFrame
{
    Q_OBJECT
public:
    explicit PathBar(QWidget *parent = nullptr);

public slots:
    void setModel(FileSystemModel *model);

signals:
    void rootChange(QString path);

private:
    FileSystemModel *model {};
    QPushButton *backButton;
    QPushButton *upButton;
    QPushButton *nextButton;

    QPushButton *createButton(QIcon icon, QString objectName);

private slots:
    void buttonClicked();
};

#endif // PATHBAR_H
