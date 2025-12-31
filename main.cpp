#include <QApplication>
#include <QIcon>
#include <QLocale>
#include <QMenu>
#include <QSettings>
#include <QTranslator>
#include <qcoreapplication.h>

#include "qvibra.h"

int main(int argc, char *argv[])
{
    // Create an Qt application...
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    QCoreApplication::setOrganizationName("Martin Hignett");
    QCoreApplication::setApplicationName("QVibra");

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

    qDebug() << "Creating QVibra...";

    QVibra qvibra(&app);

    qDebug() << "Executing app...";

    // Launch the app!
    return app.exec();
}
