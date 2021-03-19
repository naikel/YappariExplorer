/*
 * Copyright (C) 2019, 2020 Naikel Aparicio. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ''AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the author and should not be interpreted as representing
 * official policies, either expressed or implied, of the copyright holder.
 */

#ifndef BASETREEVIEW_H
#define BASETREEVIEW_H

#include <QTreeView>
#include <QMutex>

#include "Shell/ContextMenu.h"
#include "Shell/FileSystemItem.h"

/*!
 * \brief Base QTreeView class.
 *
 * This is the base QTreeView class for all the tree views of this project.
 *
 * This TreeView has the following features:
 *
 *  - Smooth scrolling: dataChanged signals sent form the model are queued and processed on intervals bundling
 *    them as few as possible, making the processing of several thousand dataChanged signals ar once very efficient.
 *
 *  - Custom selection: To select an item in the first columnthe user has to click exactly above the characters
 *    and not anywhere on the row like the QTreeView implementation. This is done by reimplementing visualRect(),
 *    indexAt() and visualRegionForSelection().
 */
class BaseTreeView : public QTreeView
{
    Q_OBJECT

public:
    BaseTreeView(QWidget *parent = nullptr);
    ~BaseTreeView();

    bool isDragging() const;

    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>()) override;
    QModelIndex indexAt(const QPoint &point) const override;
    QRect visualRect(const QModelIndex &index) const override;
    void storeCurrentSettings();

    QPoint mapToViewport(QPoint pos);
    QPoint mapFromViewport(QPoint pos);

    bool isEditingIndex(const QModelIndex &index) const;
    void edit(FileSystemItem *item);

    void deleteSelectedItems();

    void setPath(const QString &value);
    QString getPath() const;

    QList<int> getColumnsWidth() const;
    void setColumnsWidth(const QList<int> &value);

    int getSortingColumn() const;
    void setSortingColumn(int value);

    Qt::SortOrder getSortOrder() const;
    void setSortOrder(const Qt::SortOrder &value);

    QList<int> getVisualIndexes() const;
    void setVisualIndexes(const QList<int> &value);

signals:
    void contextMenuRequestedForItems(const QPoint &pos, const QModelIndexList &indexList,
                                      const ContextMenu::ContextViewAspect viewAspect, QAbstractItemView *view);
    void viewFocusIndex(const QModelIndex &index);

public slots:
    void contextMenuRequested(const QPoint &pos);
    void setRootIndex(const QModelIndex &index) override;

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void startDrag(Qt::DropActions supportedActions) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    QRegion visualRegionForSelection(const QItemSelection &selection) const override;
    bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event) override;

    virtual void selectEvent();
    virtual void backEvent();
    virtual void forwardEvent();

private:

    QMutex mutex;
    QString path;

    // Loaded Settings
    QList<int> columnsWidth;
    QList<int> visualIndexes;
    int sortingColumn;
    Qt::SortOrder sortOrder;

    // Signals queue for delayed processing
    // It's basically a list of parents that have children that must be updated.
    // Each children is indexed by its row:
    // QMap<parent, QMap<row, child>>
    QMap<QModelIndex, QMap<int, QModelIndex>*> signalsQueue;

    QModelIndex editIndex   {};

private slots:
    void setNormalCursor();
    void setBusyCursor();
    void processQueuedSignals();
    void editorClosed();
    void shouldEdit(QModelIndex sourceIndex);
    void updateRefCounter(QModelIndex index, bool increase);
};

#endif // BASETREEVIEW_H
