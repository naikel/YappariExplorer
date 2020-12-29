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

#ifndef CUSTOMEXPLORER_H
#define CUSTOMEXPLORER_H

#include <QSplitter>
#include <QFrame>

#include "View/customtreeview.h"
#include "View/customtabwidget.h"

#include "Window/AppWindow.h"

/*!
 * \class CustomExplorer
 * \brief This is the Custom Explorer view.
 *
 * An Explorer is a widget (a QFrame in this project) that has a Tree View and a File View.
 */
class CustomExplorer : public QFrame
{
public:
    CustomExplorer(int nExplorer, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    void initialize(AppWindow *mainWindow);

    QSplitter *getSplitter() const;

    CustomTreeView *getTreeView() const;
    CustomTabWidget *getTabWidget() const;

public slots:
    bool treeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void expandAndSelectRelative(QString path);
    void expandAndSelectAbsolute(QString path);
    void collapseAndSelect(QModelIndex index);
    void newTabRequested();
    void tabChanged(int index);
    void rootChangeFailed(QString path);

private:
    CustomTreeView *treeView;
    CustomTabWidget *tabWidget;
    QSplitter *splitter;

    void setupGui(int nExplorer);
};

#endif // CUSTOMEXPLORER_H
