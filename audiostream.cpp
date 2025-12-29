#include <QApplication>
#include <QAudioDevice>
#include <QJsonDocument>
#include <QMediaDevices>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRestAccessManager>
#include <QTimer>
#include <QObject>
#include <QVariant>
#include <pipewire/keys.h>
#include <pipewire/stream.h>
#include <pipewire/thread-loop.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qnetworkrequest.h>
#include <qobjectdefs.h>
#include <quuid.h>
#include <spa/param/param.h>
#include <spa/utils/type.h>


extern "C" {
    #include <pipewire/loop.h>
    #include <pipewire/pipewire.h>
    #include <spa/param/audio/format-utils.h>
    #include <spa/param/audio/format.h>
    #include <spa/param/format.h>
}

#include <vibra.h>

#include "audiostream.h"
#include "shazam_body.h"

#define SELECTED_DEVICE_SETTING "deviceId"
#define SHAZAM_URL QStringLiteral("https://amp.shazam.com/discovery/v5/en/US/android/-/tag/")
#define SHAZAM_QUERY_PARAMS QStringLiteral("?sync=true&webv3=true&sampling=true&connected=&shazamapiversion=v3&sharehub=true&video=v3")

AudioStream::AudioStream(QApplication *parent)
    : QObject(parent),
   m_currentAudioDevice(QMediaDevices::defaultAudioOutput()) {

    initializePipewire();

    m_settings = new QSettings(this);
    m_mediaDevices = new QMediaDevices(this);
    m_audioDevices = QMediaDevices::audioOutputs();

    if (m_settings->contains(SELECTED_DEVICE_SETTING)) {
        const auto deviceId = m_settings->value(SELECTED_DEVICE_SETTING);
        const auto devices = m_mediaDevices->audioOutputs();
        for (QAudioDevice device : devices) {
            if (device.id() == deviceId) {
                m_currentAudioDevice = device;
            }
        }
    }

    connect(m_mediaDevices, &QMediaDevices::audioOutputsChanged, this, &AudioStream::refreshAudioDevices);
}

AudioStream::~AudioStream() {
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

/******************************
 * Public Methods
 ******************************/

QAudioDevice AudioStream::getCurrentAudioDevice() {
    return m_currentAudioDevice;
}

QList<QAudioDevice> AudioStream::getAudioDevices() {
    return m_audioDevices;
}

/******************************
 * Slots
 ******************************/

 void AudioStream::refreshAudioDevices() {
     m_audioDevices = QMediaDevices::audioOutputs();
     audioDevicesChanged(&m_audioDevices);
 }

 void AudioStream::setCurrentAudioDevice(QAudioDevice device) {
     m_currentAudioDevice = device;
     m_settings->setValue(SELECTED_DEVICE_SETTING, QVariant(device.id()));
 }

 void AudioStream::startIdentify() {
     qDebug() << "Starting identify";

     if (!m_stream) {
         qDebug() << "No PipeWire stream!";
         return;
     }

     if (m_loop == nullptr) {
         qDebug() << "No PipeWire loop!";
         return;
     }

     // Disconnect any existing connections
     stopIdentify();

     m_isCapturing = true;

     // There must be a better way than waiting 500ms...
     QTimer::singleShot(500, this, [this] {
         qDebug() << "From startIdentify()";
         this->connectToStream();
     });
 }

 void AudioStream::stopIdentify() {
     qDebug() << "Stopping identify";

     m_isCapturing = false;

     if (!m_stream) {
         return;
     }

     pw_thread_loop_lock(m_loop);

     if (pw_stream_get_state(m_stream, nullptr) != PW_STREAM_STATE_UNCONNECTED) {
         pw_stream_disconnect(m_stream);
         qDebug() << "Stream disconnected";
     }

     pw_thread_loop_unlock(m_loop);

     m_audioBuffer.clear();
 }

/******************************
 * PipeWire logic
 ******************************/

void AudioStream::initializePipewire() {
    pw_init(nullptr, nullptr);

    m_loop = pw_thread_loop_new("QVibra", nullptr);
    m_context = pw_context_new(pw_thread_loop_get_loop(m_loop), nullptr, 0);
    m_core = pw_context_connect(m_context, nullptr, 0);

    pw_thread_loop_start(m_loop);
    pw_thread_loop_lock(m_loop);

    const auto properties = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Music",
        // Hard code to Bluez monitor for now...
        // PW_KEY_NODE_TARGET, "bluez_output.AC_80_0A_2B_EC_71.1.monitor"
        PW_KEY_STREAM_CAPTURE_SINK, "true",
        PW_KEY_APP_NAME, "QVibra",
        NULL);


    static const pw_stream_events stream_events = {
        .version = PW_VERSION_STREAM_EVENTS,
        // .state_changed = AudioStream::onStateChanged,
        .param_changed = AudioStream::onParamChanged,
        .process = AudioStream::onProcessAudio
    };

    m_stream = pw_stream_new_simple(
        pw_thread_loop_get_loop(m_loop),
        "QVibra",
        properties,
        &stream_events,
        (void *)this);

    if (!m_stream) {
        qDebug() << "Failed to create stream";
        pw_thread_loop_unlock(m_loop);
        return;
    }

    pw_thread_loop_unlock(m_loop);
}

