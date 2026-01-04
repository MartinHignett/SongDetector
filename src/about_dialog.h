#pragma once

#include <QDialog>
#include <QObject>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class AboutDialog; }
QT_END_NAMESPACE

class AboutDialog : public QDialog {
    public:
        AboutDialog(QString pipeWireVersion);

    private:
        Ui::AboutDialog*    ui;
};
