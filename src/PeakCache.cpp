#include "PeakCache.h"

const int PeakCache::LOD_COUNT;
const int PeakCache::LOD_FACTORS[PeakCache::LOD_COUNT] = { 64, 256, 1024, 4096, 16384 };

PeakCache::PeakCache()
    : m_totalFrames(0)
{
}

void PeakCache::clear()
{
    m_totalFrames = 0;
    for (int i = 0; i < LOD_COUNT; ++i) {
        m_levels[i].clear();
    }
}

void PeakCache::appendSamples(const int16_t *samples, qint64 frameCount, int channels)
{
    if (!samples || frameCount <= 0 || channels <= 0)
        return;

    qint64 startFrame = m_totalFrames;
    m_totalFrames += frameCount;

    // Build Level 0 (factor 64)
    int factor0 = LOD_FACTORS[0]; // 64
    qint64 targetPeaksL0 = (m_totalFrames + factor0 - 1) / factor0;
    size_t currentPeaksL0 = m_levels[0].size();

    for (size_t p = currentPeaksL0; p < (size_t)targetPeaksL0; ++p) {
        qint64 fStart = p * factor0;
        qint64 fEnd = std::min(fStart + factor0, m_totalFrames);

        int16_t minVal = 0;
        int16_t maxVal = 0;
        bool first = true;

        for (qint64 f = fStart; f < fEnd; ++f) {
            const int16_t *chSample = samples + f * channels;
            for (int c = 0; c < channels; ++c) {
                int16_t val = chSample[c];
                if (first) {
                    minVal = maxVal = val;
                    first = false;
                } else {
                    if (val < minVal) minVal = val;
                    if (val > maxVal) maxVal = val;
                }
            }
        }
        m_levels[0].push_back({ minVal, maxVal });
    }

    // Build subsequent levels by aggregating previous level
    for (int lvl = 1; lvl < LOD_COUNT; ++lvl) {
        int ratio = LOD_FACTORS[lvl] / LOD_FACTORS[lvl - 1]; // 4
        int factor = LOD_FACTORS[lvl];
        qint64 targetPeaks = (m_totalFrames + factor - 1) / factor;
        size_t currentPeaks = m_levels[lvl].size();

        for (size_t p = currentPeaks; p < (size_t)targetPeaks; ++p) {
            size_t prevStart = p * ratio;
            size_t prevEnd = std::min(prevStart + ratio, m_levels[lvl - 1].size());

            int16_t minVal = 0;
            int16_t maxVal = 0;
            bool first = true;

            for (size_t i = prevStart; i < prevEnd; ++i) {
                const PeakPoint &pt = m_levels[lvl - 1][i];
                if (first) {
                    minVal = pt.minVal;
                    maxVal = pt.maxVal;
                    first = false;
                } else {
                    if (pt.minVal < minVal) minVal = pt.minVal;
                    if (pt.maxVal > maxVal) maxVal = pt.maxVal;
                }
            }
            m_levels[lvl].push_back({ minVal, maxVal });
        }
    }
}

PeakPoint PeakCache::queryRange(qint64 startFrame, qint64 count, const int16_t *rawBuffer, int channels) const
{
    PeakPoint result = { 0, 0 };
    if (count <= 0 || startFrame >= m_totalFrames)
        return result;

    qint64 endFrame = std::min(startFrame + count, m_totalFrames);
    if (startFrame >= endFrame)
        return result;

    // Find best LOD level where count is at least 2x the factor
    int bestLvl = -1;
    for (int lvl = LOD_COUNT - 1; lvl >= 0; --lvl) {
        if (count >= LOD_FACTORS[lvl] * 2) {
            bestLvl = lvl;
            break;
        }
    }

    if (bestLvl >= 0) {
        int factor = LOD_FACTORS[bestLvl];
        qint64 pStart = (startFrame + factor - 1) / factor;
        qint64 pEnd = endFrame / factor;

        bool hasVal = false;
        int16_t minVal = 0, maxVal = 0;

        // Peak lookup for aligned middle portion
        if (pStart < pEnd) {
            for (qint64 p = pStart; p < pEnd && p < (qint64)m_levels[bestLvl].size(); ++p) {
                const PeakPoint &pt = m_levels[bestLvl][p];
                if (!hasVal) {
                    minVal = pt.minVal;
                    maxVal = pt.maxVal;
                    hasVal = true;
                } else {
                    if (pt.minVal < minVal) minVal = pt.minVal;
                    if (pt.maxVal > maxVal) maxVal = pt.maxVal;
                }
            }
        }

        // Edge 1: startFrame to pStart * factor
        qint64 edge1End = std::min(pStart * factor, endFrame);
        if (rawBuffer && channels > 0 && startFrame < edge1End) {
            for (qint64 f = startFrame; f < edge1End; ++f) {
                const int16_t *ch = rawBuffer + f * channels;
                for (int c = 0; c < channels; ++c) {
                    int16_t v = ch[c];
                    if (!hasVal) { minVal = maxVal = v; hasVal = true; }
                    else { if (v < minVal) minVal = v; if (v > maxVal) maxVal = v; }
                }
            }
        }

        // Edge 2: pEnd * factor to endFrame
        qint64 edge2Start = std::max(pEnd * factor, startFrame);
        if (rawBuffer && channels > 0 && edge2Start < endFrame) {
            for (qint64 f = edge2Start; f < endFrame; ++f) {
                const int16_t *ch = rawBuffer + f * channels;
                for (int c = 0; c < channels; ++c) {
                    int16_t v = ch[c];
                    if (!hasVal) { minVal = maxVal = v; hasVal = true; }
                    else { if (v < minVal) minVal = v; if (v > maxVal) maxVal = v; }
                }
            }
        }

        if (hasVal) {
            result.minVal = minVal;
            result.maxVal = maxVal;
            return result;
        }
    }

    // Direct scan if count is small or LOD not applicable
    if (rawBuffer && channels > 0) {
        bool first = true;
        for (qint64 f = startFrame; f < endFrame; ++f) {
            const int16_t *ch = rawBuffer + f * channels;
            for (int c = 0; c < channels; ++c) {
                int16_t v = ch[c];
                if (first) {
                    result.minVal = result.maxVal = v;
                    first = false;
                } else {
                    if (v < result.minVal) result.minVal = v;
                    if (v > result.maxVal) result.maxVal = v;
                }
            }
        }
    }

    return result;
}
