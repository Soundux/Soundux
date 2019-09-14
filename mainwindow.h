#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QFileDialog>
#include <QListWidgetItem>
#include <json.hpp>

using namespace std;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct PulseAudioRecordingStream
{
        int index;
        string driver;
        string flags;
        string state;
        string source;
        bool muted;
        string applicationName;
        int processId;
        string processBinary;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void closeEvent(QCloseEvent *event);

private slots:
    void on_refreshAppsButton_clicked();

    bool isValidDevice(PulseAudioRecordingStream* stream);

    void loadSources();

    void playSound(string path);

    void clearSoundFiles();

    void saveSoundFiles();

    void loadSoundFiles();

    void on_playCustomButton_clicked();

    void on_customFileChoose_clicked();

    void on_stopButton_clicked();

    void on_addSoundButton_clicked();

    void on_removeSoundButton_clicked();

    void on_clearSoundsButton_clicked();

    void on_playSoundButton_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
