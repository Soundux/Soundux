
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <stdlib.h>
#include <regex>
#include <thread>
#include <fstream>
#include <QCloseEvent>
#include <QMessageBox>

using json = nlohmann::json;

/*
 * Dependencies: pulseaudio, mpg123
 *
 * TODO: Find another way how to play it for myself and others (maybe just loopback the default output to the sink monitor)
 * TODO: Save configuration in .config folder
 *
*/

static vector<PulseAudioRecordingStream *> streams;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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
    if(defaultInput != "") {
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
    event->accept();
}


string MainWindow::getCommandOutput(char cmd[]) {
    array<char, 1028> buffer;
    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

bool MainWindow::isValidDevice(PulseAudioRecordingStream* stream) {
    return !strstr(stream->source.c_str(), ".monitor") && !strstr(stream->flags.c_str(), "DONT_MOVE");
}

bool MainWindow::loadSources() {
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

    PulseAudioRecordingStream* current = nullptr;

    while ((pos = result.find(delimiter)) != string::npos) {
        currentLine = result.substr(0, pos);
        if(regex_search(currentLine, sm, reg)) {
            auto index = sm[3];
            if(index.length() > 0) {

                if(current && isValidDevice(current)) {
                    streams.push_back(current);
                }

                current = new PulseAudioRecordingStream();
                current->index = stoi(index);
            } else if(current) {

                auto driver = sm[5];
                auto state = sm[7];
                auto flags = sm[9];
                auto source = sm[12];
                auto muted = sm[14];
                auto propertyName = sm[15];
                auto propertyValue = sm[17];

                if(driver.length() > 0) {
                    current->driver = driver.str();
                }
                if(state.length() > 0) {
                    current->state = state.str();
                }
                if(flags.length() > 0) {
                    current->flags = flags.str();
                }
                if(source.length() > 0) {
                    current->source = source.str();
                }
                if(muted.length() > 0) {
                    current->muted = muted == "yes" ? true : false;
                }
                if(propertyName.length() > 0) {
                    if(propertyName == "application.name") {
                        current->applicationName = propertyValue.str();
                    }
                    if(propertyName == "application.process.id") {
                        current->processId = stoi(propertyValue);
                    }
                    if(propertyName == "application.process.binary") {
                        current->processBinary = propertyValue.str();
                    }
                }

            }


        }

        result.erase(0, pos + delimiter.length());
    }
    if(isValidDevice(current)) {
        streams.push_back(current);
    }


    for(auto stream : streams) {
        if(stream->driver == "<protocol-native.c>") {
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

void MainWindow::playSound(string path) {
    //TODO: Stop old playback

    // Don't play the sound if the app changed (previous one no longer available)
    if(!loadSources()) {
        QMessageBox::warning(this, "", tr("Output stream no longer available...\nAborting\n"), QMessageBox::Ok);
        return;
    }

    // Get selected application
    string selectedApp = ui->outputApplication->currentText().toStdString();
    PulseAudioRecordingStream* selected = nullptr;
    for(auto stream : streams) {
        if(stream->processBinary == selectedApp) {
            selected = stream;
        }
    }

    if(selected) {
        int index = selected->index;
        string source = selected->source;

        auto moveToSink = "pacmd move-source-output " + to_string(index) + " soundboard_sink.monitor";
        auto moveBack = "pacmd move-source-output " + to_string(index) + " " + source;

         // Switch recording stream device to game sink
        system(moveToSink.c_str());

        // Set volume for game sink monitor from slider
        int value = ui->volumeSlider->value();
        system(("pacmd set-source-volume soundboard_sink.monitor " + to_string(value)).c_str());

        cout << "Start" << endl;

        std::thread forMe([&]
        {
            system(("mpg123 -o pulse \"" + path + "\"").c_str());
        });
        forMe.detach();

        auto cmdForOthers = "mpg123 -o pulse -a soundboard_sink \"" + path + "\"";
        system(cmdForOthers.c_str());

        // Switch recording stream device back
        system(moveBack.c_str());
        cout << "End" << endl;

    }
}


void MainWindow::on_refreshAppsButton_clicked()
{
    loadSources();
}

void MainWindow::on_playCustomButton_clicked()
{
    playSound(ui->customAudioText->toPlainText().toStdString());
}

void MainWindow::on_customFileChoose_clicked()
{
    QString selectedFile = QFileDialog::getOpenFileName(this, tr("Select file"), QDir::homePath(), tr("MP3 (*.mp3)"));
    QFile file(selectedFile);
    QFileInfo fileInfo((QFileInfo(file)));
    ui->customAudioText->setText(fileInfo.absoluteFilePath());
}

void MainWindow::on_stopButton_clicked()
{
    //TODO: stop
    cout << "Stop" << endl;
}

void MainWindow::on_addSoundButton_clicked()
{
    QString selectedFile = QFileDialog::getOpenFileName(this, tr("Select file"), QDir::homePath(), tr("MP3 (*.mp3)"));
    if (selectedFile != "") {
        QFile file(selectedFile);
        QFileInfo fileInfo((QFileInfo(file)));

        auto path = fileInfo.absoluteFilePath().toStdString();

        for (QListWidgetItem* item : ui->soundsListWidget->findItems("*", Qt::MatchWildcard)) {
            if (path == item->toolTip().toStdString()) {
                return;
            }
        }

        auto item = new QListWidgetItem();
        item->setText(fileInfo.baseName());
        item->setToolTip(fileInfo.absoluteFilePath());
        ui->soundsListWidget->addItem(item);

        saveSoundFiles();
    }
}

void MainWindow::on_removeSoundButton_clicked()
{
    QListWidgetItem *it = ui->soundsListWidget->takeItem(ui->soundsListWidget->currentRow());
    if (it) {
        delete it;
    }

    saveSoundFiles();
}

void MainWindow::on_clearSoundsButton_clicked()
{
    QMessageBox::StandardButton resBtn = QMessageBox::question(this, "", tr("Are you sure?\n"), QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
    if (resBtn == QMessageBox::Yes) {
        clearSoundFiles();
        saveSoundFiles();
    }
}

void MainWindow::on_playSoundButton_clicked()
{
    QListWidgetItem *it = ui->soundsListWidget->item(ui->soundsListWidget->currentRow());
    if (it) {
        playSound(it->toolTip().toStdString());
    }
}

void MainWindow::clearSoundFiles() {
    while(ui->soundsListWidget->count()>0)
    {
      ui->soundsListWidget->takeItem(0);
    }
}

void MainWindow::saveSoundFiles() {
    json jsonArray = json::array();
    for (QListWidgetItem* item : ui->soundsListWidget->findItems("*", Qt::MatchWildcard)) {
        json j;
        j[item->text().toStdString()] = item->toolTip().toStdString();
        jsonArray.push_back(j);
    }

    ofstream myfile;
    myfile.open("soundFiles.json");
    myfile << jsonArray.dump();
    myfile.close();
}

void MainWindow::loadSoundFiles() {
    ifstream fileIn("soundFiles.json");
    if(fileIn.is_open()) {
        string content((istreambuf_iterator<char>(fileIn)), istreambuf_iterator<char>());

        json j = json::parse(content);

        clearSoundFiles();

        for(auto json : j) {
            for (json::iterator it = json.begin(); it != json.end(); ++it) {
               auto name = it.key();
               auto path = it.value();
               remove(path.begin(), path.end(), '"');

               auto item = new QListWidgetItem();
               item->setText(QString::fromStdString(name));
               item->setToolTip(QString::fromStdString(path));
               ui->soundsListWidget->addItem(item);
            }
        }

        fileIn.close();
    }
}
