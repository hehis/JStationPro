#include <cassert>
#include <iostream>
#include <vector>
#include "../src/PieceTable.h"

int main()
{
    PieceTable table;
    const int frameCount = 1000;
    std::vector<int16_t> dummyData(frameCount * 2);
    for (int i = 0; i < frameCount * 2; ++i) {
        dummyData[i] = static_cast<int16_t>(i % 30000);
    }

    table.setOriginalData(dummyData.data(), frameCount, 44100, 2);
    assert(table.totalFrames() == 1000);
    std::cout << "[PASS] setOriginalData: totalFrames = " << table.totalFrames() << std::endl;

    // Test Copy
    bool copyOk = table.copyRange(100, 200);
    assert(copyOk);
    assert(table.totalFrames() == 1000);
    assert(table.canPaste());
    std::cout << "[PASS] copyRange: totalFrames unchanged = " << table.totalFrames() << std::endl;

    // Test Cut
    bool cutOk = table.cutRange(500, 100);
    assert(cutOk);
    assert(table.totalFrames() == 900);
    std::cout << "[PASS] cutRange: totalFrames = " << table.totalFrames() << std::endl;

    // Test Paste
    bool pasteOk = table.pasteAt(200);
    assert(pasteOk);
    assert(table.totalFrames() == 1100); // 900 + 200 from last cut
    std::cout << "[PASS] pasteAt: totalFrames = " << table.totalFrames() << std::endl;

    // Test Delete
    bool delOk = table.deleteRange(0, 100);
    assert(delOk);
    assert(table.totalFrames() == 1000);
    std::cout << "[PASS] deleteRange: totalFrames = " << table.totalFrames() << std::endl;

    // Test Query
    PeakPoint p = table.queryLogicalRange(50, 100);
    std::cout << "[PASS] queryLogicalRange: min = " << p.minVal << ", max = " << p.maxVal << std::endl;

    std::cout << "All PieceTable unit tests PASSED!" << std::endl;
    return 0;
}
