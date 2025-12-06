#include "settingsdialog.h"

#include <QApplication>
#include <QIcon>
#include <QLocale>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "QVibra_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
            break;
        }
    }

    QSystemTrayIcon tray;
    QMenu menu;

    menu.addAction("Settings...", []{ ConfigDialog w; w.show(); });
    menu.addAction("Quit", &app, &QApplication::quit);

    tray.setContextMenu(&menu);
    tray.setIcon(QIcon(":/resources/app.ico"));
    tray.show();

    return app.exec();
}
