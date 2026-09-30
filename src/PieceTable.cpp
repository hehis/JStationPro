#include "PieceTable.h"
#include <algorithm>

PieceTable::PieceTable()
    : m_sampleRate(44100)
    , m_channels(2)
{
}

void PieceTable::clear()
{
    m_originalBuffer.clear();
    m_addBuffer.clear();
    m_originalPeaks.clear();
    m_addPeaks.clear();
    m_pieces.clear();
    m_clipboard.clear();
}

void PieceTable::setOriginalData(const int16_t *data, qint64 frameCount, int sampleRate, int channels)
{
    clear();
    m_sampleRate = sampleRate > 0 ? sampleRate : 44100;
    m_channels = channels > 0 ? channels : 2;

    if (!data || frameCount <= 0)
        return;

    qint64 totalSamples = frameCount * m_channels;
    m_originalBuffer.resize(totalSamples);
    std::copy(data, data + totalSamples, m_originalBuffer.begin());

    m_originalPeaks.appendSamples(m_originalBuffer.constData(), frameCount, m_channels);

    Piece initialPiece;
    initialPiece.source = BufferSource::Original;
    initialPiece.startFrame = 0;
    initialPiece.frameCount = frameCount;
    m_pieces.push_back(initialPiece);
}

qint64 PieceTable::appendAddData(const int16_t *data, qint64 frameCount)
{
    if (!data || frameCount <= 0 || m_channels <= 0)
        return -1;

    qint64 startFrame = m_addBuffer.size() / m_channels;
    qint64 totalSamples = frameCount * m_channels;
    qint64 oldSize = m_addBuffer.size();
    m_addBuffer.resize(oldSize + totalSamples);
    std::copy(data, data + totalSamples, m_addBuffer.begin() + oldSize);

    m_addPeaks.appendSamples(data, frameCount, m_channels);
    return startFrame;
}

qint64 PieceTable::totalFrames() const
{
    qint64 total = 0;
    for (const auto &p : m_pieces) {
        total += p.frameCount;
    }
    return total;
}

size_t PieceTable::splitAt(qint64 logicalFrame)
{
    if (logicalFrame <= 0)
        return 0;

    qint64 cur = 0;
    for (size_t i = 0; i < m_pieces.size(); ++i) {
        qint64 next = cur + m_pieces[i].frameCount;
        if (logicalFrame == cur) {
            return i;
        }
        if (logicalFrame < next) {
            // Split piece i
            qint64 offset = logicalFrame - cur;
            Piece left = m_pieces[i];
            left.frameCount = offset;

            Piece right = m_pieces[i];
            right.startFrame += offset;
            right.frameCount -= offset;

            m_pieces[i] = left;
            m_pieces.insert(m_pieces.begin() + i + 1, right);
            return i + 1;
        }
        cur = next;
    }

    return m_pieces.size();
}

bool PieceTable::deleteRange(qint64 logicalStartFrame, qint64 frameCount)
{
    if (frameCount <= 0 || m_pieces.empty())
        return false;

    qint64 total = totalFrames();
    if (logicalStartFrame >= total)
        return false;

    if (logicalStartFrame < 0) {
        frameCount += logicalStartFrame;
        logicalStartFrame = 0;
    }
    if (logicalStartFrame + frameCount > total) {
        frameCount = total - logicalStartFrame;
    }
    if (frameCount <= 0)
        return false;

    size_t idxStart = splitAt(logicalStartFrame);
    size_t idxEnd = splitAt(logicalStartFrame + frameCount);

    m_pieces.erase(m_pieces.begin() + idxStart, m_pieces.begin() + idxEnd);
    return true;
}

bool PieceTable::copyRange(qint64 logicalStartFrame, qint64 frameCount)
{
    if (frameCount <= 0 || m_pieces.empty())
        return false;

    qint64 total = totalFrames();
    if (logicalStartFrame >= total)
        return false;

    if (logicalStartFrame < 0) {
        frameCount += logicalStartFrame;
        logicalStartFrame = 0;
    }
    if (logicalStartFrame + frameCount > total) {
        frameCount = total - logicalStartFrame;
    }
    if (frameCount <= 0)
        return false;

    size_t idxStart = splitAt(logicalStartFrame);
    size_t idxEnd = splitAt(logicalStartFrame + frameCount);

    m_clipboard.assign(m_pieces.begin() + idxStart, m_pieces.begin() + idxEnd);
    return true;
}

