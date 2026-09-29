#include "WaveformItem.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QLineF>
#include <QDebug>
#include <algorithm>

WaveformItem::WaveformItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_audioEngine(nullptr)
    , m_viewStartFrame(0)
    , m_viewEndFrame(0)
    , m_isThumbnail(false)
    , m_waveColor(QColor(0, 180, 216))          // Vibrant Cyan
    , m_backgroundColor(QColor(26, 27, 38))    // Modern Dark
    , m_selectionColor(QColor(255, 75, 75, 90)) // Light Red with alpha
{
    setOpaquePainting(true);
}

void WaveformItem::setAudioEngine(QObject *engineObj)
{
    AudioEngine *engine = qobject_cast<AudioEngine*>(engineObj);
    if (m_audioEngine == engine)
        return;

    if (m_audioEngine) {
        disconnect(m_audioEngine, nullptr, this, nullptr);
    }

    m_audioEngine = engine;

    if (m_audioEngine) {
        connect(m_audioEngine, &AudioEngine::waveformChanged, this, &WaveformItem::onWaveformChanged);
        connect(m_audioEngine, &AudioEngine::selectionChanged, this, &WaveformItem::onWaveformChanged);
        connect(m_audioEngine, &AudioEngine::isLoadedChanged, this, &WaveformItem::onWaveformChanged);
        connect(m_audioEngine, &AudioEngine::viewRangeChanged, this, &WaveformItem::onWaveformChanged);
    }

    emit audioEngineChanged();
    update();
}

void WaveformItem::setViewStartFrame(qint64 start)
{
    if (m_viewStartFrame != start) {
        m_viewStartFrame = start;
        emit viewRangeChanged();
        update();
    }
}

void WaveformItem::setViewEndFrame(qint64 end)
{
    if (m_viewEndFrame != end) {
        m_viewEndFrame = end;
        emit viewRangeChanged();
        update();
    }
}

void WaveformItem::setIsThumbnail(bool thumb)
{
    if (m_isThumbnail != thumb) {
        m_isThumbnail = thumb;
        emit isThumbnailChanged();
        update();
    }
}

void WaveformItem::setWaveColor(const QColor &c)
{
    if (m_waveColor != c) {
        m_waveColor = c;
        emit styleChanged();
        update();
    }
}

void WaveformItem::setBackgroundColor(const QColor &c)
{
    if (m_backgroundColor != c) {
        m_backgroundColor = c;
        emit styleChanged();
        update();
    }
}

void WaveformItem::setSelectionColor(const QColor &c)
{
    if (m_selectionColor != c) {
        m_selectionColor = c;
        emit styleChanged();
        update();
    }
}

void WaveformItem::onWaveformChanged()
{
    update();
}

qint64 WaveformItem::xToFrame(qreal x) const
{
    if (!m_audioEngine || width() <= 0) return 0;
    qint64 total = m_audioEngine->totalFrames();
    if (total <= 0) return 0;

    qint64 rStart = 0;
    qint64 rEnd = total;
    if (!m_isThumbnail) {
        qint64 vStart = m_viewStartFrame;
        qint64 vEnd = m_viewEndFrame;
        if (vEnd <= vStart || vEnd <= 0) {
            vStart = m_audioEngine->viewStartFrame();
            vEnd   = m_audioEngine->viewEndFrame();
        }
        if (vEnd > vStart) {
            rStart = std::max<qint64>(0, std::min<qint64>(vStart, total));
            rEnd   = std::max<qint64>(0, std::min<qint64>(vEnd, total));
        }
    }

    qint64 rLen = rEnd - rStart;
    if (rLen <= 0) return rStart;

    qreal clampedX = std::max<qreal>(0.0, std::min<qreal>(x, width()));
    return rStart + static_cast<qint64>((clampedX / width()) * rLen);
}

qreal WaveformItem::frameToX(qint64 frame) const
{
    if (!m_audioEngine || width() <= 0) return 0.0;
    qint64 total = m_audioEngine->totalFrames();
    if (total <= 0) return 0.0;

    qint64 rStart = 0;
    qint64 rEnd = total;
    if (!m_isThumbnail) {
        qint64 vStart = m_viewStartFrame;
        qint64 vEnd = m_viewEndFrame;
        if (vEnd <= vStart || vEnd <= 0) {
            vStart = m_audioEngine->viewStartFrame();
            vEnd   = m_audioEngine->viewEndFrame();
        }
        if (vEnd > vStart) {
            rStart = std::max<qint64>(0, std::min<qint64>(vStart, total));
            rEnd   = std::max<qint64>(0, std::min<qint64>(vEnd, total));
        }
    }

    qint64 rLen = rEnd - rStart;
    if (rLen <= 0) return 0.0;

    return (static_cast<qreal>(frame - rStart) / rLen) * width();
}

