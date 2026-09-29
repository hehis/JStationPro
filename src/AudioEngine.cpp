#include "AudioEngine.h"
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QDebug>
#include <QTimer>
#include <QtConcurrent>
#include <cmath>
#include <algorithm>

AudioEngine::AudioEngine(QObject *parent)
    : QObject(parent)
    , m_isLoaded(false)
    , m_isDecoding(false)
    , m_decodeProgress(0.0)
    , m_statusMessage("Ready")
    , m_viewStartFrame(0)
    , m_viewEndFrame(0)
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
    QString localPath = filePathOrUrl.trimmed();
    // Strip surrounding quotes
    if (localPath.startsWith("\"") && localPath.endsWith("\"") && localPath.length() >= 2) {
        localPath = localPath.mid(1, localPath.length() - 2);
    }

    QUrl url(localPath);
    if (url.isValid() && url.scheme().startsWith("file", Qt::CaseInsensitive)) {
        localPath = url.toLocalFile();
    } else if (localPath.startsWith("file:///", Qt::CaseInsensitive)) {
        localPath = localPath.mid(8);
    } else if (localPath.startsWith("file://", Qt::CaseInsensitive)) {
        localPath = localPath.mid(7);
    } else if (localPath.startsWith("file:", Qt::CaseInsensitive)) {
        localPath = localPath.mid(5);
    }

    localPath = QDir::toNativeSeparators(localPath);
    // Remove leading slash for Windows drive letters (e.g. \C:\ -> C:\)
    if (localPath.length() >= 3 && (localPath[0] == '\\' || localPath[0] == '/') && localPath[2] == ':') {
        localPath = localPath.mid(1);
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

    QString nativeAbsPath = QDir::toNativeSeparators(fi.absoluteFilePath());

    // Fast path: Try uncompressed WAV parser
    if (fi.suffix().compare("wav", Qt::CaseInsensitive) == 0) {
        if (tryFastWavDecode(nativeAbsPath)) {
            return;
        }
    }

    // Standard path: QAudioDecoder for MP3, AAC, FLAC, OGG, WAV, etc.
    startDecoding(nativeAbsPath);
}

void AudioEngine::importAudioDialog()
{
    QString filter = tr("All supported audio (*.wav *.mp3 *.flac *.aac *.ogg *.m4a *.wma);;Wave files (*.wav);;MP3 files (*.mp3);;All files (*.*)");
    QString path = QFileDialog::getOpenFileName(nullptr, tr("Please choose an audio file"), QString(), filter);
    if (!path.isEmpty()) {
        openAudioFile(path);
    }
}

