#include <QDebug>

#include "expandinglineedit.h"

ExpandingLineEdit::ExpandingLineEdit(const QModelIndex &index, QWidget *parent) : QLineEdit(parent)
{
    connect(this, &ExpandingLineEdit::textChanged, this, &ExpandingLineEdit::resizeToContents);
#ifdef Q_OS_WIN
    connect(this, &ExpandingLineEdit::selectionChanged, this, &ExpandingLineEdit::fixInitialSelection);
    this->item = reinterpret_cast<FileSystemItem *>(index.internalPointer());
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
    if (initialSelection) {
        initialSelection = false;
        int extWidth = item->getExtension().size();

        // Exclude the dot from the selection if there was an extension
        if (extWidth)
            extWidth++;

        setSelection(0, item->getDisplayName().size() - extWidth);
    }
}
#endif
