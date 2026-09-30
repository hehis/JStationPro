#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "AudioEngine.h"
#include "WaveformItem.h"
#include "AudioPlayer.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    app.setApplicationName("JStationPro");
    app.setApplicationDisplayName("JStation Pro - Audio Editor");
    app.setOrganizationName("VoiceAI");

    qmlRegisterType<WaveformItem>("JStation", 1, 0, "WaveformItem");
    qmlRegisterType<AudioEngine>("JStation", 1, 0, "AudioEngine");
    qmlRegisterUncreatableType<AudioPlayer>("JStation", 1, 0, "AudioPlayer", "AudioPlayer is exposed via AudioEngine");

    QQmlApplicationEngine engine;

    if (argc > 1) {
        engine.rootContext()->setContextProperty("initialAudioFile", QString::fromLocal8Bit(argv[1]));
    } else {
        engine.rootContext()->setContextProperty("initialAudioFile", QString());
    }

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
