#include "mainwindow.h"
#include "./ui_mainwindow.h"

/*
 *
 * TODO: Find another way how to play it for myself and others (maybe just loopback the default output to the sink monitor)
 *
*/

static vector<PulseAudioRecordingStream *> streams;

static vector<QHotkey *> hotkeys;

static string configFolder;
static string soundFilesConfig;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tabWidget->setTabsClosable(true);
    ui->tabWidget->setMovable(true);
    ui->stopButton->setDisabled(true);

    // Set the config variables
    configFolder = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)[0].toStdString() + "/" + windowTitle().toStdString();
    if (!filesystem::exists(configFolder))
    {
        filesystem::create_directory(configFolder);
    }
    soundFilesConfig = configFolder + "/sounds.json";

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
    string result = getCommandOutput(cmd);
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
    loadSources();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    //TODO: Only remove modules created by Soundboard
    system("pacmd unload-module module-null-sink");
    system("pacmd unload-module module-loopback");
    //TODO: Switch all recording streams back to default device
    event->accept();
}

string MainWindow::getCommandOutput(char cmd[])
{
    array<char, 1028> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe)
    {
        throw runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    return result;
}

bool MainWindow::isValidDevice(PulseAudioRecordingStream *stream)
{
    return !strstr(stream->source.c_str(), ".monitor") && !strstr(stream->flags.c_str(), "DONT_MOVE");
}

