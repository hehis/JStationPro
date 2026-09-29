#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QAudioDecoder>
#include <QAudioBuffer>
#include <memory>
#include "PieceTable.h"

class AudioEngine : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isLoaded READ isLoaded NOTIFY isLoadedChanged)
    Q_PROPERTY(bool isDecoding READ isDecoding NOTIFY isDecodingChanged)
    Q_PROPERTY(qreal decodeProgress READ decodeProgress NOTIFY decodeProgressChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(qint64 totalFrames READ totalFrames NOTIFY totalFramesChanged)
    Q_PROPERTY(double durationSeconds READ durationSeconds NOTIFY totalFramesChanged)
    Q_PROPERTY(int sampleRate READ sampleRate NOTIFY totalFramesChanged)
    Q_PROPERTY(int channels READ channels NOTIFY totalFramesChanged)
    Q_PROPERTY(qint64 viewStartFrame READ viewStartFrame WRITE setViewStartFrame NOTIFY viewRangeChanged)
    Q_PROPERTY(qint64 viewEndFrame READ viewEndFrame WRITE setViewEndFrame NOTIFY viewRangeChanged)
    Q_PROPERTY(qint64 selectionStart READ selectionStart NOTIFY selectionChanged)
    Q_PROPERTY(qint64 selectionEnd READ selectionEnd NOTIFY selectionChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)
    Q_PROPERTY(bool canPaste READ canPaste NOTIFY canPasteChanged)

public:
    explicit AudioEngine(QObject *parent = nullptr);
    ~AudioEngine() override;

    bool isLoaded() const { return m_isLoaded; }
    bool isDecoding() const { return m_isDecoding; }
    qreal decodeProgress() const { return m_decodeProgress; }
    QString statusMessage() const { return m_statusMessage; }

    qint64 totalFrames() const;
    double durationSeconds() const;
    int sampleRate() const { return m_pieceTable.sampleRate(); }
    int channels() const { return m_pieceTable.channels(); }

    qint64 viewStartFrame() const { return m_viewStartFrame; }
    qint64 viewEndFrame() const { return m_viewEndFrame; }

    qint64 selectionStart() const { return m_selectionStart; }
    qint64 selectionEnd() const { return m_selectionEnd; }
    bool hasSelection() const { return m_selectionStart >= 0 && m_selectionEnd > m_selectionStart; }
    bool canPaste() const { return m_pieceTable.canPaste(); }

    const PieceTable& pieceTable() const { return m_pieceTable; }

    Q_INVOKABLE void openAudioFile(const QString &filePathOrUrl);
    Q_INVOKABLE void importAudioDialog();
    Q_INVOKABLE void setViewStartFrame(qint64 frame);
    Q_INVOKABLE void setViewEndFrame(qint64 frame);
    Q_INVOKABLE void setViewRange(qint64 startFrame, qint64 endFrame);
    Q_INVOKABLE void zoomAt(qreal centerRatio, qreal factor);
    Q_INVOKABLE void resetView();

    Q_INVOKABLE void setSelection(qint64 startFrame, qint64 endFrame);
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void cutSelection();
    Q_INVOKABLE void copySelection();
    Q_INVOKABLE void pasteAt(qint64 insertFrame);
    Q_INVOKABLE void deleteSelection();

    Q_INVOKABLE double frameToSeconds(qint64 frame) const;
    Q_INVOKABLE qint64 secondsToFrame(double sec) const;

signals:
    void isLoadedChanged();
    void isDecodingChanged();
    void decodeProgressChanged();
    void statusMessageChanged();
    void totalFramesChanged();
    void waveformChanged();
    void viewRangeChanged();
    void selectionChanged();
    void canPasteChanged();
    void decodeError(const QString &errorString);

private slots:
    void onBufferReady();
    void onDecoderFinished();
    void onDecoderError(QAudioDecoder::Error error);
    void onDecoderPositionChanged(qint64 positionMs);
    void onDecoderDurationChanged(qint64 durationMs);

private:
    void startDecoding(const QString &localFilePath);
    bool tryFastWavDecode(const QString &localFilePath);
    void finishLoading(QVector<int16_t> &&decodedSamples, int sampleRate, int channels);
    void clampViewRange();

    PieceTable m_pieceTable;

    bool m_isLoaded;
    bool m_isDecoding;
    qreal m_decodeProgress;
    QString m_statusMessage;

    qint64 m_viewStartFrame;
    qint64 m_viewEndFrame;

    qint64 m_selectionStart;
    qint64 m_selectionEnd;

    std::unique_ptr<QAudioDecoder> m_decoder;
    QVector<int16_t> m_stagingBuffer;
    int m_detectedSampleRate;
    int m_detectedChannels;
    qint64 m_expectedDurationMs;
};
