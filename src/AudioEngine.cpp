#include "AudioEngine.h"
#include <QFileInfo>
#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QTimer>
#include <QtConcurrent>
#include <cmath>

AudioEngine::AudioEngine(QObject *parent)
    : QObject(parent)
    , m_isLoaded(false)
    , m_isDecoding(false)
    , m_decodeProgress(0.0)
    , m_statusMessage("Ready")
    , m_selectionStart(-1)
    , m_selectionEnd(-1)
    , m_detectedSampleRate(44100)
    , m_detectedChannels(2)
    , m_expectedDurationMs(0)
{
}

AudioEngine::~AudioEngine()
{
    if (m_decoder) {
        m_decoder->stop();
    }
}

qint64 AudioEngine::totalFrames() const
{
    return m_pieceTable.totalFrames();
}

double AudioEngine::durationSeconds() const
{
    int sr = m_pieceTable.sampleRate();
    if (sr <= 0) return 0.0;
    return static_cast<double>(totalFrames()) / sr;
}

double AudioEngine::frameToSeconds(qint64 frame) const
{
    int sr = m_pieceTable.sampleRate();
    if (sr <= 0) return 0.0;
    return static_cast<double>(frame) / sr;
}

qint64 AudioEngine::secondsToFrame(double sec) const
{
    int sr = m_pieceTable.sampleRate();
    if (sr <= 0) return 0;
    return static_cast<qint64>(sec * sr);
}

void AudioEngine::openAudioFile(const QString &filePathOrUrl)
{
    QString localPath = filePathOrUrl;
    if (localPath.startsWith("file:///")) {
        localPath = QUrl(filePathOrUrl).toLocalFile();
    }

    QFileInfo fi(localPath);
    if (!fi.exists() || !fi.isFile()) {
        m_statusMessage = "File not found: " + localPath;
        emit statusMessageChanged();
        emit decodeError(m_statusMessage);
        return;
    }

    m_isDecoding = true;
    m_isLoaded = false;
    m_decodeProgress = 0.0;
    m_statusMessage = "Opening " + fi.fileName() + "...";
    clearSelection();
    emit isDecodingChanged();
    emit isLoadedChanged();
    emit decodeProgressChanged();
    emit statusMessageChanged();

    // Fast path: if standard uncompressed WAV file, decode directly
    if (fi.suffix().compare("wav", Qt::CaseInsensitive) == 0) {
        if (tryFastWavDecode(localPath)) {
            return;
        }
    }

    // Standard path: QAudioDecoder for MP3, AAC, FLAC, OGG, WAV, etc.
    startDecoding(localPath);
}

