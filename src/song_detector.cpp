
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>
#include <qsystemtrayicon.h>
#include <vibra.h>

#include "song_detector.h"
#include "pipewire/pipewire_monitor.h"
#include "settingsdialog.h"
#include "settings.h"

SongDetector::SongDetector(QApplication* app) :
    m_applicationName("SongDetector"),
    m_pipeWireMonitor(nullptr),
    m_shazam(this) {
    initialisePipeWire();

    connect(&m_shazam, &Shazam::detectionComplete, this, &SongDetector::onDetectionComplete);

    // Setup system tray menu...
    m_menu.addAction(QCoreApplication::translate("ContextMenu", "Settings..."), this, &SongDetector::onOpenSettings);
    m_menu.addAction(QCoreApplication::translate("ContextMenu", "Start Identify"), this, &SongDetector::onStartDetection);
    m_menu.addAction(QCoreApplication::translate("ContextMenu", "Quit"), app, &QApplication::quit);

    // Create a system tray icon and show it
    m_trayIcon.setContextMenu(&m_menu);
    setTrayIcon();
    m_trayIcon.show();
}

void SongDetector::initialisePipeWire() {
    // Delete any existing PipeWire monitor instance
    if (m_pipeWireMonitor != nullptr) {
        delete m_pipeWireMonitor;
    }

    m_pipeWireMonitor = new PipeWireMonitor(m_applicationName, this);
    connect(m_pipeWireMonitor, &PipeWireMonitor::captureCompleted, this, &SongDetector::onCaptureCompleted);
}

void SongDetector::setTrayIcon() {
    bool darkModeIcon = false;
    QSettings settings(this);

    if (settings.contains(DARK_TRAY_ICON_SETTING)) {
        darkModeIcon = settings.value(DARK_TRAY_ICON_SETTING) == true;
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
void SongDetector::onStartDetection() {
    m_pipeWireMonitor->startCapture(10);
}

void SongDetector::onCaptureCompleted(QByteArray audioBuffer) {
    qDebug() << "onCaptureCompleted";

    const auto fp = vibra_get_fingerprint_from_signed_pcm(
        audioBuffer.data(),
        audioBuffer.size(),
        m_pipeWireMonitor->getSampleRate(),
        m_pipeWireMonitor->getBitsPerSample(),
        m_pipeWireMonitor->getChannels()
    );

    const auto uri = QString::fromUtf8(vibra_get_uri_from_fingerprint(fp));
    m_shazam.detectFromUri(uri, m_pipeWireMonitor->getBufferLengthInSeconds());
}

void SongDetector::onDetectionComplete(const ShazamResponse& response) {
    qDebug() << "onDetectionComplete";

    if (response.getFound()) {
        m_trayIcon.showMessage(
            "SongDetector - Song identified",
            QString("Found %1 by %2").arg(response.getTitle(), response.getArtist()),
            QSystemTrayIcon::Information,
            5000
        );
    } else {
        m_trayIcon.showMessage(
            "SongDetector - Failed to identify song",
            "",
            QSystemTrayIcon::Warning,
            5000
        );
    }
}

void SongDetector::onOpenSettings() {
    const auto settingsDialog = new SettingsDialog(nullptr);
    settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(settingsDialog, &SettingsDialog::forceDarkModeChanged, this, &SongDetector::onForceDarkIconChanged);
    connect(settingsDialog, &SettingsDialog::currentDeviceChanged, this, &SongDetector::onCurrentDeviceChanged);
    settingsDialog->show();
}

void SongDetector::onForceDarkIconChanged() {
    setTrayIcon();
}

void SongDetector::onCurrentDeviceChanged(const QString& deviceId) {
    qDebug() << "TODO: onCurrentDeviceChanged";
}
