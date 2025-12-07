#include <QAudioDevice>
#include <QMediaDevices>
#include <QObject>
#include <qmediadevices.h>

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog
    )
{
    this->m_mediaDevices = new QMediaDevices(this);
    ui->setupUi(this);
    this->setAudioDevices();
    connect(m_mediaDevices, &QMediaDevices::audioOutputsChanged, this, &SettingsDialog::setAudioDevices);
}

SettingsDialog::~SettingsDialog()
{
    delete m_mediaDevices;
    delete ui;
}

void SettingsDialog::setAudioDevices(){
    ui->audioDeviceCombo->clear();
    const auto devices = QMediaDevices::audioOutputs();
    for (const QAudioDevice &device : devices) {
        ui->audioDeviceCombo->addItem(device.description(), QVariant::fromValue(device));
    }
}