void AudioStream::paramChanged(void* userData, uint32_t id, const struct spa_pod* param) {
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
        default:
            qDebug() << "Unknown parameter id:" << id << "- ignoring";
    }
}

void AudioStream::handleFinalFormat(const struct spa_pod* param) {
    // Parse the final format
    struct spa_audio_info format;
    if (spa_format_parse(param, &format.media_type, &format.media_subtype) < 0) {
        qDebug() << "Failed to parse final format";
        return;
    }

    if (format.media_type != SPA_MEDIA_TYPE_audio ||
        format.media_subtype != SPA_MEDIA_SUBTYPE_raw) {
        qDebug() << "Final format is not audio/raw";
        return;
    }

    if (spa_format_audio_raw_parse(param, &format.info.raw) < 0) {
        qDebug() << "Failed to parse final audio format details";
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

    qDebug() << "Final stream parameters - Format:" << format_name << "Rate:" << m_sampleRate << "Channels:" << m_channels;
}

void AudioStream::negotiateFormat(const struct spa_pod* param) {
    struct spa_audio_info format;
    if (spa_format_parse(param, &format.media_type, &format.media_subtype) < 0) {
        qDebug() << "Failed to parse media format";
        return;
    }

    if (format.media_type != SPA_MEDIA_TYPE_audio ||
        format.media_subtype != SPA_MEDIA_SUBTYPE_raw) {
        qDebug() << "Not audio/raw format";
        return;
    }

    if (spa_format_audio_raw_parse(param, &format.info.raw) < 0) {
        qDebug() << "Failed to parse audio format details";
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

    qDebug() << "Final negotiated - Rate:" << m_sampleRate << "Channels:" << m_channels;
    qDebug() << "Accepted format and updated params";
}

/*
 * Assumes that m_loop and m_stream are valid
 */
void AudioStream::connectToStream() {
    pw_thread_loop_lock(m_loop);

    enum pw_stream_state current_state = pw_stream_get_state(m_stream, nullptr);

    if (current_state != PW_STREAM_STATE_UNCONNECTED) {
        qDebug() << "Stream not in unconnected state, aborting";
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

void AudioStream::activateStreamWhenReady() {
    if (!m_stream) return;

    pw_thread_loop_lock(m_loop);

    enum pw_stream_state state = pw_stream_get_state(m_stream, nullptr);

    if (state == PW_STREAM_STATE_PAUSED) {
        // Add debug to see what we're actually targeting
        const struct pw_properties* props = pw_stream_get_properties(m_stream);
        if (props) {
            const char* target = pw_properties_get(props, PW_KEY_NODE_TARGET);
            qDebug() << "Stream target:" << (target ? target : "none");
        }

        bool result = pw_stream_set_active(m_stream, true);

        if (!result) {
            qDebug() << "Activation failed, will retry in 100ms...";
            pw_thread_loop_unlock(m_loop);
            QTimer::singleShot(100, this, &AudioStream::activateStreamWhenReady);
            return;
        }
    }

    pw_thread_loop_unlock(m_loop);
}

void AudioStream::processAudio(void *userData) {
    static int callCount = 0;

    pw_buffer* buf = pw_stream_dequeue_buffer(m_stream);
    if (!buf) {
        qDebug() << "No buffer available";
        return;
    }

    if (!buf->buffer->datas[0].data) {
        qDebug() << "Buffer data is null";
        pw_stream_queue_buffer(m_stream, buf);
        return;
    }

    // Get the PCM data from the buf
    const char* raw_pcm = static_cast<const char*>(buf->buffer->datas[0].data);
    int pcm_data_size = buf->buffer->datas[0].chunk->size;

    if (m_isCapturing.load()) {
        m_audioBuffer.append(raw_pcm, pcm_data_size);
        if (m_audioBuffer.size() < m_minBufferSize) {
            pw_stream_queue_buffer(m_stream, buf);
            return;
        }

        m_isCapturing = false;

        // Use Vibra fingerprinting
        const auto fp = vibra_get_fingerprint_from_signed_pcm(
            m_audioBuffer.data(),
            m_audioBuffer.size(),
            m_sampleRate,
            m_bytesPerSample * 8,
            m_channels
        );

        auto uri = QString::fromUtf8(vibra_get_uri_from_fingerprint(fp));

        QMetaObject::invokeMethod(this, [this, uri]() {
            shazamLookup(uri);
        }, Qt::QueuedConnection);

        // Stop after identification
        QMetaObject::invokeMethod(this, "stopIdentify", Qt::QueuedConnection);
    }

    pw_stream_queue_buffer(m_stream, buf);
}

void AudioStream::shazamLookup(const QString& uri) {
    const auto networkAccessManager = new QNetworkAccessManager(this);
    const auto restAccessManager = new QRestAccessManager(networkAccessManager, this);

    ShazamBody shazamBody(uri, m_bufferLengthInSeconds);
    const auto jsonBody = shazamBody.toJsonDocument();

    const QString url =
        SHAZAM_URL +
        QUuid::createUuid().toString(QUuid::WithoutBraces) +
        "/" +
        QUuid::createUuid().toString(QUuid::WithoutBraces) +
        SHAZAM_QUERY_PARAMS;

    auto request = QNetworkRequest(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader, "Dalvik/2.1.0 (Linux; U; Android 6.0.1; SM-G920F Build/MMB29K)");
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Content-Language", "en_US");

    const auto response = restAccessManager->post(request, jsonBody);
    QObject::connect(response, &QNetworkReply::finished, this, [response, restAccessManager, networkAccessManager]() {
        qDebug() << "Shazam finished!";

        if (response) {
            QRestReply restResponse(response);
            qDebug() << restResponse.readBody();
            response->deleteLater();
        }

        restAccessManager->deleteLater();
        networkAccessManager->deleteLater();
    });
}

void AudioStream::shazamLookupFinished() {
    qDebug() << "Shazam finished!";

    auto reply = qobject_cast<QNetworkReply*>(sender());

    if (reply) {
        QRestReply response(reply);
        qDebug() << "HTTP Status: " << response.httpStatus();
        qDebug() << "HTTP: " << response.errorString();
        qDebug() << "Shazam response: " << response.readBody();
        reply->deleteLater();
    }
}

/******************************
 * PipeWire event handlers
 ******************************/

void AudioStream::onParamChanged(void* userData, uint32_t id, const struct spa_pod* param) {
    static_cast<AudioStream*>(userData)->paramChanged(userData, id, param);
}

void AudioStream::onProcessAudio(void* userData) {
    auto* self = static_cast<AudioStream*>(userData);
    self->processAudio(userData);
}
