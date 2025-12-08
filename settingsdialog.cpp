#include <QAudioDevice>
#include <QMediaDevices>
#include <QObject>
#include <qmediadevices.h>

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(AudioStream *audioStream, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    this->m_audioStream = audioStream;
    ui->setupUi(this);
    this->updateAudioDevices();
    connect(m_audioStream, &AudioStream::audioDevicesChanged, this, &SettingsDialog::updateAudioDevices);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::updateAudioDevices(){
    ui->audioDeviceCombo->clear();
    const auto devices = this->m_audioStream->getAudioDevices();
    for(const QAudioDevice &device : devices) {
        ui->audioDeviceCombo->addItem(device.description(), QVariant::fromValue(device));
    }
}
