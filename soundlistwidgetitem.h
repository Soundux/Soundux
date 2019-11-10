#ifndef SOUNDLISTWIDGETITEM_H
#define SOUNDLISTWIDGETITEM_H

#include <QObject>
#include <QWidget>
#include <QListWidgetItem>

class SoundListWidgetItem : public QListWidgetItem
{
public:
    explicit SoundListWidgetItem(QListWidget *parent = nullptr);
    explicit SoundListWidgetItem(QString content, QListWidget *parent = nullptr);
    ~SoundListWidgetItem();
    void setHotkey(QVariant hotkey);
    QVariant hotkey;
};

#endif // SOUNDLISTWIDGETITEM_H
