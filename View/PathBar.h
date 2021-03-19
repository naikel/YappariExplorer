#ifndef PATHBAR_H
#define PATHBAR_H

#include <QPushButton>
#include <QFrame>

#include "View/Util/FileSystemHistory.h"
#include "View/PathBarButton.h"
#include "View/PathWidget.h"

class PathBar : public QFrame
{
    Q_OBJECT
public:
    explicit PathBar(QWidget *parent = nullptr);

public slots:
    void selectTreeIndex(const QModelIndex& selectedIndex);
    void setHistory(FileSystemHistory *history);

signals:
    void rootIndexChangeRequested(const QModelIndex &index);
    void pathChangeRequested(const QString &path);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    FileSystemHistory *history      {};
    PathBarButton *backButton       {};
    PathBarButton *upButton         {};
    PathBarButton *nextButton       {};
    PathWidget *pathWidget          {};

    PathBarButton *createButton(QIcon icon, QString objectName);

    QString getDisplayName(const QModelIndex &index);

private slots:
    void buttonClicked();
    void selectedIndex(const QModelIndex &index);
    void menuSelected(QAction *action);
};

#endif // PATHBAR_H
