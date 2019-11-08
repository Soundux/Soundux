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
#include <QDialogButtonBox>
#include <QLabel>

#include <customkeysequenceedit.h>

using namespace std;

class SetHotkeyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SetHotkeyDialog(QWidget *parent = nullptr, QListWidgetItem* item = nullptr);

private slots:
    virtual void accept() override;
    virtual void truncateShortcut();

private:
    QListWidgetItem* item;
    CustomKeySequenceEdit* edit;
};

#endif // SETHOTKEYDIALOG_H
