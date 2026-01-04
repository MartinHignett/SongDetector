#include <QtVersion>
#include <qtversionchecks.h>

#include "about_dialog.h"
#include "ui_about_dialog.h"

AboutDialog::AboutDialog(QString pipeWireVersion) :
    QDialog(nullptr),
    ui(new Ui::AboutDialog) {
        ui->setupUi(this);
        ui->pipeWireVersionText->setText(pipeWireVersion);
        ui->qtVersionText->setText(QT_VERSION_STR);
}
