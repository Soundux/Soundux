#include "mainwindow.h"
#include "ui_mainwindow.h"

/*
 *
 * TODO: Find another way how to play it for myself and others (maybe just loopback the default output to the sink monitor)
 *
*/

static vector<QHotkey *> hotkeys;

static string configFolder;
static string soundFilesConfig;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->tabWidget->setTabsClosable(true);
    ui->tabWidget->setMovable(true);
    ui->stopButton->setDisabled(true);

    ui->remoteVolumeSlider->setStyle(new ClickableSliderStyle(ui->remoteVolumeSlider->style()));
    ui->localVolumeSlider->setStyle(new ClickableSliderStyle(ui->localVolumeSlider->style()));

    // Set the config variables
    configFolder = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)[0].toStdString() + "/" + windowTitle().toStdString();
    if (!filesystem::exists(configFolder))
    {
        filesystem::create_directory(configFolder);
    }
    soundFilesConfig = configFolder + "/sounds.json";

    soundPlayback = new SoundPlayback(this, ui);

    settingsDialog = new SettingsDialog(this, configFolder, soundPlayback);

    // Disable resizing
    this->setFixedSize(this->width(), this->height());

    //TODO: Only remove modules created by Soundboard
    system("pacmd unload-module module-null-sink");
    system("pacmd unload-module module-loopback");

    // Create null sink
    system("pacmd load-module module-null-sink sink_name=soundboard_sink sink_properties=device.description=Soundboard-Sink");

    // Create loopback for output devices (so that you can hear it)
    //system("pacmd load-module module-loopback source=\"soundboard_sink.monitor\"");

    // get default input device
    string defaultInput = "";
    char cmd[] = "pacmd dump";
    string result = soundPlayback->getCommandOutput(cmd);
    regex reg(R"rgx(set-default-source (.+))rgx");
    smatch sm;
    regex_search(result, sm, reg);
    defaultInput = sm[1].str();

    // Create loopback for input
    if (defaultInput != "")
    {
        cout << "Found default input device " << defaultInput << endl;
        auto createLoopBack = "pacmd load-module module-loopback source=\"" + defaultInput + "\" sink=\"soundboard_sink\"";
        system(createLoopBack.c_str());
    }

    loadSoundFiles();
    soundPlayback->loadSources();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    //TODO: Only remove modules created by Soundboard
    system("pacmd unload-module module-null-sink");
    system("pacmd unload-module module-loopback");
    //TODO: Switch all recording streams back to default device
    event->accept();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::syncVolume(bool remote)
{
    // Get volume from slider
    int localValue = ui->localVolumeSlider->value();
    int remoteValue = ui->remoteVolumeSlider->value();

    if (ui->syncCheckBox->isChecked()) {
        if (remote) {
            ui->localVolumeSlider->setValue(remoteValue);
        } else {
            ui->remoteVolumeSlider->setValue(localValue);
        }
    }

    // TODO: this is disabled until I find a good solution
    /*

    char cmd[] = "pacmd list-sink-inputs";
    string result = getCommandOutput(cmd);
    string delimiter = "\n";
    size_t pos = 0;
    string currentLine;

    // Tell me if there is a better way to parse the pulseaudio output
    regex reg(R"rgx(((index: (\d+)))|(driver: )(.*)|(state: )(.*)|(flags: )(.*)|(source: .*)(<(.*)>)|(muted: )(.{0,3})|([a-zA-Z-.0-9_]*)\ =\ (\"(.*)\"))rgx");
    smatch sm;

    PulseAudioPlaybackStream *current = nullptr;

    while ((pos = result.find(delimiter)) != string::npos)
    {
        currentLine = result.substr(0, pos);
        if (regex_search(currentLine, sm, reg))
        {
            auto index = sm[3];
            if (index.length() > 0)
            {
                if (current)
                {
                    soundPlayback->checkAndChangeVolume(current, localValue);
                }

                current = new PulseAudioPlaybackStream();
                current->index = stoi(index);
            }
            else
            {
                auto propertyName = sm[15];
                auto propertyValue = sm[17];
                if (propertyName.length() > 0)
                {
                    if (propertyName == "application.name")
                    {
                        current->applicationName = propertyValue.str();
                    }
                }
            }
        }

        result.erase(0, pos + delimiter.length());
    }
    soundPlayback->checkAndChangeVolume(current, localValue);
    */
}

// Sync volume when the slider value has changed
void MainWindow::on_localVolumeSlider_valueChanged(int value)
{
    syncVolume(false);
}
void MainWindow::on_remoteVolumeSlider_valueChanged(int value)
{
    syncVolume(true);
}


void MainWindow::on_refreshAppsButton_clicked()
{
    soundPlayback->loadSources();
}

void MainWindow::on_stopButton_clicked()
{
    soundPlayback->stopSound();
}