void WaveformItem::paint(QPainter *painter)
{
    int w = static_cast<int>(width());
    int h = static_cast<int>(height());
    if (w <= 0 || h <= 0) return;

    // Fill background
    painter->fillRect(0, 0, w, h, m_backgroundColor);

    qreal centerY = h / 2.0;

    // Draw baseline
    painter->setPen(QPen(QColor(50, 54, 75), 1));
    painter->drawLine(0, static_cast<int>(centerY), w, static_cast<int>(centerY));

    if (!m_audioEngine || !m_audioEngine->isLoaded() || m_audioEngine->totalFrames() <= 0) {
        return;
    }

    qint64 total = m_audioEngine->totalFrames();
    qint64 rStart = 0;
    qint64 rEnd = total;

    if (!m_isThumbnail) {
        qint64 vStart = m_viewStartFrame;
        qint64 vEnd = m_viewEndFrame;
        if (vEnd <= vStart || vEnd <= 0) {
            vStart = m_audioEngine->viewStartFrame();
            vEnd   = m_audioEngine->viewEndFrame();
        }
        if (vEnd > vStart) {
            rStart = std::max<qint64>(0, std::min<qint64>(vStart, total));
            rEnd   = std::max<qint64>(0, std::min<qint64>(vEnd, total));
        } else {
            rStart = 0;
            rEnd = total;
        }
    }

    qint64 rLen = rEnd - rStart;
    if (rLen <= 0) rLen = total;
    if (rLen <= 0) return;

    qreal halfH = (h - 6) / 2.0;
    if (halfH < 1.0) halfH = 1.0;

    // Disable antialiasing for crisp single-pixel vertical lines
    painter->setRenderHint(QPainter::Antialiasing, false);

    const PieceTable &pt = m_audioEngine->pieceTable();

    if (rLen <= w) {
        // Zoomed in: 1 pixel or more per sample
        // Enable antialiasing for smooth lines
        painter->setRenderHint(QPainter::Antialiasing, true);

        QVector<QPointF> points;
        points.reserve(rLen + 1);

        qreal spacing = static_cast<qreal>(w) / static_cast<qreal>(rLen);
        bool showText = spacing >= 35.0; // Show value text if spacing is comfortable

        painter->setFont(QFont("Arial", 8));
        QPen textPen(QColor(200, 200, 200));

        for (qint64 i = 0; i < rLen; ++i) {
            qint64 frame = rStart + i;
            PeakPoint peak = pt.queryLogicalRange(frame, 1);
            qreal y = centerY - (static_cast<qreal>(peak.maxVal) / 32768.0) * halfH;
            qreal x = (static_cast<qreal>(i) / static_cast<qreal>(rLen)) * w;
            points.push_back(QPointF(x, y));
        }

        // Connect the dots
        painter->setPen(QPen(m_waveColor, 1.5));
        painter->drawPolyline(points.constData(), points.size());

        // Draw points and optional text
        for (int i = 0; i < points.size(); ++i) {
            QPointF p = points[i];
            // Draw a small circle at the sample point
            painter->setBrush(m_waveColor);
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(p, 3.0, 3.0);

            if (showText) {
                PeakPoint peak = pt.queryLogicalRange(rStart + i, 1);
                painter->setPen(textPen);
                QString valStr = QString::number(peak.maxVal);
                // Draw text above the point, or below if it's near the top
                qreal ty = (p.y() > 20) ? p.y() - 8 : p.y() + 15;
                painter->drawText(QRectF(p.x() - 30, ty - 10, 60, 20), Qt::AlignCenter, valStr);
            }
        }
    } else {
        // Zoomed out: multiple samples per pixel
        painter->setRenderHint(QPainter::Antialiasing, false);
        QVector<QLineF> lines;
        lines.reserve(w);

        for (int x = 0; x < w; ++x) {
            qint64 fStart = rStart + (static_cast<qint64>(x) * rLen) / w;
            qint64 fEnd   = rStart + (static_cast<qint64>(x + 1) * rLen) / w;
            if (fEnd <= fStart) fEnd = fStart + 1;

            PeakPoint peak = pt.queryLogicalRange(fStart, fEnd - fStart);

            qreal yTop    = centerY - (static_cast<qreal>(peak.maxVal) / 32768.0) * halfH;
            qreal yBottom = centerY - (static_cast<qreal>(peak.minVal) / 32768.0) * halfH;

            if (yBottom - yTop < 2.0) {
                yTop = centerY - 1.0;
                yBottom = centerY + 1.0;
            }

            lines.push_back(QLineF(x, yTop, x, yBottom));
        }

        painter->setPen(QPen(m_waveColor, 1));
        painter->drawLines(lines.constData(), lines.size());
    }

    // Draw Selection overlay or Cursor
    if (m_audioEngine->selectionStart() >= 0) {
        qint64 selStart = m_audioEngine->selectionStart();
        qint64 selEnd   = m_audioEngine->selectionEnd();

        bool hasRange = (selEnd > selStart);

        if (hasRange) {
            if (selEnd > rStart && selStart < rEnd) {
                qreal x1 = (static_cast<qreal>(std::max(selStart, rStart) - rStart) / rLen) * w;
                qreal x2 = (static_cast<qreal>(std::min(selEnd, rEnd) - rStart) / rLen) * w;
                qreal selW = std::max<qreal>(1.0, x2 - x1);

                QRectF selRect(x1, 0, selW, h);
                painter->fillRect(selRect, m_selectionColor);

                painter->setPen(QPen(QColor(255, 80, 80, 220), 1.5));
                if (selStart >= rStart) {
                    painter->drawLine(QLineF(x1, 0, x1, h));
                }
                if (selEnd <= rEnd) {
                    painter->drawLine(QLineF(x2, 0, x2, h));
                }
            }
        } else {
            // Draw single cursor line
            if (selStart >= rStart && selStart <= rEnd) {
                qreal xCursor = (static_cast<qreal>(selStart - rStart) / rLen) * w;
                painter->setPen(QPen(QColor(255, 80, 80, 220), 1.5));
                painter->drawLine(QLineF(xCursor, 0, xCursor, h));
            }
        }
    }
}
