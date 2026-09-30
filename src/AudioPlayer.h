#pragma once

#include <QObject>
#include <QAudioOutput>
#include <QIODevice>

class AudioEngine;

class AudioPlayer : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY playingStateChanged)
    Q_PROPERTY(bool isLooping READ isLooping WRITE setIsLooping NOTIFY loopStateChanged)
    Q_PROPERTY(qint64 playCursor READ playCursor NOTIFY playCursorChanged)

public:
    explicit AudioPlayer(AudioEngine *engine, QObject *parent = nullptr);
    ~AudioPlayer() override;

    bool isPlaying() const { return m_isPlaying; }
    bool isLooping() const { return m_isLooping; }
    void setIsLooping(bool loop);
    qint64 playCursor() const { return m_playCursor; }

    Q_INVOKABLE void togglePlay();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void setPlayCursor(qint64 frame);

signals:
    void playingStateChanged();
    void loopStateChanged();
    void playCursorChanged();

private:
    AudioEngine *m_engine;
    bool m_isPlaying = false;
    bool m_isLooping = false;
    qint64 m_playCursor = -1;
    qint64 m_playStart = 0;
    qint64 m_playEnd = 0;

    class AudioPlayerIODevice;
    AudioPlayerIODevice *m_playerDevice = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    
    friend class AudioPlayerIODevice;
};
