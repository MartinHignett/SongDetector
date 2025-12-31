
#include <QApplication>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>
#include <qsystemtrayicon.h>


#include "qvibra.h"
#include "audiostream.h"
#include "settingsdialog.h"

QVibra::QVibra(QApplication* app) {
    auto audioStream = m_audioStream = new AudioStream(this);

    connect(m_audioStream, &AudioStream::songIdentified, this, &QVibra::onSongIdentified);

    // Setup system tray menu...
    m_menu.addAction(QCoreApplication::translate("ContextMenu", "Settings..."), [audioStream]{
        const auto settingsDialog = new SettingsDialog(audioStream);
        settingsDialog->show();
    });
    m_menu.addAction(QCoreApplication::translate("ContextMenu", "Quit"), app, &QApplication::quit);

    // Create a system tray icon and show it
    m_trayIcon.setContextMenu(&m_menu);
    m_trayIcon.setIcon(QIcon(":/resources/app.ico"));
    m_trayIcon.show();
}

void QVibra::onSongIdentified(const Song* song) {
    qDebug() << "onSongIdentified";

    m_trayIcon.showMessage(
        "QVibra - Song Identified",
        QString("Found %1 by %2").arg(song->getTitle(), song->getArtist()),
        QSystemTrayIcon::Information,
        5000
    );
}
