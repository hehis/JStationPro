#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>
#include <QtGlobal>

struct PeakPoint {
    int16_t minVal;
    int16_t maxVal;
};

class PeakCache {
public:
    PeakCache();

    // Clears all cached peak levels
    void clear();

    // Appends interleaved int16 samples into peak pyramid
    // frameCount: number of sample frames (each frame has channels samples)
    void appendSamples(const int16_t *samples, qint64 frameCount, int channels);

    // Queries min and max values for a range of frames [startFrame, startFrame + count)
    PeakPoint queryRange(qint64 startFrame, qint64 count, const int16_t *rawBuffer, int channels) const;

    qint64 totalFrames() const { return m_totalFrames; }

private:
    qint64 m_totalFrames;

    // LOD factors: e.g. 64, 256, 1024, 4096, 16384
    static const int LOD_COUNT = 5;
    static const int LOD_FACTORS[LOD_COUNT];

    // Pyramid storage: m_levels[i] has resolution LOD_FACTORS[i] frames per peak
    std::vector<PeakPoint> m_levels[LOD_COUNT];
};
