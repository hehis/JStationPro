#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "AudioEngine.h"
#include "WaveformItem.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);
    app.setApplicationName("JStationPro");
    app.setApplicationDisplayName("JStation Pro - Audio Editor");
    app.setOrganizationName("VoiceAI");

    qmlRegisterType<WaveformItem>("JStation", 1, 0, "WaveformItem");
    qmlRegisterType<AudioEngine>("JStation", 1, 0, "AudioEngine");

    QQmlApplicationEngine engine;
    AudioEngine audioEngine;
    engine.rootContext()->setContextProperty("audioEngine", &audioEngine);

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    if (argc > 1) {
        audioEngine.openAudioFile(QString::fromLocal8Bit(argv[1]));
    }

    return app.exec();
}
