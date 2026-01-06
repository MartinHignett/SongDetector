#include <QDebug>
#include <QMetaMethod>
#include <QObject>
#include <QStringView>
#include <QTimer>
#include <pipewire/core.h>
#include <pipewire/version.h>

extern "C" {
    #include <pipewire/keys.h>
    #include <pipewire/loop.h>
    #include <pipewire/pipewire.h>
    #include <pipewire/stream.h>
    #include <pipewire/thread-loop.h>
    #include <spa/param/audio/format-utils.h>
    #include <spa/param/audio/format.h>
    #include <spa/param/format.h>
    #include <spa/param/param.h>
    #include <spa/utils/type.h>
}

#include "pipewire_monitor.h"


/*
 * Constructor
 */
PipeWireMonitor::PipeWireMonitor(QString& applicationName, QAudioDevice* device, QObject* parent) :
    QObject(parent),
    m_useDefaultDevice(device == nullptr) {
        setApplicationName(applicationName);

        if (!m_useDefaultDevice) {
            QString deviceId = device->id();
            setDeviceId(&deviceId);
        }

        initializePipewire();
}

PipeWireMonitor::PipeWireMonitor(QString& applicationName, QString* deviceId, QObject* parent) :
    QObject(parent),
    m_useDefaultDevice(deviceId == nullptr) {
        setApplicationName(applicationName);

        if (!m_useDefaultDevice) {
            setDeviceId(deviceId);
        }

        initializePipewire();
}

PipeWireMonitor::PipeWireMonitor(QString& applicationName, QObject* parent) :
    QObject(parent),
    m_useDefaultDevice(true) {
        setApplicationName(applicationName);
        initializePipewire();
}

void PipeWireMonitor::setApplicationName(QString& applicationName) {
    const auto applicationNameBytes = applicationName.toUtf8();
    char* strApplicationName = new char[applicationNameBytes.size() + 1];
    strcpy(strApplicationName, applicationNameBytes.constData());
    m_applicationName.reset(strApplicationName);
}

void PipeWireMonitor::setDeviceId(QString* deviceId) {
    if (!m_useDefaultDevice) {
        const auto deviceIdBytes = deviceId->toUtf8();
        char* strDeviceId = new char[deviceIdBytes.size() + 1];
        strcpy(strDeviceId, deviceIdBytes.constData());
        m_deviceId.reset(strDeviceId);
    }
}

/*
 * Destructor
 */
PipeWireMonitor::~PipeWireMonitor() {
    if (m_stream != nullptr) {
        pw_thread_loop_lock(m_loop);
        pw_stream_destroy(m_stream);
        m_stream = nullptr;
        pw_thread_loop_unlock(m_loop);
    }

    if (m_context != nullptr) {
        pw_context_destroy(m_context);
    }

    if (m_loop != nullptr) {
        pw_thread_loop_destroy(m_loop);
    }

    pw_deinit();
}

/*******************************************************
 * Public APIs
 *******************************************************/

void PipeWireMonitor::startCapture(int minDurationInSeconds, QAudioDevice* device) {
    qDebug() << "Starting capture";

    if (!m_stream) {
        qDebug() << "No PipeWire stream!";
        return;
    }

    if (m_loop == nullptr) {
        qDebug() << "No PipeWire loop!";
        return;
    }

    // Disconnect any existing connections
    onStopCapture();
    m_audioBuffer.clear();
    m_isCapturing = true;

    // There must be a better way than waiting 500ms...
    QTimer::singleShot(500, this, [this] {
        this->connectToStream();
    });
}

/***********************************************
 * Getters
 ***********************************************/

int PipeWireMonitor::getBufferLengthInSeconds() {
    return m_bufferLengthInSeconds;
}

int PipeWireMonitor::getChannels() {
    return m_channels;
}

int PipeWireMonitor::getBitsPerSample() {
    return m_bytesPerSample * 8;
}

int PipeWireMonitor::getSampleRate() {
    return m_sampleRate;
}

QString PipeWireMonitor::getPipeWireVersion() {
    return m_pipeWireVersion;
}

/*******************************************************
 * Private methods
 *******************************************************/

