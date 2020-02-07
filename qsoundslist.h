#ifndef QSOUNDSLIST_H
#define QSOUNDSLIST_H

#include <QObject>
#include <QListWidget>

using namespace std;

class QSoundsList : public QListWidget
{
    Q_OBJECT

public:
    explicit QSoundsList();
    string directory = "";
private:

signals:

};

#endif // QSOUNDSLIST_H
