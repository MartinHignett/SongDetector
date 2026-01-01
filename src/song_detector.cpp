
#include <QApplication>
#include <QDir>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>
#include <qapplication.h>
#include <qguiapplication.h>
#include <qsystemtrayicon.h>

#include "song_detector.h"
#include "audiostream.h"
#include "settingsdialog.h"
#include "settings.h"

SongDetector::SongDetector(QApplication* app) {
    auto audioStream = m_audioStream = new AudioStream(this);

    connect(m_audioStream, &AudioStream::songIdentified, this, &SongDetector::onSongIdentified);

    // Setup system tray menu...
    m_menu.addAction(QCoreApplication::translate("ContextMenu", "Settings..."), [audioStream, this]{
        const auto settingsDialog = new SettingsDialog(audioStream);
        connect(settingsDialog, &SettingsDialog::forceDarkModeChanged, this, &SongDetector::onForceDarkIconChanged);
        settingsDialog->show();
    });
    m_menu.addAction(QCoreApplication::translate("ContextMenu", "Quit"), app, &QApplication::quit);

    // Create a system tray icon and show it
    m_trayIcon.setContextMenu(&m_menu);
    setTrayIcon();
    m_trayIcon.show();
}

void SongDetector::setTrayIcon() {
    bool darkModeIcon = false;
    auto settings = new QSettings(this);

    if (settings->contains(DARK_TRAY_ICON_SETTING)) {
        darkModeIcon = settings->value(DARK_TRAY_ICON_SETTING) == true;
    } else {
        // Try to guess if we should use dark mode
        QPalette palette = QApplication::palette();
        QColor backgroundColor = palette.color(QPalette::AlternateBase);

        // Check the brightness of the background colour
        darkModeIcon = backgroundColor.lightness() < 128;
    }

    if (darkModeIcon) {
        m_trayIcon.setIcon(QIcon(":/resources/icons/app-dark-mode.svg"));
    } else {
        m_trayIcon.setIcon(QIcon(":/resources/icons/app-light-mode.svg"));
    }
}

/*
 * Slots
 */
void SongDetector::onSongIdentified(const Song* song) {
    qDebug() << "onSongIdentified";

    m_trayIcon.showMessage(
        "SongDetector - Song Identified",
        QString("Found %1 by %2").arg(song->getTitle(), song->getArtist()),
        QSystemTrayIcon::Information,
        5000
    );
}

void SongDetector::onForceDarkIconChanged() {
    setTrayIcon();
}
