#include "AudioPlayer.h"
#include "AudioEngine.h"
#include <QAudioFormat>
#include <QMetaObject>
#include <cmath>
#include <algorithm>

class AudioPlayer::AudioPlayerIODevice : public QIODevice {
public:
    AudioPlayerIODevice(AudioPlayer *player, QObject *parent = nullptr)
        : QIODevice(parent), m_player(player) {}
        
    void startPlayback(qint64 startFrame, qint64 endFrame, bool loop) {
        m_startFrame = startFrame;
        m_endFrame = endFrame;
        m_loop = loop;
        m_currentFrame = startFrame;
        open(QIODevice::ReadOnly);
    }
    
    qint64 currentFrame() const { return m_currentFrame; }
    void setCurrentFrame(qint64 frame) { m_currentFrame = frame; }
    void setLooping(bool loop) { m_loop = loop; }

protected:
    qint64 readData(char *data, qint64 maxlen) override {
        if (!m_player || !m_player->m_engine || m_player->m_engine->totalFrames() <= 0) return 0;
        
        qint64 framesToRead = maxlen / (m_player->m_engine->channels() * sizeof(int16_t));
        if (framesToRead <= 0) return 0;
        
        qint64 framesRemaining = m_endFrame - m_currentFrame;
        if (framesRemaining <= 0) {
            if (m_loop) {
                m_currentFrame = m_startFrame;
                framesRemaining = m_endFrame - m_startFrame;
            } else {
                return 0; // EOF
            }
        }
        
        qint64 framesToActualRead = std::min(framesToRead, framesRemaining);
        qint64 readFrames = m_player->m_engine->pieceTable().readFrames(m_currentFrame, framesToActualRead, reinterpret_cast<int16_t*>(data));
        
        m_currentFrame += readFrames;
        
        return readFrames * m_player->m_engine->channels() * sizeof(int16_t);
    }
    
    qint64 writeData(const char *data, qint64 len) override { return 0; }
    
private:
    AudioPlayer *m_player;
    qint64 m_currentFrame = 0;
    qint64 m_startFrame = 0;
    qint64 m_endFrame = 0;
    bool m_loop = false;
};

AudioPlayer::AudioPlayer(AudioEngine *engine, QObject *parent)
    : QObject(parent), m_engine(engine)
{
    m_playerDevice = new AudioPlayerIODevice(this, this);
}

AudioPlayer::~AudioPlayer()
{
    if (m_audioOutput) {
        m_audioOutput->stop();
        delete m_audioOutput;
    }
}

void AudioPlayer::togglePlay()
{
    if (m_isPlaying) {
        if (m_audioOutput && m_audioOutput->state() == QAudio::ActiveState) {
            m_audioOutput->suspend();
            m_isPlaying = false;
            emit playingStateChanged();
        }
    } else {
        if (!m_audioOutput) {
            QAudioFormat format;
            format.setSampleRate(m_engine->sampleRate());
            format.setChannelCount(m_engine->channels());
            format.setSampleSize(16);
            format.setCodec("audio/pcm");
            format.setByteOrder(QAudioFormat::LittleEndian);
            format.setSampleType(QAudioFormat::SignedInt);

            m_audioOutput = new QAudioOutput(format, this);
            m_audioOutput->setNotifyInterval(30); // 30ms interval for smooth UI updates
            connect(m_audioOutput, &QAudioOutput::notify, this, [this]() {
                if (m_isPlaying && m_playerDevice) {
                    // Update play cursor based on actually processed time
                    qint64 frameOffset = m_audioOutput->processedUSecs() * m_engine->sampleRate() / 1000000;
                    
                    qint64 currentFrame = m_playStart + frameOffset;
                    if (m_isLooping && m_playEnd > m_playStart) {
                        currentFrame = m_playStart + (frameOffset % (m_playEnd - m_playStart));
                    }
                    m_playCursor = currentFrame;
                    emit playCursorChanged();
                }
            });
            connect(m_audioOutput, &QAudioOutput::stateChanged, this, [this](QAudio::State state) {
                if (state == QAudio::IdleState) {
                    if (!m_isLooping) {
                        stop();
                    }
                }
            });
        }

        if (m_audioOutput && m_audioOutput->state() == QAudio::SuspendedState 
            && m_playStart == (m_engine->selectionStart() >= 0 ? m_engine->selectionStart() : 0)
            && m_playEnd == (m_engine->selectionEnd() > m_engine->selectionStart() ? m_engine->selectionEnd() : m_engine->totalFrames())) {
            // Resume from pause
            m_audioOutput->resume();
        } else {
            // Start fresh
            m_playStart = (m_engine->selectionStart() >= 0) ? m_engine->selectionStart() : 0;
            m_playEnd = (m_engine->selectionEnd() > m_engine->selectionStart()) ? m_engine->selectionEnd() : m_engine->totalFrames();

            m_playCursor = m_playStart;
            if (m_playerDevice->isOpen()) {
                m_playerDevice->close();
            }
            m_playerDevice->startPlayback(m_playStart, m_playEnd, m_isLooping);
            m_audioOutput->start(m_playerDevice);
        }
        
        m_isPlaying = true;
        emit playingStateChanged();
        emit playCursorChanged();
    }
}

void AudioPlayer::stop()
{
    if (m_isPlaying || (m_audioOutput && m_audioOutput->state() == QAudio::SuspendedState)) {
        if (m_audioOutput) m_audioOutput->stop();
        if (m_playerDevice) m_playerDevice->close();
        m_isPlaying = false;
        m_playCursor = -1; // hide cursor
        emit playingStateChanged();
        emit playCursorChanged();
    }
}

void AudioPlayer::setPlayCursor(qint64 frame)
{
    if (m_isPlaying) {
        m_playerDevice->setCurrentFrame(frame);
    }
}

void AudioPlayer::setIsLooping(bool loop)
{
    if (m_isLooping != loop) {
        m_isLooping = loop;
        if (m_playerDevice) {
            m_playerDevice->setLooping(loop);
        }
        emit loopStateChanged();
    }
}