void PipeWireMonitor::initializePipewire() {
    pw_init(nullptr, nullptr);

    m_pipeWireVersion = QString(pw_get_library_version());

    m_loop = pw_thread_loop_new(m_applicationName.data(), nullptr);
    m_context = pw_context_new(pw_thread_loop_get_loop(m_loop), nullptr, 0);
    m_core = pw_context_connect(m_context, nullptr, 0);

    pw_thread_loop_start(m_loop);
    pw_thread_loop_lock(m_loop);

    const auto properties = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Monitor",
        PW_KEY_MEDIA_ROLE, "Music",
        // Hard code to Bluez monitor for now...
        // PW_KEY_NODE_TARGET, "bluez_output.AC_80_0A_2B_EC_71.1.monitor"
        PW_KEY_STREAM_CAPTURE_SINK, "true",
        PW_KEY_APP_NAME, m_applicationName.data(),
        NULL);

    static const pw_stream_events stream_events = {
        .version = PW_VERSION_STREAM_EVENTS,
        // .state_changed = AudioStream::onStateChanged,
        .param_changed = PipeWireMonitor::onParamChanged,
        .process = PipeWireMonitor::onProcessAudio
    };

    m_stream = pw_stream_new_simple(
        pw_thread_loop_get_loop(m_loop),
        m_applicationName.data(),
        properties,
        &stream_events,
        (void *)this);

    if (!m_stream) {
        qCritical() << "Failed to create stream";
        pw_thread_loop_unlock(m_loop);
        return;
    }

    pw_thread_loop_unlock(m_loop);
}

void PipeWireMonitor::negotiateFormat(const struct spa_pod* param) {
    struct spa_audio_info format;
    if (spa_format_parse(param, &format.media_type, &format.media_subtype) < 0) {
        qCritical() << "Failed to parse media format";
        return;
    }

    if (format.media_type != SPA_MEDIA_TYPE_audio ||
        format.media_subtype != SPA_MEDIA_SUBTYPE_raw) {
        qCritical() << "Not audio/raw format";
        return;
    }

    if (spa_format_audio_raw_parse(param, &format.info.raw) < 0) {
        qCritical() << "Failed to parse audio format details";
        return;
    }

    // DON'T modify the format, just accept what's offered
    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    const struct spa_pod *params[1];

    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_Format, &format.info.raw);

    pw_stream_update_params(m_stream, params, 1);

    // Store format info
    m_sampleRate = format.info.raw.rate;
    m_channels = format.info.raw.channels;

    // Re-calculate the minimum buffer size
    m_minBufferSize = m_sampleRate * m_channels * m_bytesPerSample * m_bufferLengthInSeconds;

    qInfo() << "Final negotiated - Rate:" << m_sampleRate << "Channels:" << m_channels;
    qInfo() << "Accepted format and updated params";
}

void PipeWireMonitor::handleFinalFormat(const struct spa_pod* param) {
    // Parse the final format
    struct spa_audio_info format;
    if (spa_format_parse(param, &format.media_type, &format.media_subtype) < 0) {
        qCritical() << "Failed to parse final format";
        return;
    }

    if (format.media_type != SPA_MEDIA_TYPE_audio ||
        format.media_subtype != SPA_MEDIA_SUBTYPE_raw) {
        qCritical() << "Final format is not audio/raw";
        return;
    }

    if (spa_format_audio_raw_parse(param, &format.info.raw) < 0) {
        qCritical() << "Failed to parse final audio format details";
        return;
    }

    // For debuging purposes
    const char* format_name = "UNKNOWN";
    if (format.info.raw.format == SPA_AUDIO_FORMAT_S16_LE)
        format_name = "SPA_AUDIO_FORMAT_S16_LE (signed 16-bit LE)";
    else if (format.info.raw.format == SPA_AUDIO_FORMAT_U16_LE)
        format_name = "SPA_AUDIO_FORMAT_U16_LE (unsigned 16-bit LE)";
    else if (format.info.raw.format == SPA_AUDIO_FORMAT_F32_LE)
        format_name = "SPA_AUDIO_FORMAT_F32_LE (float 32-bit LE)";
    else if (format.info.raw.format == SPA_AUDIO_FORMAT_F32)
        format_name = "SPA_AUDIO_FORMAT_F32";

    // Store the final format info
    m_sampleRate = format.info.raw.rate;
    m_channels = format.info.raw.channels;

    // Re-calculate the minimum buffer size
    m_minBufferSize = m_sampleRate * m_channels * m_bytesPerSample * m_bufferLengthInSeconds;

    qInfo() << "Final stream parameters - Format:" << format_name << "Rate:" << m_sampleRate << "Channels:" << m_channels;
}

