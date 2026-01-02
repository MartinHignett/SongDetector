#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <pthread.h>
#include <qcontainerfwd.h>
#include <qmenu.h>
#include <qtmetamacros.h>

#include "pipewire/pipewire_monitor.h"
#include "shazam/shazam.h"

class SongDetector : public QObject {
    Q_OBJECT

public:
    SongDetector(QApplication* app);

public slots:
    void                onForceDarkIconChanged();
    void                onStartDetection();
    void                onOpenSettings();
    void                onCaptureCompleted(QByteArray audioBuffer);
    void                onDetectionComplete(const ShazamResponse& response);
    void                onCurrentDeviceChanged(const QString& deviceId);

private:
    PipeWireMonitor*    m_pipeWireMonitor = nullptr;
    Shazam              m_shazam;
    QSystemTrayIcon     m_trayIcon;
    QMenu               m_menu;
    QString             m_applicationName;

    void                setTrayIcon();
    void                initialisePipeWire();
};
