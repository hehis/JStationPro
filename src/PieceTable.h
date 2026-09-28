#pragma once

#include <vector>
#include <cstdint>
#include <QVector>
#include <QtGlobal>
#include "PeakCache.h"

enum class BufferSource {
    Original,
    Add
};

struct Piece {
    BufferSource source;
    qint64 startFrame; // Start frame in physical buffer
    qint64 frameCount; // Number of frames
};

class PieceTable {
public:
    PieceTable();

    void clear();

    // Sets initial decoded audio data into the Original buffer and creates initial piece
    void setOriginalData(const int16_t *data, qint64 frameCount, int sampleRate, int channels);

    // Append new audio data into the Add buffer (e.g. for external paste or recording)
    qint64 appendAddData(const int16_t *data, qint64 frameCount);

    qint64 totalFrames() const;
    int sampleRate() const { return m_sampleRate; }
    int channels() const { return m_channels; }

    // Logical editing operations
    bool deleteRange(qint64 logicalStartFrame, qint64 frameCount);
    bool copyRange(qint64 logicalStartFrame, qint64 frameCount);
    bool cutRange(qint64 logicalStartFrame, qint64 frameCount);
    bool pasteAt(qint64 logicalInsertFrame);
    bool canPaste() const { return !m_clipboard.empty(); }

    // High performance query of min/max values for a logical range
    PeakPoint queryLogicalRange(qint64 logicalStartFrame, qint64 frameCount) const;

    const std::vector<Piece>& pieces() const { return m_pieces; }

private:
    // Splits the piece table at logicalFrame so that logicalFrame lies on a piece boundary
    // Returns index of the piece starting at logicalFrame
    size_t splitAt(qint64 logicalFrame);

    int m_sampleRate;
    int m_channels;

    QVector<int16_t> m_originalBuffer;
    QVector<int16_t> m_addBuffer;

    PeakCache m_originalPeaks;
    PeakCache m_addPeaks;

    std::vector<Piece> m_pieces;
    std::vector<Piece> m_clipboard;
};
