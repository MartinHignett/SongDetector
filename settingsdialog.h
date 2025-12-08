#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QAudioDevice>
#include <QDialog>
#include <QMediaDevices>

#include "audiostream.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsDialog; }
QT_END_NAMESPACE

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(AudioStream *audioStream, QWidget *parent = nullptr);
    ~SettingsDialog();

private:
    Ui::SettingsDialog  *ui;
    AudioStream *m_audioStream;

    void updateAudioDevices();

// private slots:
//     void getAudioDevices();
};

#endif // SETTINGSDIALOG_H