bool MainWindow::loadSources()
{
    // Save previously selected applicaton
    auto previouslySelected = ui->outputApplication->currentText();

    streams.clear();
    ui->outputApplication->clear();

    char cmd[] = "pacmd list-source-outputs";
    string result = getCommandOutput(cmd);
    string delimiter = "\n";
    size_t pos = 0;
    string currentLine;

    // Tell me if there is a better way to parse the pulseaudio output
    regex reg(R"rgx(((index: (\d+)))|(driver: )(.*)|(state: )(.*)|(flags: )(.*)|(source: .*)(<(.*)>)|(muted: )(.{0,3})|([a-zA-Z-.0-9_]*)\ =\ (\"(.*)\"))rgx");
    smatch sm;

    PulseAudioRecordingStream *current = nullptr;

    while ((pos = result.find(delimiter)) != string::npos)
    {
        currentLine = result.substr(0, pos);
        if (regex_search(currentLine, sm, reg))
        {
            auto index = sm[3];
            if (index.length() > 0)
            {

                if (current && isValidDevice(current))
                {
                    streams.push_back(current);
                }

                current = new PulseAudioRecordingStream();
                current->index = stoi(index);
            }
            else if (current)
            {

                auto driver = sm[5];
                auto state = sm[7];
                auto flags = sm[9];
                auto source = sm[12];
                auto muted = sm[14];
                auto propertyName = sm[15];
                auto propertyValue = sm[17];

                if (driver.length() > 0)
                {
                    current->driver = driver.str();
                }
                if (state.length() > 0)
                {
                    current->state = state.str();
                }
                if (flags.length() > 0)
                {
                    current->flags = flags.str();
                }
                if (source.length() > 0)
                {
                    current->source = source.str();
                }
                if (muted.length() > 0)
                {
                    current->muted = muted == "yes" ? true : false;
                }
                if (propertyName.length() > 0)
                {
                    if (propertyName == "application.name")
                    {
                        current->applicationName = propertyValue.str();
                    }
                    if (propertyName == "application.process.id")
                    {
                        current->processId = stoi(propertyValue);
                    }
                    if (propertyName == "application.process.binary")
                    {
                        current->processBinary = propertyValue.str();
                    }
                }
            }
        }

        result.erase(0, pos + delimiter.length());
    }
    if (isValidDevice(current))
    {
        streams.push_back(current);
    }

    for (auto stream : streams)
    {
        if (stream->driver == "<protocol-native.c>")
        {
            ui->outputApplication->addItem(QString(stream->processBinary.c_str()));
        }
    }

    // This automatically sets the selected item to the previous one. if it does not exists it does nothing
    ui->outputApplication->setCurrentText(previouslySelected);

    // Return if the output was not changed
    return ui->outputApplication->currentText() == previouslySelected;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::playSound(string path)
{
    //TODO: Remove this and stop old playback or enable multiple sounds at once (maybe create a setting for it)
    if (ui->stopButton->isEnabled())
    {
        return;
    }

    // Don't play the sound if the app changed (previous one no longer available)
    if (!loadSources())
    {
        QMessageBox::warning(this, "", tr("Output stream no longer available...\nAborting\n"), QMessageBox::Ok);
        return;
    }

    bool isMP3 = strstr(path.c_str(), ".mp3");

    if (isMP3)
    {
        ostringstream mpg123Check;
        mpg123Check << "which mpg123 >/dev/null 2>&1";
        bool canPlayMP3 = (system(mpg123Check.str().c_str()) == 0);
        if (!canPlayMP3)
        {
            QMessageBox::critical(this, "", tr("Can't play mp3 file!\nmpg123 is not installed or not in the $PATH\nPlease install it and restart the program\n"), QMessageBox::Ok);
            return;
        }
    }

    // Get selected application
    string selectedApp = ui->outputApplication->currentText().toStdString();
    PulseAudioRecordingStream *selected = nullptr;
    for (auto stream : streams)
    {
        if (stream->processBinary == selectedApp)
        {
            selected = stream;
        }
    }

    if (selected)
    {
        int index = selected->index;
        string source = selected->source;

        cout << "Source before was " << source << endl;

        auto moveToSink = "pacmd move-source-output " + to_string(index) + " soundboard_sink.monitor";
        auto moveBack = "pacmd move-source-output " + to_string(index) + " " + source;

        // Switch recording stream device to game sink
        system(moveToSink.c_str());

        auto forMe = std::thread([=]() {
            ui->localVolumeSlider->setDisabled(true);
            auto cmdForMe = "paplay --volume=" + to_string(ui->localVolumeSlider->value()) + " \"" + path + "\"";
            if (isMP3)
            {
                cmdForMe = "mpg123 -o pulse -f " + to_string(ui->localVolumeSlider->value() / 2) + " \"" + path + "\"";
            }
            system(cmdForMe.c_str());
            ui->localVolumeSlider->setDisabled(false);
        });
        forMe.detach();

        auto forOthers = std::thread([=]() {
            ui->stopButton->setDisabled(false);
            ui->remoteVolumeSlider->setDisabled(true);
            auto cmdForOthers = "paplay -d soundboard_sink --volume=" + to_string(ui->remoteVolumeSlider->value()) + " \"" + path + "\"";
            if (isMP3)
            {
                cmdForOthers = "mpg123 -o pulse -a soundboard_sink -f " + to_string(ui->remoteVolumeSlider->value() / 2) + " \"" + path + "\"";
            }
            system(cmdForOthers.c_str());
            // Switch recording stream device back
            system(moveBack.c_str());
            ui->stopButton->setDisabled(true);
            ui->remoteVolumeSlider->setDisabled(false);
            // Repeat when the check box is checked
            if (ui->repeatCheckBox->isChecked())
            {
                playSound(path);
            }
        });
        forOthers.detach();
    }
}

void MainWindow::checkAndChangeVolume(PulseAudioPlaybackStream *stream, int value)
{
    // TODO: Only set it when this was created by Soundboard
    // Set the volume if the application is paplay or mpg123
    if (stream->applicationName == "paplay" || stream->applicationName == "mpg123")
    {
        system(("pacmd set-sink-input-volume " + to_string(stream->index) + " " + to_string(value)).c_str());
    }
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
                    checkAndChangeVolume(current, localValue);
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
    checkAndChangeVolume(current, localValue);
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
    loadSources();
}

void MainWindow::on_stopButton_clicked()
{
    // Fix continuous playback
    if (ui->repeatCheckBox->isChecked())
    {
        ui->repeatCheckBox->setChecked(false);
    }
    //TODO: Only kill players started from Soundboard
    system("killall mpg123");
    system("killall paplay");
    ui->stopButton->setDisabled(true);
    ui->localVolumeSlider->setDisabled(false);
    ui->remoteVolumeSlider->setDisabled(false);
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

    auto item = new QListWidgetItem();
    item->setText(fileInfo.baseName());
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

void MainWindow::on_soundsListWidget_itemDoubleClicked(QListWidgetItem *listWidgetItem)
{
    if (listWidgetItem)
    {
        playSound(listWidgetItem->toolTip().toStdString());
    }
}

void MainWindow::on_removeSoundButton_clicked()
{
    if (getActiveView())
    {
        QListWidgetItem *it = getActiveView()->takeItem(getActiveView()->currentRow());
        if (it)
        {
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

QListWidgetItem *MainWindow::getSelectedItem()
{
    if (getActiveView())
    {
        return getActiveView()->item(getActiveView()->currentRow());
    }
    return nullptr;
}

void MainWindow::on_playSoundButton_clicked()
{
    QListWidgetItem *it = getActiveView()->item(getActiveView()->currentRow());
    if (it)
    {
        playSound(it->toolTip().toStdString());
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
    QListWidgetItem *it = getActiveView()->item(getActiveView()->currentRow());
    if (it)
    {
        SetHotkeyDialog shd(this, it);
        shd.exec();

        if (!it->data(1).isNull()) {
            registerHotkey(it, it->data(1).toString());
        } else {
            unregisterHotkey(it);
        }

        saveSoundFiles();
    }
}

void MainWindow::registerHotkey(QListWidgetItem* it, QString keys)
{
    // Unregister previous hotkey
    unregisterHotkey(it);

    it->setData(1, keys);
    auto neger = QKeySequence(keys);

    auto hotkey = new QHotkey(QKeySequence(keys), true, this);

    if (hotkey->isRegistered())
    {
        hotkeys.push_back(hotkey);
        auto toPlay = it->toolTip().toStdString();
        connect(hotkey, &QHotkey::activated, this, [=]() {
            playSound(toPlay);
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

void MainWindow::unregisterHotkey(QListWidgetItem *it)
{
    auto previousHotkey = it->data(1);
    if (!previousHotkey.isNull())
    {
        auto previousHotkeyStr = previousHotkey.toString().toStdString();

        for (QHotkey *hotkey : hotkeys)
        {
            auto hotkeyStr = hotkey->shortcut().toString().toStdString();
            if (caseInSensStringCompare(hotkeyStr, previousHotkeyStr))
            {
                delete hotkey;
            }
        }

        // Reset Data
        it->setData(1, QVariant());
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

        for (QListWidgetItem *item : listWidget->findItems("*", Qt::MatchWildcard))
        {
            json j;
            j["name"] = item->text().toStdString();
            j["path"] = item->toolTip().toStdString();

            auto hotkey = item->data(1);
            if (!hotkey.isNull())
            {
                auto hotkeyStr = item->data(1).toString().toStdString();
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

                    auto item = new QListWidgetItem();
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

void MainWindow::on_settingsButton_clicked()
{
    SettingsDialog sd(this);
    sd.exec();
}
