#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QLocale>
#include <QLockFile>
#include <QMenu>
#include <QSettings>
#include <QTranslator>
#include <qcoreapplication.h>

#include "song_detector.h"

#define APPLICATION_NAME QStringLiteral("SongDetector")

int main(int argc, char *argv[])
{
    // Create an Qt application...
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    QCoreApplication::setOrganizationName(APPLICATION_NAME);
    QCoreApplication::setApplicationName(APPLICATION_NAME);

    // ...but not if its already running...
    QString lockFilePath = QDir::temp().absoluteFilePath(APPLICATION_NAME + ".lock");
    QLockFile lockFile(lockFilePath);

    // Try to lock the file for 100ms
    if (!lockFile.tryLock(100)) {
        qDebug() << "SongDetector is already running";
        return 1;
    }

    // ...with tranlsation support
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = APPLICATION_NAME + "_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
            break;
        }
    }

    SongDetector songDetector(&app);

    // Launch the app!
    return app.exec();
}
