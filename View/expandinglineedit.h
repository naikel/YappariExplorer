#ifndef EXPANDINGLINEEDIT_H
#define EXPANDINGLINEEDIT_H

#include <QModelIndex>
#include <QLineEdit>
#include <QObject>

#include "Shell/filesystemitem.h"

class ExpandingLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    ExpandingLineEdit(const QModelIndex &index, QWidget *parent = nullptr);

public slots:
    void resizeToContents();
    void fixInitialSelection();

private:
    FileSystemItem *item;
    bool initialSelection = true;

};

#endif // EXPANDINGLINEEDIT_H
