#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QAudioDevice>
#include <QDialog>
#include <QMediaDevices>

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
    Ui::SettingsDialog *ui;
    std::shared_ptr<QMediaDevices> m_mediaDevices;

    void getAudioDevices();

private slots:
    void setAudioDevices();
};

#endif // SETTINGSDIALOG_H
