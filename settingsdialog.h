#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class ConfigDialog; }
QT_END_NAMESPACE

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    ConfigDialog(QWidget *parent = nullptr);
    ~ConfigDialog();

private:
    Ui::ConfigDialog *ui;
};
#endif // CONFIGDIALOG_H
