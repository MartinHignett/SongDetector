#ifndef QVIBRA_H
#define QVIBRA_H

#include <QObject>
#include <QSystemTrayIcon>
#include <pthread.h>
#include <qmenu.h>
#include <qtmetamacros.h>

#include "song.h"
#include "audiostream.h"

class QVibra : public QObject {
    Q_OBJECT;

public:
    QVibra(QApplication* app);

public slots:
    void onSongIdentified(const Song* song);

private:
    AudioStream*    m_audioStream;
    QSystemTrayIcon m_trayIcon;
    QMenu           m_menu;
};

#endif // QVIBRA_H
