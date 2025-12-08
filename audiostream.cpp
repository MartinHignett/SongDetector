#include <QApplication>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QObject>
#include <QVariant>
#include <qaudiodevice.h>

#include "audiostream.h"

AudioStream::AudioStream(QApplication *parent)
    : QObject(parent),
   m_currentAudioDevice(QMediaDevices::defaultAudioOutput()) {
    this->m_mediaDevices = new QMediaDevices(this);
    this->m_audioDevices = QMediaDevices::audioOutputs();
    connect(m_mediaDevices, &QMediaDevices::audioOutputsChanged, this, &AudioStream::refreshAudioDevices);
}

void AudioStream::refreshAudioDevices() {
    this->m_audioDevices = QMediaDevices::audioOutputs();
    this->audioDevicesChanged(&this->m_audioDevices);
}

void AudioStream::setCurrentAudioDevice(QAudioDevice device) {
    this->m_currentAudioDevice = device;
}

QAudioDevice AudioStream::getCurrentAudioDevice() {
    return this->m_currentAudioDevice;
}

QList<QAudioDevice> AudioStream::getAudioDevices() {
    return this->m_audioDevices;
}