void MainWindow::on_addFolderButton_clicked()
{
    auto selectedFolder = QFileDialog::getExistingDirectory(this, ("Select folder"), QDir::homePath());

    if (selectedFolder != "")
    {
        QDir directory(selectedFolder);
        QFileInfo fileInfo(selectedFolder);
        auto created = createTab(fileInfo.fileName());

        QStringList files = directory.entryList({"*.mp3", "*.wav", "*.ogg"}, QDir::Files);
        for (auto fileName : files)
        {
            QFile file(directory.absoluteFilePath(fileName));
            addSoundToView(file, created);
        }
        saveSoundFiles();
    }
}

void MainWindow::addSoundToView(QFile &file, QListWidget *widget)
{
    QFileInfo fileInfo(file);

    auto path = fileInfo.absoluteFilePath().toStdString();

    for (QListWidgetItem *item : widget->findItems("*", Qt::MatchWildcard))
    {
        // Check if Sound is already added
        if (path == item->toolTip().toStdString())
        {
            auto already = "The sound " + item->text().toStdString() + " is already in the list";
            QMessageBox::warning(this, "", tr(already.c_str()), QMessageBox::Ok);
            return;
        }
    }

    auto item = new SoundListWidgetItem();
    item->setText(fileInfo.completeBaseName());
    item->setToolTip(fileInfo.absoluteFilePath());
    widget->addItem(item);
}

void MainWindow::on_addSoundButton_clicked()
{
    if (!getActiveView())
    {
        createTab("Main");
    }
    QStringList selectedFiles = QFileDialog::getOpenFileNames(this, tr("Select file"), QDir::homePath(), tr("Sound files (*.mp3 *.wav *.ogg)"));
    for (auto selectedFile : selectedFiles)
    {
        if (selectedFile != "")
        {
            QFile file(selectedFile);
            addSoundToView(file, getActiveView());
        }
    }
    saveSoundFiles();
}

void MainWindow::on_settingsButton_clicked()
{
    settingsDialog->exec();
}

void MainWindow::on_soundsListWidget_itemDoubleClicked(QListWidgetItem *listWidgetItem)
{
    if (listWidgetItem)
    {
        soundPlayback->playSound(listWidgetItem->toolTip().toStdString());
    }
}

void MainWindow::on_removeSoundButton_clicked()
{
    if (getActiveView())
    {
        SoundListWidgetItem *it = getSelectedItem();
        if (it)
        {
            unregisterHotkey(it);
            delete it;
            saveSoundFiles();
        }
    }
}

void MainWindow::on_clearSoundsButton_clicked()
{
    if (getActiveView())
    {
        QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Clear sounds", tr("Are you sure?\n"), QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
        if (resBtn == QMessageBox::Yes)
        {
            clearSoundFiles();
            saveSoundFiles();
        }
    }
}

SoundListWidgetItem *MainWindow::getSelectedItem()
{
    if (getActiveView())
    {
        return (SoundListWidgetItem*) getActiveView()->item(getActiveView()->currentRow());
    }
    return nullptr;
}

void MainWindow::on_playSoundButton_clicked()
{
    SoundListWidgetItem *it = getSelectedItem();
    if (it)
    {
        soundPlayback->playSound(it->toolTip().toStdString());
    }
}

void MainWindow::on_addTabButton_clicked()
{
    bool ok;
    QString text = QInputDialog::getText(this, "Add a tab", "Tab Text:", QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty())
    {
        createTab(text);
        saveSoundFiles();
    }
}

void MainWindow::on_setHotkeyButton_clicked()
{
    SoundListWidgetItem *it = getSelectedItem();
    if (it)
    {
        SetHotkeyDialog shd(this, it);
        auto clicked = shd.exec();
        if (clicked == 1)
        {
            auto given = shd.getSequence();
            if (!given.isNull()) {
                registerHotkey(it, given.toString());
            } else {
                unregisterHotkey(it);
            }

            saveSoundFiles();
        }
    }
}

void MainWindow::registerHotkey(SoundListWidgetItem* it, QString keys)
{
    // Unregister previous hotkey
    unregisterHotkey(it);

    it->setHotkey(keys);
    cout << "register " << keys.toStdString() << endl;
    auto neger = QKeySequence(keys);

    auto hotkey = new QHotkey(QKeySequence(keys), true, this);

    if (hotkey->isRegistered())
    {
        hotkeys.push_back(hotkey);
        auto toPlay = it->toolTip().toStdString();
        connect(hotkey, &QHotkey::activated, this, [=]() {
            soundPlayback->playSound(toPlay);
        });
    }
    else
    {
        unregisterHotkey(it);
        QMessageBox::warning(this, "Could not register " + keys, "Either the key combination is not valid or it's not possible to use this combination (Maybe another program is using it)", QMessageBox::Ok);
    }
}

