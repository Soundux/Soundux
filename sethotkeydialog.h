#ifndef SETHOTKEYDIALOG_H
#define SETHOTKEYDIALOG_H

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <array>
#include <stdlib.h>

#include <QDialog>
#include <QWidget>
#include <QListWidgetItem>
#include <QKeySequenceEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLabel>

#include <customkeysequenceedit.h>
#include <soundlistwidgetitem.h>

using namespace std;

class SetHotkeyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SetHotkeyDialog(QWidget *parent = nullptr, SoundListWidgetItem* item = nullptr);

private slots:
    virtual void accept() override;
    virtual void truncateShortcut();
    void on_clearButton_pressed();

private:
    SoundListWidgetItem* item;
    CustomKeySequenceEdit* edit;
};

#endif // SETHOTKEYDIALOG_H
