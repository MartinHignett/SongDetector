#include <QAudioDevice>
#include <QMediaDevices>
#include <QObject>
#include <qcombobox.h>
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
    connect(this->ui->audioDeviceCombo, &QComboBox::currentIndexChanged, this, &SettingsDialog::onDeviceChanged);
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
        if(device.id() == this->m_audioStream->getCurrentAudioDevice().id()) {
            // TODO: Select the current audio device in the combobox
        }
    }
}

void SettingsDialog::onDeviceChanged() {
    const auto device = this->ui->audioDeviceCombo->currentData().value<QAudioDevice>();
    this->m_audioStream->setCurrentAudioDevice(device);
}
