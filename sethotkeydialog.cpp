#include "sethotkeydialog.h"

SetHotkeyDialog::SetHotkeyDialog(QWidget *parent, QListWidgetItem* item) :
    QDialog(parent)
{
    this->item = item;

    setWindowTitle("Set Hotkey");

    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* itemLabel = new QLabel(this);
    itemLabel->setText("Setting hotkey for " + item->text());

    layout->addWidget(itemLabel);

    QLabel* infoLabel = new QLabel(this);
    infoLabel->setText("Input your hotkey here:");

    layout->addWidget(infoLabel);

    edit = new CustomKeySequenceEdit();
    auto current = item->data(1).toString();
    if (!current.isNull()) {
        edit->setKeySequence(QKeySequence(current));
    }

    layout->addWidget(edit);


    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    layout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    //connect(edit, SIGNAL(editingFinished()), this, SLOT(truncateShortcut()));
}

// Thanks to https://stackoverflow.com/a/38424451
void SetHotkeyDialog::truncateShortcut()
{
    int value = edit->keySequence()[0];
    QKeySequence shortcut(value);
    edit->setKeySequence(shortcut);
}

void SetHotkeyDialog::accept()
{
    if (edit->keySequence().isEmpty()) {
        item->setData(1, QVariant());
    } else {
        item->setData(1, edit->keySequence().toString());
    }
    QDialog::accept();
}
