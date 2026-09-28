#include <QCoreApplication>
#include <iostream>
#include <QTimer>
#include "../src/AudioEngine.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    AudioEngine engine;

    QObject::connect(&engine, &AudioEngine::isLoadedChanged, [&]() {
        std::cout << "isLoadedChanged: " << (engine.isLoaded() ? "true" : "false") << std::endl;
        if (engine.isLoaded()) {
            std::cout << "Total frames: " << engine.totalFrames() << std::endl;
            std::cout << "Duration: " << engine.durationSeconds() << "s" << std::endl;
            PeakPoint pt = engine.pieceTable().queryLogicalRange(0, engine.totalFrames());
            std::cout << "Whole peak min: " << pt.minVal << " max: " << pt.maxVal << std::endl;
            PeakPoint ptHalf = engine.pieceTable().queryLogicalRange(0, 1000);
            std::cout << "First 1000 peak min: " << ptHalf.minVal << " max: " << ptHalf.maxVal << std::endl;
            QTimer::singleShot(100, &app, &QCoreApplication::quit);
        }
    });

    QObject::connect(&engine, &AudioEngine::decodeError, [&](const QString &err) {
        std::cout << "Decode error: " << err.toStdString() << std::endl;
        QTimer::singleShot(100, &app, &QCoreApplication::quit);
    });

    QString filePath = argc > 1 ? argv[1] : "sample_audio.wav";
    std::cout << "Opening audio file: " << filePath.toStdString() << std::endl;
    engine.openAudioFile(filePath);

    QTimer::singleShot(5000, [&]() {
        std::cout << "TIMEOUT! isLoaded: " << engine.isLoaded() << " status: " << engine.statusMessage().toStdString() << std::endl;
        app.quit();
    });

    return app.exec();
}
