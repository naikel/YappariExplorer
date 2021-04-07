#include <QDebug>

#include "ExpandingLineEdit.h"

#include "Model/FileSystemModel.h"

ExpandingLineEdit::ExpandingLineEdit(const QModelIndex &index, QWidget *parent) : QLineEdit(parent), index(index)
{
    connect(this, &ExpandingLineEdit::textChanged, this, &ExpandingLineEdit::resizeToContents);
#ifdef Q_OS_WIN
    connect(this, &ExpandingLineEdit::selectionChanged, this, &ExpandingLineEdit::fixInitialSelection);
#endif

}

void ExpandingLineEdit::resizeToContents()
{
    if (parentWidget() != nullptr) {

        QPoint position = pos();

        int hintWidth = minimumWidth() + fontMetrics().horizontalAdvance(displayText()) + (fontMetrics().horizontalAdvance(QChar(' ')) * 3);
        setMaximumWidth(hintWidth);

        if (isRightToLeft())
            move(position.x() - hintWidth, position.y());

        resize(hintWidth, height());
    }
}

#ifdef Q_OS_WIN
void ExpandingLineEdit::fixInitialSelection()
{
    if (initialSelection && index.isValid()) {

        initialSelection = false;
        int extWidth = index.data(FileSystemModel::ExtensionRole).toString().size();

        // Exclude the dot from the selection if there was an extension
        if (extWidth)
            extWidth++;

        setSelection(0, index.data(Qt::DisplayRole).toString().size() - extWidth);
        qDebug() << "Selection" << index.data(Qt::DisplayRole).toString().size() - extWidth;
    }

}
#endif
