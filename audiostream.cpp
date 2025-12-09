#include <QApplication>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QObject>
#include <QVariant>

#include "audiostream.h"

#define SELECTED_DEVICE_SETTING "deviceId"

AudioStream::AudioStream(QApplication *parent)
    : QObject(parent),
   m_currentAudioDevice(QMediaDevices::defaultAudioOutput()) {
    m_settings = new QSettings(this);
    this->m_mediaDevices = new QMediaDevices(this);
    this->m_audioDevices = QMediaDevices::audioOutputs();

    if(this->m_settings->contains(SELECTED_DEVICE_SETTING)) {
        const auto deviceId = this->m_settings->value(SELECTED_DEVICE_SETTING);
        const auto devices = this->m_mediaDevices->audioOutputs();
        for(QAudioDevice device : devices) {
            if(device.id() == deviceId) {
                this->m_currentAudioDevice = device;
            }
        }
    }

    connect(m_mediaDevices, &QMediaDevices::audioOutputsChanged, this, &AudioStream::refreshAudioDevices);
}

void AudioStream::refreshAudioDevices() {
    this->m_audioDevices = QMediaDevices::audioOutputs();
    this->audioDevicesChanged(&this->m_audioDevices);
}

void AudioStream::setCurrentAudioDevice(QAudioDevice device) {
    this->m_currentAudioDevice = device;
    this->m_settings->setValue(SELECTED_DEVICE_SETTING, QVariant(device.id()));
}

QAudioDevice AudioStream::getCurrentAudioDevice() {
    return this->m_currentAudioDevice;
}

QList<QAudioDevice> AudioStream::getAudioDevices() {
    return this->m_audioDevices;
}
