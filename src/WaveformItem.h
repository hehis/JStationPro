#pragma once

#include <QQuickPaintedItem>
#include <QColor>
#include "AudioEngine.h"

class WaveformItem : public QQuickPaintedItem {
    Q_OBJECT

    Q_PROPERTY(QObject* audioEngine READ audioEngine WRITE setAudioEngine NOTIFY audioEngineChanged)
    Q_PROPERTY(qint64 viewStartFrame READ viewStartFrame WRITE setViewStartFrame NOTIFY viewRangeChanged)
    Q_PROPERTY(qint64 viewEndFrame READ viewEndFrame WRITE setViewEndFrame NOTIFY viewRangeChanged)
    Q_PROPERTY(bool isThumbnail READ isThumbnail WRITE setIsThumbnail NOTIFY isThumbnailChanged)
    Q_PROPERTY(QColor waveColor READ waveColor WRITE setWaveColor NOTIFY styleChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY styleChanged)
    Q_PROPERTY(QColor selectionColor READ selectionColor WRITE setSelectionColor NOTIFY styleChanged)

public:
    explicit WaveformItem(QQuickItem *parent = nullptr);
    ~WaveformItem() override = default;

    QObject* audioEngine() const { return m_audioEngine; }
    void setAudioEngine(QObject *engine);

    qint64 viewStartFrame() const {
        if (!m_isThumbnail && m_viewEndFrame <= m_viewStartFrame && m_audioEngine) {
            return m_audioEngine->viewStartFrame();
        }
        return m_viewStartFrame;
    }
    void setViewStartFrame(qint64 start);

    qint64 viewEndFrame() const {
        if (!m_isThumbnail && m_viewEndFrame <= m_viewStartFrame && m_audioEngine) {
            return m_audioEngine->viewEndFrame();
        }
        return m_viewEndFrame;
    }
    void setViewEndFrame(qint64 end);

    bool isThumbnail() const { return m_isThumbnail; }
    void setIsThumbnail(bool thumb);

    QColor waveColor() const { return m_waveColor; }
    void setWaveColor(const QColor &c);

    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor &c);

    QColor selectionColor() const { return m_selectionColor; }
    void setSelectionColor(const QColor &c);

    void paint(QPainter *painter) override;

    // Helper functions for QML coordinate to frame mapping
    Q_INVOKABLE qint64 xToFrame(qreal x) const;
    Q_INVOKABLE qreal frameToX(qint64 frame) const;

signals:
    void audioEngineChanged();
    void viewRangeChanged();
    void isThumbnailChanged();
    void styleChanged();

private slots:
    void onWaveformChanged();

private:
    AudioEngine *m_audioEngine;
    qint64 m_viewStartFrame;
    qint64 m_viewEndFrame;
    bool m_isThumbnail;

    QColor m_waveColor;
    QColor m_backgroundColor;
    QColor m_selectionColor;
};
