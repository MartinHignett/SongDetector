#include <QAudioDevice>
#include <QCheckBox>
#include <QComboBox>
#include <QMediaDevices>
#include <QObject>

#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "settings.h"

SettingsDialog::SettingsDialog(QSettings *settings)
    : QDialog(nullptr)
    , m_settings(settings)
    , ui(new Ui::SettingsDialog) {
        m_mediaDevices = new QMediaDevices(this);
        ui->setupUi(this);

        if (m_settings->contains(DARK_TRAY_ICON_SETTING) && m_settings->value(DARK_TRAY_ICON_SETTING) == true) {
            ui->darkModeIcon->setCheckState(Qt::CheckState::Checked);
        } else {
            ui->darkModeIcon->setCheckState(Qt::CheckState::Unchecked);
        }

        updateAudioDevices();
        connect(m_mediaDevices, &QMediaDevices::audioOutputsChanged, this, &SettingsDialog::updateAudioDevices);
        connect(ui->audioDeviceCombo, &QComboBox::currentIndexChanged, this, &SettingsDialog::onDeviceChanged);
        connect(ui->darkModeIcon, &QCheckBox::clicked, this, &SettingsDialog::setForceDarkMode);
        connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &SettingsDialog::close);
        setMaximumSize(size());
        setMinimumSize(size());
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::updateAudioDevices(){
    ui->audioDeviceCombo->clear();
    const auto devices = QMediaDevices::audioOutputs();

    for(const QAudioDevice &device : devices) {
        ui->audioDeviceCombo->addItem(device.description(), QVariant::fromValue(device));
    }

    if (m_settings->contains(SELECTED_DEVICE_SETTING)) {
        const auto currentDeviceId = m_settings->value(SELECTED_DEVICE_SETTING);
        int index = ui->audioDeviceCombo->findData(currentDeviceId);

        if (index >=0) {
            ui->audioDeviceCombo->setCurrentIndex(index);
        }
    }
}

void SettingsDialog::onDeviceChanged() {
    const auto device = ui->audioDeviceCombo->currentData().value<QAudioDevice>();
    m_settings->setValue(SELECTED_DEVICE_SETTING, device.id());
    currentDeviceChanged(device.id());
}

/********************************
 * Slots
 ********************************/

void SettingsDialog::setForceDarkMode(bool checked) {
    m_settings->setValue(DARK_TRAY_ICON_SETTING, QVariant(checked));
    forceDarkModeChanged();
}