bool PieceTable::cutRange(qint64 logicalStartFrame, qint64 frameCount)
{
    if (!copyRange(logicalStartFrame, frameCount))
        return false;
    return deleteRange(logicalStartFrame, frameCount);
}

bool PieceTable::pasteAt(qint64 logicalInsertFrame)
{
    if (m_clipboard.empty())
        return false;

    qint64 total = totalFrames();
    if (logicalInsertFrame < 0)
        logicalInsertFrame = 0;
    if (logicalInsertFrame > total)
        logicalInsertFrame = total;

    size_t idx = splitAt(logicalInsertFrame);
    m_pieces.insert(m_pieces.begin() + idx, m_clipboard.begin(), m_clipboard.end());
    return true;
}

PeakPoint PieceTable::queryLogicalRange(qint64 logicalStartFrame, qint64 frameCount) const
{
    PeakPoint result = { 0, 0 };
    if (frameCount <= 0 || m_pieces.empty())
        return result;

    qint64 logicalEndFrame = logicalStartFrame + frameCount;
    qint64 curLogical = 0;
    bool hasResult = false;

    for (const auto &p : m_pieces) {
        qint64 pStart = curLogical;
        qint64 pEnd = curLogical + p.frameCount;

        if (pEnd <= logicalStartFrame) {
            curLogical = pEnd;
            continue;
        }
        if (pStart >= logicalEndFrame) {
            break;
        }

        // Overlap with piece p
        qint64 overlapStart = std::max(logicalStartFrame, pStart);
        qint64 overlapEnd = std::min(logicalEndFrame, pEnd);
        qint64 overlapCount = overlapEnd - overlapStart;

        if (overlapCount > 0) {
            qint64 inPieceOffset = overlapStart - pStart;
            qint64 physStart = p.startFrame + inPieceOffset;

            PeakPoint pt;
            if (p.source == BufferSource::Original) {
                const int16_t *rawPtr = m_originalBuffer.isEmpty() ? nullptr : m_originalBuffer.constData();
                pt = m_originalPeaks.queryRange(physStart, overlapCount, rawPtr, m_channels);
            } else {
                const int16_t *rawPtr = m_addBuffer.isEmpty() ? nullptr : m_addBuffer.constData();
                pt = m_addPeaks.queryRange(physStart, overlapCount, rawPtr, m_channels);
            }

            if (!hasResult) {
                result = pt;
                hasResult = true;
            } else {
                if (pt.minVal < result.minVal) result.minVal = pt.minVal;
                if (pt.maxVal > result.maxVal) result.maxVal = pt.maxVal;
            }
        }

        curLogical = pEnd;
    }

    return result;
}

qint64 PieceTable::readFrames(qint64 logicalStartFrame, qint64 frameCount, int16_t* outBuffer) const
{
    if (frameCount <= 0 || logicalStartFrame < 0 || m_pieces.empty())
        return 0;

    qint64 logicalEndFrame = logicalStartFrame + frameCount;
    qint64 curLogical = 0;
    qint64 outIndex = 0;

    for (const auto &p : m_pieces) {
        qint64 pStart = curLogical;
        qint64 pEnd = curLogical + p.frameCount;

        if (pEnd <= logicalStartFrame) {
            curLogical = pEnd;
            continue;
        }
        if (pStart >= logicalEndFrame) {
            break;
        }

        qint64 overlapStart = std::max(logicalStartFrame, pStart);
        qint64 overlapEnd = std::min(logicalEndFrame, pEnd);
        qint64 overlapCount = overlapEnd - overlapStart;

        if (overlapCount > 0) {
            qint64 inPieceOffset = overlapStart - pStart;
            qint64 physStart = p.startFrame + inPieceOffset;
            
            const int16_t *srcBuffer = (p.source == BufferSource::Original) ? 
                (m_originalBuffer.isEmpty() ? nullptr : m_originalBuffer.constData()) : 
                (m_addBuffer.isEmpty() ? nullptr : m_addBuffer.constData());
                
            if (srcBuffer) {
                memcpy(outBuffer + outIndex * m_channels, srcBuffer + physStart * m_channels, overlapCount * m_channels * sizeof(int16_t));
            } else {
                memset(outBuffer + outIndex * m_channels, 0, overlapCount * m_channels * sizeof(int16_t));
            }
            outIndex += overlapCount;
        }
        curLogical = pEnd;
    }
    return outIndex;
}
