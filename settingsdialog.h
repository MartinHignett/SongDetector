#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QAudioDevice>
#include <QDialog>
#include <QMediaDevices>
#include <QSettings>

#include "audiostream.h"
#include "song_detector.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsDialog; }
QT_END_NAMESPACE

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(AudioStream* audioStream, QWidget *parent = nullptr);
    ~SettingsDialog();

private:
    Ui::SettingsDialog* ui;
    AudioStream*        m_audioStream;
    QSettings*          m_settings;

    void updateAudioDevices();
    void onDeviceChanged();

signals:
    void forceDarkModeChanged();

private slots:
     void startFingerprint();
     void setForceDarkMode(bool checked);
};

#endif // SETTINGSDIALOG_H
