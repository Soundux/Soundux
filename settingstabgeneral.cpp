#include "settingstabgeneral.h"

SettingsTabGeneral::SettingsTabGeneral(json _data, SoundPlayback* soundPlayback) : SettingsTab("general", _data)
{
    this->soundPlayback = soundPlayback;
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(0);

    QGridLayout* general_layout = new QGridLayout();
    layout->addLayout(general_layout);

    int row = 0;

    general_layout->addWidget(new QLabel(tr("Language:")), row, 0);
    languageSelection = new QComboBox();
    // TODO
    languageSelection->setDisabled(true);
    languageSelection->addItem(QLocale::languageToString(QLocale("en-US").language()));
    languageSelection->addItem(QLocale::languageToString(QLocale("de-DE").language()));
    general_layout->addWidget(languageSelection, row, 1);

    row++;

    general_layout->addWidget(new QLabel(tr("Stop hotkey:")), row, 0);
    stopHotkey = new CustomKeySequenceEdit(this);
    general_layout->addWidget(stopHotkey, row, 1);

    if (this->data != nullptr) {
       if (this->data.contains("language")) {
           languageSelection->setCurrentText(QString::fromStdString(this->data["language"].get<std::string>()));
       }
       if (this->data.contains("stopHotkey")) {
           stopHotkey->setKeySequence(QKeySequence(QString::fromStdString(this->data["stopHotkey"].get<std::string>())));
           updateStopHotkey();
       }
    }

}

void SettingsTabGeneral::updateStopHotkey()
{
    if (hotkeyStop != nullptr) {
        delete hotkeyStop;
        hotkeyStop = nullptr;
    }

    if (!stopHotkey || stopHotkey->keySequence().isEmpty()) {
        return;
    }

    hotkeyStop = new QHotkey(stopHotkey->keySequence(), true, this);

    if (hotkeyStop->isRegistered())
    {
        connect(hotkeyStop, &QHotkey::activated, this, [=]() {
            soundPlayback->stopSound();
        });
    } else {
        QMessageBox::warning(this, "Could not register " + stopHotkey->keySequence().toString(), "Either the key combination is not valid or it's not possible to use this combination (Maybe another program is using it)", QMessageBox::Ok);
    }
}

json SettingsTabGeneral::tabSettings()
{
    json j;
    j["language"] = languageSelection->currentText().toStdString();
    j["stopHotkey"] = stopHotkey->keySequence().toString().toStdString();

    updateStopHotkey();

    this->data = j;
    return j;
}
