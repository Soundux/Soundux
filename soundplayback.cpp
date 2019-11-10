#include "soundplayback.h"

static vector<PulseAudioRecordingStream *> streams;

SoundPlayback::SoundPlayback(QWidget *parent, Ui::MainWindow* mainWindow) : QObject(parent)
{
    this->parent = parent;
    this->ui = mainWindow;
}

SoundPlayback::~SoundPlayback()
{

}

bool SoundPlayback::isValidDevice(PulseAudioRecordingStream *stream)
{
    return !strstr(stream->source.c_str(), ".monitor") && !strstr(stream->flags.c_str(), "DONT_MOVE");
}

string SoundPlayback::getCommandOutput(char cmd[])
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

void SoundPlayback::checkAndChangeVolume(PulseAudioPlaybackStream *stream, int value)
{
    // TODO: Only set it when this was created by Soundboard
    // Set the volume if the application is paplay or mpg123
    if (stream->applicationName == "paplay" || stream->applicationName == "mpg123")
    {
        system(("pacmd set-sink-input-volume " + to_string(stream->index) + " " + to_string(value)).c_str());
    }
}

void SoundPlayback::stopSound()
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


bool SoundPlayback::loadSources()
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

void SoundPlayback::playSound(string path)
{
    //TODO: Remove this and stop old playback or enable multiple sounds at once (maybe create a setting for it)
    if (ui->stopButton->isEnabled())
    {
        return;
    }

    // Don't play the sound if the app changed (previous one no longer available)
    if (!loadSources())
    {
        QMessageBox::warning(parent, "", tr("Output stream no longer available...\nAborting\n"), QMessageBox::Ok);
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
            QMessageBox::critical(parent, "", tr("Can't play mp3 file!\nmpg123 is not installed or not in the $PATH\nPlease install it and restart the program\n"), QMessageBox::Ok);
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
            auto cmdForMe = "paplay --volume=" + to_string(ui->localVolumeSlider->value()) + " \"" + path + "\"";
            if (isMP3)
            {
                cmdForMe = "mpg123 -o pulse -f " + to_string(ui->localVolumeSlider->value() / 2) + " \"" + path + "\"";
            }
            system(cmdForMe.c_str());
        });
        forMe.detach();

        auto forOthers = std::thread([=]() {
            ui->stopButton->setDisabled(false);
            ui->remoteVolumeSlider->setDisabled(true);
            ui->localVolumeSlider->setDisabled(true);

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
            ui->localVolumeSlider->setDisabled(false);

            // Repeat when the check box is checked
            if (ui->repeatCheckBox->isChecked())
            {
                playSound(path);
            }
        });
        forOthers.detach();
    }
}
