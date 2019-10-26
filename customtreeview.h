#ifndef CUSTOMTREEVIEW_H
#define CUSTOMTREEVIEW_H

#include <QTreeView>

class CustomTreeView : public QTreeView
{
    Q_OBJECT

public:
    CustomTreeView(QWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;
    void setRootIndex(const QModelIndex &index) override;
    QModelIndex selectedItem();

public slots:

    void initialize();
    void setNormalCursor();
    void setBusyCursor();

protected:

    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    bool initialized {false};

};

#endif // CUSTOMTREEVIEW_H
