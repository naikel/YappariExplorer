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
    void addNewTab(const QString path);

public slots:
    void setViewIndex(const QModelIndex &index);
    void changeRootPath(const QString path);
    void doubleClicked(const QModelIndex &index);
    void updateTab();
    void newTabClicked();
    void closeTab(int index);

signals:
    void rootChanged(const QString path);
    void newTabRequested();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
};

#endif // CUSTOMTABWIDGET_H
