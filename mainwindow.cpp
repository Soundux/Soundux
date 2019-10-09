
#include "mainwindow.h"
#include "./ui_mainwindow.h"

/*
 *
 * TODO: Find another way how to play it for myself and others (maybe just loopback the default output to the sink monitor)
 *
*/

static vector<PulseAudioRecordingStream *> streams;

static string configFolder;
static string soundFilesConfig;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
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

    // Tell me if there is a better way to parse the pulseaudio source outputs
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

        // Set volume for game sink monitor from slider
        int value = ui->volumeSlider->value();
        system(("pacmd set-source-volume soundboard_sink.monitor " + to_string(value)).c_str());

        try
        {
            forMe.join();
            forOthers.join();
        }
        catch (...)
        {
        }

        forMe = std::thread([=]() {
            auto cmdForMe = "mpg123 -o pulse \"" + path + "\"";
            system(cmdForMe.c_str());
        });

        forOthers = std::thread([=]() {
            ui->stopButton->setDisabled(false);
            auto cmdForOthers = "mpg123 -o pulse -a soundboard_sink \"" + path + "\"";
            system(cmdForOthers.c_str());
            // Switch recording stream device back
            system(moveBack.c_str());
            ui->stopButton->setDisabled(true);
        });
    }
}

void MainWindow::on_refreshAppsButton_clicked()
{
    loadSources();
}

void MainWindow::on_stopButton_clicked()
{
    //TODO: Only kill mpg123 started from Soundboard
    system("killall mpg123");
    //pthread_kill(forMe.native_handle(), 9);
    //pthread_kill(forOthers.native_handle(), 9);
    ui->stopButton->setDisabled(true);
}

void MainWindow::on_addFolderButton_clicked()
{
    auto selectedFolder = QFileDialog::getExistingDirectory(this, ("Select folder"), QDir::homePath());

    if (selectedFolder != "")
    {
        QDir directory(selectedFolder);
        QFileInfo fileInfo(selectedFolder);
        auto created = createTab(fileInfo.fileName());

        QStringList files = directory.entryList(QStringList() << "*.mp3", QDir::Files);
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
            QMessageBox::warning(this, "", tr("This sound is already in the list"), QMessageBox::Ok);
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
    QStringList selectedFiles = QFileDialog::getOpenFileNames(this, tr("Select file"), QDir::homePath(), tr("MP3 (*.mp3)"));
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

void MainWindow::on_playSoundButton_clicked()
{
    if (getActiveView())
    {
        QListWidgetItem *it = getActiveView()->item(getActiveView()->currentRow());
        if (it)
        {
            playSound(it->toolTip().toStdString());
        }
    }
}

void MainWindow::on_addTabButton_clicked()
{
    bool ok;
    QString text = QInputDialog::getText(0, "Add a tab", "Tab Text:", QLineEdit::Normal, "", &ok);
    if (ok && !text.isEmpty())
    {
        createTab(text);
    }
}

void MainWindow::on_tabWidget_tabBarDoubleClicked(int index)
{
    bool ok;
    QString text = QInputDialog::getText(0, "Rename tab", "Tab Text:", QLineEdit::Normal, ui->tabWidget->tabText(index), &ok);
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
    saveSoundFiles();
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
            j[item->text().toStdString()] = item->toolTip().toStdString();
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
                    for (auto child : _child.items())
                    {
                        auto soundName = child.key();
                        auto soundPath = child.value();
                        remove(soundPath.begin(), soundPath.end(), '"');

                        auto item = new QListWidgetItem();
                        item->setText(QString::fromStdString(soundName));
                        item->setToolTip(QString::fromStdString(soundPath));
                        soundsListWidget->addItem(item);
                    }
                }
            }
        }

        fileIn.close();
    }
}