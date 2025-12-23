#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

#include <QApplication>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QObject>
#include <QSettings>
#include <pipewire/pipewire.h>
#include <qaudiodevice.h>
#include <qmediadevices.h>
#include <qobject.h>

class AudioStream : public QObject {
    Q_OBJECT;

    public:
        AudioStream(QApplication *parent = nullptr);
        ~AudioStream();

        QAudioDevice        getCurrentAudioDevice();
        QList<QAudioDevice> getAudioDevices();

        pw_stream*          m_stream;

        void paramChanged(void* userData, uint32_t id, const struct spa_pod* param);
        void processAudio(void *userData);

    public slots:
        void refreshAudioDevices();
        void setCurrentAudioDevice(QAudioDevice device);
        void startIdentify();
        void stopIdentify();

    signals:
        void audioDevicesChanged(QList<QAudioDevice> *devices);
        // void currentAudioDeviceChanged(QAudioDevice *device);

    private:
        QMediaDevices       *m_mediaDevices;
        QList<QAudioDevice> m_audioDevices;
        QAudioDevice        m_currentAudioDevice;
        QSettings*          m_settings;
        QByteArray          m_audioBuffer;  // Add this to accumulate audio

        pw_thread_loop*     m_loop = nullptr;
        pw_context*         m_context = nullptr;
        pw_properties*      m_properties = nullptr;
        pw_core*            m_core = nullptr;
        struct spa_hook     m_stream_listener = {};
        int                 m_sampleRate  = 44100; // Sample rate
        int                 m_channels    = 2; // Number of channels
        int                 m_sampleWidth = 16; // bytes per sample
        int                 m_bufferLengthInSeconds = 10;
        int                 m_minBufferSize = m_sampleRate * m_channels * m_bufferLengthInSeconds;

        void                initializePipewire();
        void                connectToStream();
        void                negotiateFormat(const struct spa_pod* param);
        static void         onProcessAudio(void *userData);
        static void         onParamChanged(void* userData, uint32_t id, const struct spa_pod* param);

        // This is for debugging only
        static void         onStateChanged(void* userData, enum pw_stream_state old_state, enum pw_stream_state state, const char* error);
};

#endif // AUDIOSTREAM_H
