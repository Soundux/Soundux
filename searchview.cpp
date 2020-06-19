#include "searchview.h"

#include <mainwindow.h>

SearchView::SearchView(QWidget *parent, QTabWidget *tabWidget, SoundPlayback* soundPlayback) : QDockWidget(parent)
{
    this->tabWidget = tabWidget;
    this->soundPlayback = soundPlayback;

    auto layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setMargin(0);

    auto searchBox = new QLineEdit(this);
    layout->addWidget(searchBox);
    layout->setAlignment(searchBox, Qt::AlignTop);

    soundsList = new QSoundsList();
    soundsList->setObjectName("searchResults");

    connect(soundsList, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(on_soundsList_itemDoubleClicked(QListWidgetItem *)));

    layout->addWidget(soundsList);
    layout->setAlignment(soundsList, Qt::AlignJustify);


    connect(searchBox, SIGNAL(textChanged(QString)), this, SLOT(on_searchBox_textChanged(QString)));

    auto widget = new QWidget(this);
    widget->setLayout(layout);

    setWidget(widget);
    setWindowTitle("Search");
    setFeatures(DockWidgetClosable);
    setMinimumWidth(250);
}

// TODO: right click scrollToItem()

void SearchView::on_searchBox_textChanged(const QString &text) {
    auto searchText = text.toStdString();
    transform(searchText.begin(), searchText.end(), searchText.begin(), [](char c) { return tolower(c); });

    this->soundsList->clear();

    if (searchText.length() <= 0) {
        return;
    }

    for (auto i = 0; i < tabWidget->count(); i++)
    {
        QSoundsList *listWidget = (QSoundsList *)tabWidget->widget(i);

        for (auto *_item : listWidget->findItems("*", Qt::MatchWildcard))
        {
            auto item = (SoundListWidgetItem*) _item;
            auto name = item->text().toStdString();
            transform(name.begin(), name.end(), name.begin(), [](char c) { return tolower(c); });
            auto path = item->toolTip().toStdString();

            if (name.find(searchText) != string::npos) {
                // why do we have to create a new item here?
                auto newItem = new SoundListWidgetItem(this->soundsList);
                newItem->setText(item->text());
                newItem->setToolTip(item->toolTip());

                this->soundsList->addItem(newItem);
                //this->soundsList->addItem(item);
            }

        }

    }
}


void SearchView::on_soundsList_itemDoubleClicked(QListWidgetItem *listWidgetItem)
{
    if (listWidgetItem)
    {
        this->soundPlayback->playSound(listWidgetItem->toolTip().toStdString());
    }
}
