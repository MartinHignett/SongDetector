#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

#include <QApplication>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QObject>
#include <qaudiodevice.h>
#include <qmediadevices.h>
#include <qobject.h>

class AudioStream : public QObject {
    Q_OBJECT;

    public:
        AudioStream(QApplication *parent = nullptr);

        QAudioDevice        getCurrentAudioDevice();
        QList<QAudioDevice> getAudioDevices();

    private:
        QMediaDevices       *m_mediaDevices;
        QList<QAudioDevice> m_audioDevices;
        QAudioDevice        m_currentAudioDevice;

    public slots:
        void refreshAudioDevices();
        void setCurrentAudioDevice(QAudioDevice device);

    signals:
        void audioDevicesChanged(QList<QAudioDevice> *devices);
        // void currentAudioDeviceChanged(QAudioDevice *device);
};

#endif // AUDIOSTREAM_H
