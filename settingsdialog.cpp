#include <QAudioDevice>
#include <QMediaDevices>
#include <QObject>
#include <memory>
#include <qmediadevices.h>

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog
    )
{;
    this->m_mediaDevices = std::make_shared<QMediaDevices>(this);
    ui->setupUi(this);
    this->setAudioDevices();
    connect(m_mediaDevices.get(), &QMediaDevices::audioOutputsChanged, this, &SettingsDialog::setAudioDevices);
}

SettingsDialog::~SettingsDialog()
{
    disconnect(m_mediaDevices.get(), &QMediaDevices::audioInputsChanged, this, &SettingsDialog::setAudioDevices);
    delete ui;
}

void SettingsDialog::setAudioDevices(){
    ui->audioDeviceCombo->clear();
    const auto devices = QMediaDevices::audioOutputs();
    for (const QAudioDevice &device : devices) {
        ui->audioDeviceCombo->addItem(device.description(), QVariant::fromValue(device));
    }
}
