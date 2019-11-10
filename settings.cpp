#include "settings.h"

#include <QDialogButtonBox>
#include <QListWidget>
#include <QVBoxLayout>
#include <settingstabgeneral.h>

static string settingsConfig;
static json j;

SettingsDialog::SettingsDialog(QWidget *parent, string configFolder, SoundPlayback *soundPlayback) :
    QDialog(parent)
{
    settingsConfig = configFolder + "/settings.json";
    setWindowTitle(tr("Settings"));

    ifstream fileIn(settingsConfig);
    if (fileIn.is_open())
    {
        string content((istreambuf_iterator<char>(fileIn)), istreambuf_iterator<char>());
        j = json::parse(content);
    }

    QVBoxLayout* layout = new QVBoxLayout(this);

    tab_widget_ = new QTabWidget();
    layout->addWidget(tab_widget_);

    AddTab(new SettingsTabGeneral(j, soundPlayback), tr("General"));

    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    layout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void SettingsDialog::reject()
{
    foreach (SettingsTab* tab, tabs_) {
        tab->reset();
    }

    QDialog::reject();
}

void SettingsDialog::accept()
{
    json jsonTabs;

    foreach (SettingsTab* tab, tabs_) {
        jsonTabs[tab->name] = tab->tabSettings();
    }

    ofstream myfile;
    myfile.open(settingsConfig);
    myfile << jsonTabs.dump();
    myfile.close();

    QDialog::accept();
}

void SettingsDialog::AddTab(SettingsTab *tab, const QString &title)
{
  tab_widget_->addTab(tab, title);
  tabs_.append(tab);
}
