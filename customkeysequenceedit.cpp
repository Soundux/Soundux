#include "customkeysequenceedit.h"

#include <QKeyEvent>

CustomKeySequenceEdit::CustomKeySequenceEdit(QWidget *parent) : QKeySequenceEdit(parent) { }

CustomKeySequenceEdit::~CustomKeySequenceEdit() { }

void CustomKeySequenceEdit::keyPressEvent(QKeyEvent *pEvent)
{
    QKeySequenceEdit::keyPressEvent(pEvent);

    int value = keySequence()[0];
    QKeySequence shortcut(value);
    setKeySequence(shortcut);
}
