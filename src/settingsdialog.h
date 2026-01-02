#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QAudioDevice>
#include <QDialog>
#include <QMediaDevices>
#include <QSettings>
#include <qmediadevices.h>

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsDialog; }
QT_END_NAMESPACE

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

private:
    Ui::SettingsDialog* ui;
    QSettings*          m_settings;
    QMediaDevices*      m_mediaDevices;

    void onDeviceChanged();

signals:
    void forceDarkModeChanged();
    void currentDeviceChanged(const QString& deviceId);

private slots:
     void updateAudioDevices();
     void setForceDarkMode(bool checked);
};

#endif // SETTINGSDIALOG_H
