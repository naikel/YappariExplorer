#ifndef CUSTOMTABWIDGET_H
#define CUSTOMTABWIDGET_H

#include <QTabWidget>

#include "detailedview.h"
#include "filesystemmodel.h"

class CustomTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    CustomTabWidget(QWidget *parent = nullptr);

public slots:
    void changeRootIndex(const QModelIndex &index);
    void changeRootPath(const QString path);
    void doubleClicked(const QModelIndex &index);
    void updateTab();

signals:
    void rootChanged(const QString path);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
};

#endif // CUSTOMTABWIDGET_H