bool AudioEngine::tryFastWavDecode(const QString &localFilePath)
{
    QFile file(localFilePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    // Minimum RIFF header: 12 bytes
    QByteArray header = file.read(12);
    if (header.size() < 12)
        return false;

    if (memcmp(header.constData(), "RIFF", 4) != 0 ||
        memcmp(header.constData() + 8, "WAVE", 4) != 0)
    {
        return false;
    }

    quint16 audioFormat = 0;
    quint16 numChannels = 0;
    quint32 sampleRate  = 0;
    quint16 bitsPerSample = 0;

    qint64 dataOffset = 0;
    quint32 dataBytes = 0;
    bool foundFmt = false;
    bool foundData = false;

    qint64 fileSize = file.size();

    // Scan chunks
    while (file.pos() + 8 <= fileSize) {
        char chunkId[4];
        if (file.read(chunkId, 4) != 4) break;
        quint32 chunkSize = 0;
        if (file.read(reinterpret_cast<char*>(&chunkSize), 4) != 4) break;

        qint64 chunkDataPos = file.pos();

        if (memcmp(chunkId, "fmt ", 4) == 0 && chunkSize >= 16) {
            QByteArray fmtData = file.read(std::min<quint32>(chunkSize, 40));
            if (fmtData.size() >= 16) {
                const char *d = fmtData.constData();
                audioFormat   = *reinterpret_cast<const quint16*>(d);
                numChannels   = *reinterpret_cast<const quint16*>(d + 2);
                sampleRate    = *reinterpret_cast<const quint32*>(d + 4);
                bitsPerSample = *reinterpret_cast<const quint16*>(d + 14);

                // Handle WAVE_FORMAT_EXTENSIBLE (0xFFFE)
                if (audioFormat == 0xFFFE && fmtData.size() >= 40) {
                    quint16 subFormat = *reinterpret_cast<const quint16*>(d + 24);
                    audioFormat = subFormat; // 1 for PCM, 3 for IEEE Float
                }
                foundFmt = true;
            }
        } else if (memcmp(chunkId, "data", 4) == 0) {
            foundData = true;
            dataBytes = chunkSize;
            dataOffset = chunkDataPos;
        }

        // Align to 2 bytes as per RIFF standard
        qint64 nextPos = chunkDataPos + chunkSize + (chunkSize & 1);
        if (!file.seek(nextPos)) break;
    }

    if (!foundFmt || !foundData || numChannels == 0 || sampleRate == 0 || dataBytes == 0) {
        return false;
    }

    // Supported formats: PCM (1) or IEEE Float (3)
    if (audioFormat != 1 && audioFormat != 3) {
        return false;
    }

    // Direct background read
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

            if (audioFormat == 1 && bitsPerSample == 16) {
                int sampleCount = chunk.size() / 2;
                const int16_t *src = reinterpret_cast<const int16_t*>(chunk.constData());
                samples.reserve(samples.size() + sampleCount);
                for (int i = 0; i < sampleCount; ++i) {
                    samples.push_back(src[i]);
                }
            } else if (audioFormat == 1 && bitsPerSample == 24) {
                int sampleCount = chunk.size() / 3;
                const quint8 *src = reinterpret_cast<const quint8*>(chunk.constData());
                samples.reserve(samples.size() + sampleCount);
                for (int i = 0; i < sampleCount; ++i) {
                    qint32 val = (src[i * 3 + 0]) | (src[i * 3 + 1] << 8) | (src[i * 3 + 2] << 16);
                    if (val & 0x800000) val |= 0xFF000000;
                    samples.push_back(static_cast<int16_t>(val >> 8));
                }
            } else if (audioFormat == 1 && bitsPerSample == 32) {
                int sampleCount = chunk.size() / 4;
                const qint32 *src = reinterpret_cast<const qint32*>(chunk.constData());
                samples.reserve(samples.size() + sampleCount);
                for (int i = 0; i < sampleCount; ++i) {
                    samples.push_back(static_cast<int16_t>(src[i] >> 16));
                }
            } else if (audioFormat == 3 && bitsPerSample == 32) {
                int sampleCount = chunk.size() / 4;
                const float *src = reinterpret_cast<const float*>(chunk.constData());
                samples.reserve(samples.size() + sampleCount);
                for (int i = 0; i < sampleCount; ++i) {
                    float s = std::max(-1.0f, std::min(1.0f, src[i]));
                    samples.push_back(static_cast<int16_t>(s * 32767.0f));
                }
            } else if (audioFormat == 1 && bitsPerSample == 8) {
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
    } else if (fmt.sampleType() == QAudioFormat::SignedInt && fmt.sampleSize() == 24) {
        const quint8 *src = reinterpret_cast<const quint8*>(buffer.constData());
        for (int i = 0; i < totalSamples; ++i) {
            qint32 val = (src[i * 3 + 0]) | (src[i * 3 + 1] << 8) | (src[i * 3 + 2] << 16);
            if (val & 0x800000) val |= 0xFF000000;
            dest[i] = static_cast<int16_t>(val >> 8);
        }
    } else if (fmt.sampleType() == QAudioFormat::UnSignedInt && fmt.sampleSize() == 8) {
        const quint8 *src = buffer.constData<quint8>();
        for (int i = 0; i < totalSamples; ++i) {
            int val = static_cast<int>(src[i]) - 128;
            dest[i] = static_cast<int16_t>(val * 256);
        }
    } else {
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

    qint64 frameCount = decodedSamples.size() / channels;
    m_pieceTable.setOriginalData(decodedSamples.constData(), frameCount, sampleRate, channels);

    m_isDecoding = false;
    m_isLoaded = true;
    m_decodeProgress = 1.0;

    m_viewStartFrame = 0;
    m_viewEndFrame = frameCount;

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
    emit viewRangeChanged();
    emit waveformChanged();
    emit canPasteChanged();
}

void AudioEngine::clampViewRange()
{
    qint64 total = totalFrames();
    if (total <= 0) {
        m_viewStartFrame = 0;
        m_viewEndFrame = 0;
        emit viewRangeChanged();
        return;
    }

    if (m_viewStartFrame < 0) m_viewStartFrame = 0;
    if (m_viewEndFrame > total || m_viewEndFrame <= 0) m_viewEndFrame = total;
    if (m_viewEndFrame <= m_viewStartFrame) {
        m_viewEndFrame = std::min<qint64>(total, m_viewStartFrame + std::max<qint64>(100, total / 20));
    }
    emit viewRangeChanged();
}

void AudioEngine::setViewStartFrame(qint64 frame)
{
    qint64 total = totalFrames();
    if (total <= 0) return;
    frame = std::max<qint64>(0, std::min<qint64>(frame, m_viewEndFrame - 10));
    if (m_viewStartFrame != frame) {
        m_viewStartFrame = frame;
        emit viewRangeChanged();
    }
}

void AudioEngine::setViewEndFrame(qint64 frame)
{
    qint64 total = totalFrames();
    if (total <= 0) return;
    frame = std::min<qint64>(total, std::max<qint64>(frame, m_viewStartFrame + 10));
    if (m_viewEndFrame != frame) {
        m_viewEndFrame = frame;
        emit viewRangeChanged();
    }
}

void AudioEngine::setViewRange(qint64 startFrame, qint64 endFrame)
{
    qint64 total = totalFrames();
    if (total <= 0) return;
    startFrame = std::max<qint64>(0, startFrame);
    endFrame   = std::min<qint64>(total, endFrame);
    if (endFrame <= startFrame) endFrame = std::min<qint64>(total, startFrame + 10);

    if (m_viewStartFrame != startFrame || m_viewEndFrame != endFrame) {
        m_viewStartFrame = startFrame;
        m_viewEndFrame = endFrame;
        emit viewRangeChanged();
    }
}

void AudioEngine::zoomAt(qreal centerRatio, qreal factor)
{
    qint64 total = totalFrames();
    if (total <= 0) return;

    centerRatio = std::max<qreal>(0.0, std::min<qreal>(1.0, centerRatio));
    qint64 curLen = m_viewEndFrame - m_viewStartFrame;
    if (curLen <= 0) curLen = total;

    qint64 newLen = static_cast<qint64>(curLen * factor);
    qint64 minLen = std::min<qint64>(total, 10); // Allow deep zoom to 10 samples
    if (newLen < minLen) newLen = minLen;
    if (newLen > total) newLen = total;

    qint64 centerFrame = m_viewStartFrame + static_cast<qint64>(curLen * centerRatio);
    qint64 newStart = centerFrame - static_cast<qint64>(newLen * centerRatio);
    qint64 newEnd = newStart + newLen;

    if (newStart < 0) {
        newEnd -= newStart;
        newStart = 0;
    }
    if (newEnd > total) {
        newStart -= (newEnd - total);
        newEnd = total;
    }
    if (newStart < 0) newStart = 0;

    setViewRange(newStart, newEnd);
}

void AudioEngine::resetView()
{
    setViewRange(0, totalFrames());
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
        clampViewRange();
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
        insertFrame = m_selectionStart >= 0 ? m_selectionStart : m_viewStartFrame;
    }
    if (m_pieceTable.pasteAt(insertFrame)) {
        m_statusMessage = QString("Pasted at frame %1").arg(insertFrame);
        clearSelection();
        clampViewRange();
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
        clampViewRange();
        emit totalFramesChanged();
        emit waveformChanged();
        emit statusMessageChanged();
    }
}
