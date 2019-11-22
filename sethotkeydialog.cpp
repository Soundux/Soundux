#include "sethotkeydialog.h"

SetHotkeyDialog::SetHotkeyDialog(QWidget *parent, SoundListWidgetItem* item) :
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
    auto current = item->hotkey.toString();
    if (!current.isNull()) {
        edit->setKeySequence(QKeySequence(current));
    }

    layout->addWidget(edit);

    QPushButton* clearButton = new QPushButton("Clear", this);
    layout->addWidget(clearButton);
    connect(clearButton, SIGNAL(pressed()), this, SLOT(on_clearButton_pressed()));


    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    layout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void SetHotkeyDialog::on_clearButton_pressed()
{
    edit->clear();
}

QVariant SetHotkeyDialog::getSequence()
{
    if (edit->keySequence().isEmpty()) {
        return QVariant();
    } else {
        return edit->keySequence().toString();
    }
}

