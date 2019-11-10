#ifndef SETTINGSTABGENERAL_H
#define SETTINGSTABGENERAL_H

#include <QWidget>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QHotkey>

#include <customkeysequenceedit.h>

#include <settingstab.h>
#include <soundplayback.h>

class SettingsTabGeneral : public SettingsTab
{
    Q_OBJECT
public:
    explicit SettingsTabGeneral(json _data, SoundPlayback* soundPlayback);
    virtual json tabSettings() override;

private:
  QComboBox* languageSelection;
  CustomKeySequenceEdit* stopHotkey;
  SoundPlayback* soundPlayback;
  QHotkey* hotkeyStop = nullptr;

  void updateStopHotkey();
};

#endif // SETTINGSTABGENERAL_H
