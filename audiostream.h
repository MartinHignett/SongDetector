#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

#include <QApplication>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QObject>
#include <QRestAccessManager>
#include <QRestReply>
#include <QSettings>
#include <pipewire/pipewire.h>
#include <qaudiodevice.h>
#include <qmediadevices.h>

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
        void shazamLookupFinished();
        void activateStreamWhenReady();

    signals:
        void audioDevicesChanged(QList<QAudioDevice> *devices);
        // void currentAudioDeviceChanged(QAudioDevice *device);

    private:
        QMediaDevices       *m_mediaDevices;
        QList<QAudioDevice> m_audioDevices;
        QAudioDevice        m_currentAudioDevice;
        QSettings*          m_settings;
        // Stores the captued audio in PCM format
        QByteArray          m_audioBuffer;
        // Makes sure that we don't keep modifying m_audioBuffer
        // once we have enough data.
        std::atomic<bool>   m_isCapturing{true};

        /* PipeWire properties */
        pw_thread_loop*     m_loop = nullptr;
        pw_context*         m_context = nullptr;
        pw_properties*      m_properties = nullptr;
        pw_core*            m_core = nullptr;
        struct spa_hook     m_stream_listener = {};
        struct pw_registry* m_registry = nullptr;
        struct spa_hook     m_registry_listener;
        uint32_t            m_monitor_fl_port_id = 0;
        uint32_t            m_monitor_fr_port_id = 0;
        uint32_t            m_input_port_id = 0;

        int                 m_sampleRate  = 44100;  // Sample rate
        int                 m_channels    = 1;      // Number of channels
        int                 m_bytesPerSample = 4;   // Bytes per sample
        int                 m_bufferLengthInSeconds = 10;
        int                 m_minBufferSize = m_sampleRate * m_channels * m_bytesPerSample * m_bufferLengthInSeconds;

        void                createLinks();
        void                forceConnection();
        void                initializePipewire();
        void                connectToStream();
        void                negotiateFormat(const struct spa_pod* param);
        void                handleFinalFormat(const struct spa_pod* param);

        /*
         * Shazam Methods
         */
        void                shazamLookup(const QString& uri);
        void                getRequestContent(const char* uri, const unsigned int sample_ms);

        /*
         * PipeWire Event Handlers
         */
        static void         onProcessAudio(void *userData);
        static void         onParamChanged(void* userData, uint32_t id, const struct spa_pod* param);

        // This is for debugging only
        // static void         onStateChanged(void* userData, enum pw_stream_state old_state, enum pw_stream_state state, const char* error);
};

#endif // AUDIOSTREAM_H