void PipeWireMonitor::paramChanged(void* userData, uint32_t id, const struct spa_pod* param) {
    if (param == nullptr) {
        return;
    }

    // Handle different parameter types
    switch(id) {
        case SPA_PARAM_EnumFormat:
            negotiateFormat(param);
            break;
        case SPA_PARAM_Format:
            handleFinalFormat(param);
            break;
        // default:
        //     qDebug() << "Unknown parameter id:" << id << "- ignoring";
    }
}

void PipeWireMonitor::readFromStream(void *userData) {
    if (!m_isCapturing.load()) {
        qDebug() << "Not capturing, ignoring callback";
        return;
    }

    pw_buffer* buf = pw_stream_dequeue_buffer(m_stream);

    if (!buf) {
        qCritical() << "No buffer available";
        return;
    }

    if (!buf->buffer->datas[0].data) {
        qWarning() << "Buffer data is null";
        pw_stream_queue_buffer(m_stream, buf);
        return;
    }

    // Get the PCM data from the buf
    const char* raw_pcm = static_cast<const char*>(buf->buffer->datas[0].data);
    int pcm_data_size = buf->buffer->datas[0].chunk->size;

    m_audioBuffer.append(raw_pcm, pcm_data_size);
    if (m_audioBuffer.size() < m_minBufferSize) {
        pw_stream_queue_buffer(m_stream, buf);
        return;
    }

    m_isCapturing = false;

    // Can't modify the PipeWire thread from within the thread itself,
    // so defer this to the main UI thread:
    QMetaObject::invokeMethod(this, "onStopCapture", Qt::QueuedConnection);

    // Can't send signals from the PipeWire thread,
    // so defer this to the main UI thread:
    QMetaObject::invokeMethod(this, "captureCompleted", Qt::QueuedConnection, Q_ARG(QByteArray, m_audioBuffer));
    return;
}

void PipeWireMonitor::onStopCapture() {
    qDebug() << "Stopping capture";

    m_isCapturing = false;

    if (!m_stream) {
        return;
    }

    pw_thread_loop_lock(m_loop);

    if (pw_stream_get_state(m_stream, nullptr) != PW_STREAM_STATE_UNCONNECTED) {
        pw_stream_disconnect(m_stream);
    }

    pw_thread_loop_unlock(m_loop);
}

void PipeWireMonitor::connectToStream() {
    pw_thread_loop_lock(m_loop);

    enum pw_stream_state current_state = pw_stream_get_state(m_stream, nullptr);

    if (current_state != PW_STREAM_STATE_UNCONNECTED) {
        qCritical() << "Stream not in unconnected state, aborting";
        pw_thread_loop_unlock(m_loop);
        return;
    }

    uint8_t buffer[1024];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    const struct spa_pod *params[1];

    struct spa_audio_info_raw audio_info = {};
    audio_info.format = SPA_AUDIO_FORMAT_F32;    // 32-bit float

    params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &audio_info);

    int result = pw_stream_connect(m_stream,
        PW_DIRECTION_INPUT,
        PW_ID_ANY,
        (pw_stream_flags)(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_RT_PROCESS),
        params, 1);

    if (result < 0) {
        qDebug() << "Error connecting to stream: " << result;
    } else {
        qDebug() << "Connected to stream";
    }

    pw_thread_loop_unlock(m_loop);
}

/******************************
 * PipeWire event handlers
 ******************************/

void PipeWireMonitor::onParamChanged(void* userData, uint32_t id, const struct spa_pod* param) {
    static_cast<PipeWireMonitor*>(userData)->paramChanged(userData, id, param);
}

void PipeWireMonitor::onProcessAudio(void* userData) {
    auto* self = static_cast<PipeWireMonitor*>(userData);
    self->readFromStream(userData);
}