bool AudioEngine::tryFastWavDecode(const QString &localFilePath)
{
    QFile file(localFilePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QByteArray header = file.read(44);
    if (header.size() < 44)
        return false;

    if (memcmp(header.constData(), "RIFF", 4) != 0 ||
        memcmp(header.constData() + 8, "WAVE", 4) != 0 ||
        memcmp(header.constData() + 12, "fmt ", 4) != 0)
    {
        return false;
    }

    quint16 audioFormat = *reinterpret_cast<const quint16*>(header.constData() + 20);
    quint16 numChannels = *reinterpret_cast<const quint16*>(header.constData() + 22);
    quint32 sampleRate  = *reinterpret_cast<const quint32*>(header.constData() + 24);
    quint16 bitsPerSample = *reinterpret_cast<const quint16*>(header.constData() + 34);

    // Find 'data' chunk
    qint64 dataOffset = 12;
    qint64 fileSize = file.size();
    bool foundData = false;
    quint32 dataBytes = 0;

    file.seek(12);
    while (file.pos() + 8 <= fileSize) {
        char chunkId[4];
        if (file.read(chunkId, 4) != 4) break;
        quint32 chunkSize = 0;
        if (file.read(reinterpret_cast<char*>(&chunkSize), 4) != 4) break;

        if (memcmp(chunkId, "data", 4) == 0) {
            foundData = true;
            dataBytes = chunkSize;
            dataOffset = file.pos();
            break;
        } else {
            file.seek(file.pos() + chunkSize);
        }
    }

    if (!foundData || numChannels == 0 || sampleRate == 0)
        return false;

    // Direct asynchronous read in background
    (void)QtConcurrent::run([this, localFilePath, dataOffset, dataBytes, audioFormat, numChannels, sampleRate, bitsPerSample]() {
        QFile f(localFilePath);
        if (!f.open(QIODevice::ReadOnly)) {
            QMetaObject::invokeMethod(this, [this]() {
                m_isDecoding = false;
                m_statusMessage = "Failed to read WAV file";
                emit isDecodingChanged();
                emit statusMessageChanged();
            });
            return;
        }

        f.seek(dataOffset);
        qint64 bytesLeft = dataBytes;
        qint64 totalBytes = dataBytes;
        QVector<int16_t> samples;
        const qint64 chunkSize = 65536; // 64KB per chunk

        while (bytesLeft > 0 && f.bytesAvailable() > 0) {
            qint64 toRead = std::min(bytesLeft, chunkSize);
            QByteArray chunk = f.read(toRead);
            if (chunk.isEmpty()) break;
            bytesLeft -= chunk.size();

            // Convert samples to 16-bit signed
            if (audioFormat == 1 && bitsPerSample == 16) {
                int sampleCount = chunk.size() / 2;
                const int16_t *src = reinterpret_cast<const int16_t*>(chunk.constData());
                samples.reserve(samples.size() + sampleCount);
                for (int i = 0; i < sampleCount; ++i) {
                    samples.push_back(src[i]);
                }
            } else if (audioFormat == 3 && bitsPerSample == 32) { // 32-bit float
                int sampleCount = chunk.size() / 4;
                const float *src = reinterpret_cast<const float*>(chunk.constData());
                samples.reserve(samples.size() + sampleCount);
                for (int i = 0; i < sampleCount; ++i) {
                    float s = std::max(-1.0f, std::min(1.0f, src[i]));
                    samples.push_back(static_cast<int16_t>(s * 32767.0f));
                }
            } else if (audioFormat == 1 && bitsPerSample == 8) { // 8-bit unsigned
                int sampleCount = chunk.size();
                const quint8 *src = reinterpret_cast<const quint8*>(chunk.constData());
                samples.reserve(samples.size() + sampleCount);
                for (int i = 0; i < sampleCount; ++i) {
                    int val = static_cast<int>(src[i]) - 128;
                    samples.push_back(static_cast<int16_t>(val * 256));
                }
            }

            qreal progress = 1.0 - (static_cast<double>(bytesLeft) / totalBytes);
            QMetaObject::invokeMethod(this, [this, progress]() {
                m_decodeProgress = progress;
                emit decodeProgressChanged();
            });
        }

        QMetaObject::invokeMethod(this, [this, s = std::move(samples), sampleRate, numChannels]() mutable {
            finishLoading(std::move(s), sampleRate, numChannels);
        });
    });

    return true;
}

void AudioEngine::startDecoding(const QString &localFilePath)
{
    m_stagingBuffer.clear();
    m_expectedDurationMs = 0;
    m_detectedSampleRate = 44100;
    m_detectedChannels = 2;

    if (!m_decoder) {
        m_decoder = std::make_unique<QAudioDecoder>();
        connect(m_decoder.get(), &QAudioDecoder::bufferReady, this, &AudioEngine::onBufferReady);
        connect(m_decoder.get(), &QAudioDecoder::finished, this, &AudioEngine::onDecoderFinished);
        connect(m_decoder.get(), QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error), this, &AudioEngine::onDecoderError);
        connect(m_decoder.get(), &QAudioDecoder::positionChanged, this, &AudioEngine::onDecoderPositionChanged);
        connect(m_decoder.get(), &QAudioDecoder::durationChanged, this, &AudioEngine::onDecoderDurationChanged);
    } else {
        m_decoder->stop();
    }

    m_decoder->setSourceFilename(localFilePath);
    m_decoder->start();
}

void AudioEngine::onBufferReady()
{
    if (!m_decoder) return;

    QAudioBuffer buffer = m_decoder->read();
    if (!buffer.isValid()) return;

    const QAudioFormat &fmt = buffer.format();
    m_detectedSampleRate = fmt.sampleRate();
    m_detectedChannels = fmt.channelCount();

    int frameCount = buffer.frameCount();
    if (frameCount <= 0 || m_detectedChannels <= 0) return;

    int totalSamples = frameCount * m_detectedChannels;
    int oldSize = m_stagingBuffer.size();
    m_stagingBuffer.resize(oldSize + totalSamples);
    int16_t *dest = m_stagingBuffer.data() + oldSize;

    if (fmt.sampleType() == QAudioFormat::SignedInt && fmt.sampleSize() == 16) {
        const int16_t *src = buffer.constData<int16_t>();
        std::copy(src, src + totalSamples, dest);
    } else if (fmt.sampleType() == QAudioFormat::Float) {
        const float *src = buffer.constData<float>();
        for (int i = 0; i < totalSamples; ++i) {
            float s = std::max(-1.0f, std::min(1.0f, src[i]));
            dest[i] = static_cast<int16_t>(s * 32767.0f);
        }
    } else if (fmt.sampleType() == QAudioFormat::SignedInt && fmt.sampleSize() == 32) {
        const qint32 *src = buffer.constData<qint32>();
        for (int i = 0; i < totalSamples; ++i) {
            dest[i] = static_cast<int16_t>(src[i] >> 16);
        }
    } else if (fmt.sampleType() == QAudioFormat::UnSignedInt && fmt.sampleSize() == 8) {
        const quint8 *src = buffer.constData<quint8>();
        for (int i = 0; i < totalSamples; ++i) {
            int val = static_cast<int>(src[i]) - 128;
            dest[i] = static_cast<int16_t>(val * 256);
        }
    } else {
        // Fallback: 16-bit copy
        const int16_t *src = reinterpret_cast<const int16_t*>(buffer.constData());
        std::copy(src, src + totalSamples, dest);
    }
}

