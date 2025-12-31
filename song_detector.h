#ifndef SONG_DETECTOR_H
#define SONG_DETECTOR_H

#include <QObject>
#include <QSystemTrayIcon>
#include <pthread.h>
#include <qmenu.h>
#include <qtmetamacros.h>

#include "song.h"
#include "audiostream.h"

class SongDetector : public QObject {
    Q_OBJECT;

public:
    SongDetector(QApplication* app);

public slots:
    void onSongIdentified(const Song* song);

private:
    AudioStream*    m_audioStream;
    QSystemTrayIcon m_trayIcon;
    QMenu           m_menu;
};

#endif // SONG_DETECTOR_H
