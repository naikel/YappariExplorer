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

#include "Shell/contextmenu.h"
#include "Shell/filesystemitem.h"

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

    void setModel(QAbstractItemModel *model) override;
    bool isDragging() const;

    // inline functions
    inline FileSystemModel *getFileSystemModel() const {
        return static_cast<FileSystemModel *>(model());
    }

    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>()) override;
    QModelIndex indexAt(const QPoint &point) const override;
    QRect visualRect(const QModelIndex &index) const override;

    QPoint mapToViewport(QPoint pos);
    QPoint mapFromViewport(QPoint pos);

    int getDefaultRowHeight() const;

signals:
    void contextMenuRequestedForItems(const QPoint &pos, const QList<FileSystemItem *> fileSystemItems, const ContextMenu::ContextViewAspect viewAspect);

public slots:
    virtual void initialize();
    void setNormalCursor();
    void setBusyCursor();
    void contextMenuRequested(const QPoint &pos);
    void showError(qint32 err, QString errMessage);
    virtual bool setRoot(QString path);
    void processQueuedSignals();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void startDrag(Qt::DropActions supportedActions) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    QRegion visualRegionForSelection(const QItemSelection &selection) const override;

    virtual void selectEvent();
    virtual void backEvent();

private:

    int defaultRowHeight;
    QMutex mutex;

    // Signals queue for delayed processing
    // It's basically a list of parents that have children that must be updated.
    // Each children is indexed by its row:
    // QMap<parent, QMap<row, child>>
    QMap<QModelIndex, QMap<int, QModelIndex>*> signalsQueue;


};

#endif // BASETREEVIEW_H