bool compareChar(char &c1, char &c2)
{
    if (c1 == c2)
        return true;
    else if (toupper(c1) == toupper(c2))
        return true;
    return false;
}

bool caseInSensStringCompare(string &str1, string &str2)
{
    return ((str1.size() == str2.size()) &&
            equal(str1.begin(), str1.end(), str2.begin(), &compareChar));
}

void MainWindow::unregisterHotkey(SoundListWidgetItem *it)
{
    auto previousHotkey = it->hotkey;
    if (!previousHotkey.isNull())
    {
        auto previousHotkeyStr = previousHotkey.toString().toStdString();
        cout << "unregister " << previousHotkeyStr << endl;

        for (QHotkey *hotkey : hotkeys)
        {
            auto hotkeyStr = hotkey->shortcut().toString().toStdString();
            if (caseInSensStringCompare(hotkeyStr, previousHotkeyStr))
            {
                delete hotkey;
            }
        }

        // Reset Data
        it->setHotkey(QVariant());
    }
}

void MainWindow::on_tabWidget_tabBarDoubleClicked(int index)
{
    bool ok;
    QString text = QInputDialog::getText(this, "Rename tab", "Tab Text:", QLineEdit::Normal, ui->tabWidget->tabText(index), &ok);
    if (ok && !text.isEmpty())
    {
        ui->tabWidget->setTabText(index, text);
        saveSoundFiles();
    }
}

void MainWindow::on_tabWidget_tabCloseRequested(int index)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Delete tab", tr("Are you sure?\n"), QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
    if (resBtn == QMessageBox::Yes)
    {
        ui->tabWidget->removeTab(index);
        saveSoundFiles();
    }
}

QListWidget *MainWindow::createTab(QString title)
{
    auto soundsListWidget = new QListWidget();
    soundsListWidget->setObjectName(title);
    connect(soundsListWidget, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(on_soundsListWidget_itemDoubleClicked(QListWidgetItem *)));
    ui->tabWidget->addTab(soundsListWidget, title);
    return soundsListWidget;
}

void MainWindow::clearSoundFiles()
{
    if (getActiveView())
    {
        while (getActiveView()->count() > 0)
        {
            getActiveView()->takeItem(0);
        }
    }
}

QListWidget *MainWindow::getActiveView()
{
    return (QListWidget *)ui->tabWidget->widget(ui->tabWidget->currentIndex());
}

void MainWindow::saveSoundFiles()
{
    json jsonTabs = json::array();

    for (auto i = 0; i < ui->tabWidget->count(); i++)
    {
        auto title = ui->tabWidget->tabText(i).toStdString();
        QListWidget *listWidget = (QListWidget *)ui->tabWidget->widget(i);

        json tabJson;
        json tabJsonSounds = json::array();

        for (auto *_item : listWidget->findItems("*", Qt::MatchWildcard))
        {
            auto item = (SoundListWidgetItem*) _item;
            json j;
            j["name"] = item->text().toStdString();
            j["path"] = item->toolTip().toStdString();

            auto hotkey = item->hotkey;
            if (!hotkey.isNull())
            {
                auto hotkeyStr = hotkey.toString().toStdString();
                j["hotkey"] = hotkeyStr;
            }

            tabJsonSounds.push_back(j);
        }

        tabJson[title] = tabJsonSounds;
        jsonTabs.push_back(tabJson);
    }

    ofstream myfile;
    myfile.open(soundFilesConfig);
    myfile << jsonTabs.dump();
    myfile.close();
}

void MainWindow::loadSoundFiles()
{
    ifstream fileIn(soundFilesConfig);
    if (fileIn.is_open())
    {
        clearSoundFiles();

        string content((istreambuf_iterator<char>(fileIn)), istreambuf_iterator<char>());
        json j = json::parse(content);

        for (auto item : j.get<vector<json>>())
        {
            for (auto object : item.items())
            {
                auto tabName = object.key().c_str();

                auto soundsListWidget = createTab(tabName);

                auto childItems = object.value().get<vector<json>>();
                for (auto _child : childItems)
                {
                    auto soundName = _child["name"];
                    auto soundPath = _child["path"];
                    remove(soundPath.begin(), soundPath.end(), '"');

                    auto item = new SoundListWidgetItem();
                    item->setText(QString::fromStdString(soundName));
                    item->setToolTip(QString::fromStdString(soundPath));

                    auto soundHotkey = _child["hotkey"];
                    if (!soundHotkey.is_null())
                    {
                        // Set hotkey back
                        registerHotkey(item, QString::fromStdString(soundHotkey));
                    }
                    soundsListWidget->addItem(item);
                }
            }
        }

        fileIn.close();
    }
}