void AudioEngine::onDecoderPositionChanged(qint64 positionMs)
{
    if (m_expectedDurationMs > 0) {
        m_decodeProgress = std::min(1.0, static_cast<double>(positionMs) / m_expectedDurationMs);
        emit decodeProgressChanged();
    }
}

void AudioEngine::onDecoderDurationChanged(qint64 durationMs)
{
    m_expectedDurationMs = durationMs;
}

void AudioEngine::onDecoderFinished()
{
    m_decodeProgress = 1.0;
    emit decodeProgressChanged();
    finishLoading(std::move(m_stagingBuffer), m_detectedSampleRate, m_detectedChannels);
}

void AudioEngine::onDecoderError(QAudioDecoder::Error error)
{
    m_isDecoding = false;
    m_statusMessage = QString("Decode Error: %1").arg(error);
    emit isDecodingChanged();
    emit statusMessageChanged();
    emit decodeError(m_statusMessage);
}

void AudioEngine::finishLoading(QVector<int16_t> &&decodedSamples, int sampleRate, int channels)
{
    m_statusMessage = "Building waveform pyramid...";
    emit statusMessageChanged();

    // Load into PieceTable
    qint64 frameCount = decodedSamples.size() / channels;
    m_pieceTable.setOriginalData(decodedSamples.constData(), frameCount, sampleRate, channels);

    m_isDecoding = false;
    m_isLoaded = true;
    m_decodeProgress = 1.0;
    m_statusMessage = QString("Loaded: %1 frames (%2 s, %3 Hz, %4 ch)")
                        .arg(frameCount)
                        .arg(durationSeconds(), 0, 'f', 2)
                        .arg(sampleRate)
                        .arg(channels);

    emit isDecodingChanged();
    emit isLoadedChanged();
    emit decodeProgressChanged();
    emit statusMessageChanged();
    emit totalFramesChanged();
    emit waveformChanged();
    emit canPasteChanged();
}

void AudioEngine::setSelection(qint64 startFrame, qint64 endFrame)
{
    if (startFrame > endFrame) {
        std::swap(startFrame, endFrame);
    }
    qint64 total = totalFrames();
    startFrame = std::max<qint64>(0, std::min<qint64>(startFrame, total));
    endFrame = std::max<qint64>(0, std::min<qint64>(endFrame, total));

    if (m_selectionStart != startFrame || m_selectionEnd != endFrame) {
        m_selectionStart = startFrame;
        m_selectionEnd = endFrame;
        emit selectionChanged();
    }
}

void AudioEngine::clearSelection()
{
    if (m_selectionStart != -1 || m_selectionEnd != -1) {
        m_selectionStart = -1;
        m_selectionEnd = -1;
        emit selectionChanged();
    }
}

void AudioEngine::cutSelection()
{
    if (!hasSelection()) return;
    qint64 len = m_selectionEnd - m_selectionStart;
    if (m_pieceTable.cutRange(m_selectionStart, len)) {
        m_statusMessage = QString("Cut %1 frames").arg(len);
        clearSelection();
        emit totalFramesChanged();
        emit waveformChanged();
        emit canPasteChanged();
        emit statusMessageChanged();
    }
}

void AudioEngine::copySelection()
{
    if (!hasSelection()) return;
    qint64 len = m_selectionEnd - m_selectionStart;
    if (m_pieceTable.copyRange(m_selectionStart, len)) {
        m_statusMessage = QString("Copied %1 frames").arg(len);
        emit canPasteChanged();
        emit statusMessageChanged();
    }
}

void AudioEngine::pasteAt(qint64 insertFrame)
{
    if (!canPaste()) return;
    if (insertFrame < 0) {
        insertFrame = m_selectionStart >= 0 ? m_selectionStart : 0;
    }
    if (m_pieceTable.pasteAt(insertFrame)) {
        m_statusMessage = QString("Pasted at frame %1").arg(insertFrame);
        clearSelection();
        emit totalFramesChanged();
        emit waveformChanged();
        emit statusMessageChanged();
    }
}

void AudioEngine::deleteSelection()
{
    if (!hasSelection()) return;
    qint64 len = m_selectionEnd - m_selectionStart;
    if (m_pieceTable.deleteRange(m_selectionStart, len)) {
        m_statusMessage = QString("Deleted %1 frames").arg(len);
        clearSelection();
        emit totalFramesChanged();
        emit waveformChanged();
        emit statusMessageChanged();
    }
}
