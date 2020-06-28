#ifndef SEARCHVIEW_H
#define SEARCHVIEW_H

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <fstream>
#include <locale>

#include <QDockWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QTabWidget>
#include <QListWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <qsoundslist.h>
#include <soundplayback.h>
#include <soundlistwidgetitem.h>

class SearchView : public QDockWidget
{
    Q_OBJECT
public:
    explicit SearchView(QWidget *parent = nullptr, QTabWidget *tabWidget = nullptr, SoundPlayback *soundPlayback = nullptr);
private:
    QSoundsList* soundsList;
    QTabWidget* tabWidget;
    SoundPlayback* soundPlayback;
    QLineEdit* searchBox;
private slots:
    void on_searchBox_textChanged(const QString &text);
    void on_soundsList_itemDoubleClicked(QListWidgetItem *listWidgetItem);
    void on_visibilityChanged(bool visible);
};

#endif // SEARCHVIEW_H
