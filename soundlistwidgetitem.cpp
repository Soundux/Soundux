#include "soundlistwidgetitem.h"

SoundListWidgetItem::SoundListWidgetItem(QListWidget *parent) : QListWidgetItem(parent)
{

}

SoundListWidgetItem::SoundListWidgetItem(QString content, QListWidget *parent) : QListWidgetItem(content, parent)
{

}

SoundListWidgetItem::~SoundListWidgetItem()
{

}

void SoundListWidgetItem::setHotkey(QVariant hotkey)
{
    this->hotkey = hotkey;
}
