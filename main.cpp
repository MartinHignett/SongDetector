#include "settingsdialog.h"

#include <QApplication>
#include <QIcon>
#include <QLocale>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QTranslator>
#include <qcoreapplication.h>

int main(int argc, char *argv[])
{
    // Create an Qt application...
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    // ...with tranlsation support
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "QVibra_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
            break;
        }
    }

    // Setup system tray menu...
    QMenu menu;
    menu.addAction(QCoreApplication::translate("ContextMenu", "Settings..."), []{
        auto settings = new SettingsDialog();
        settings->show();
    });
    menu.addAction(QCoreApplication::translate("ContextMenu", "Quit"), &app, &QApplication::quit);

    // Create a system tray icon and show it
    QSystemTrayIcon tray;
    tray.setContextMenu(&menu);
    tray.setIcon(QIcon(":/resources/app.ico"));
    tray.show();

    // Launch the app!
    return app.exec();
}
