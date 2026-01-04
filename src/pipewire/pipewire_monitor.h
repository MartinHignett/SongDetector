#pragma once

#include <QObject>
#include <QAudioDevice>
#include <pipewire/pipewire.h>
#include <qcontainerfwd.h>
#include <qobject.h>
#include <qscopedpointer.h>

class PipeWireMonitor : public QObject {
    Q_OBJECT

    public:
        PipeWireMonitor(QString& applicationName, QAudioDevice* device = nullptr, QObject* parent = nullptr);
        PipeWireMonitor(QString& applicationName, QString* deviceId = nullptr, QObject* parent = nullptr);
        PipeWireMonitor(QString& applicationName, QObject* parent = nullptr);
        ~PipeWireMonitor();

        void    startCapture(int minDurationInSeconds, QAudioDevice* device = nullptr);

        /*
         * Getters
         */
        int     getBufferLengthInSeconds();
        int     getSampleRate();
        int     getBitsPerSample();
        int     getChannels();
        QString getPipeWireVersion();

    signals:

        /*
         * Raised when the stream has started capturing
         */
        void started();

        /*
         * Raised when the monitor has read at least `minDurationInSeconds`
         * of data.
         */
        void captureCompleted(QByteArray buffer);

        // TODO: Progress indicator
        // void progressUpdate(int secondsProcessed);

    public slots:
        void    onStopCapture();
        // TODO: Support cancellation
        // void cancelCapture();

    private:
        void                initializePipewire();
        void                setApplicationName(QString& applicationName);
        void                setDeviceId(QString* deviceId);
        void                paramChanged(void* userData, uint32_t id, const struct spa_pod* param);
        void                negotiateFormat(const struct spa_pod* param);
        void                handleFinalFormat(const struct spa_pod* param);
        void                readFromStream(void *userData);
        void                connectToStream();

        /*
         * These are char* because that is what the PipeWire API needs
         */
        QScopedArrayPointer<char> m_applicationName;
        QScopedArrayPointer<char> m_deviceId;

        QString             m_pipeWireVersion;

        // True if m_deviceId has not been set
        const bool          m_useDefaultDevice;

        // Makes sure that we don't keep modifying m_audioBuffer
        // once we have enough data.
        std::atomic<bool>   m_isCapturing{true};

        // Stores the captured audio in PCM format
        QByteArray          m_audioBuffer;

        /*
        * PipeWire event handlers
        */
        static void     onProcessAudio(void *userData);
        static void     onParamChanged(void *userData, uint32_t id, const struct spa_pod *param);

        /*
         * PipeWire data structures
         */
        pw_thread_loop      *m_loop = nullptr;
        pw_context*         m_context = nullptr;
        pw_properties*      m_properties = nullptr;
        pw_core*            m_core = nullptr;
        pw_stream*          m_stream;
        struct spa_hook     m_stream_listener = {};
        struct pw_registry  *m_registry = nullptr;
        struct spa_hook     m_registry_listener;
        uint32_t            m_monitor_fl_port_id = 0;
        uint32_t            m_monitor_fr_port_id = 0;
        uint32_t            m_input_port_id = 0;

        int                 m_sampleRate  = 44100;  // Sample rate
        int                 m_channels    = 1;      // Number of channels
        int                 m_bytesPerSample = 4;   // Bytes per sample
        int                 m_bufferLengthInSeconds = 10;
        int                 m_minBufferSize = 0; // m_sampleRate * m_channels * m_bytesPerSample * m_bufferLengthInSeconds;
};
