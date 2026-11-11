#include "src/imaging/debayer.h"

#include <algorithm>

namespace sabit::imaging {

namespace {

// Colour index of the four sites of a 2x2 CFA cell: 0 = R, 1 = G, 2 = B.
// cfa[y & 1][x & 1]
struct Cfa {
    int c[2][2];
};

Cfa make_cfa(core::BayerPattern p) {
    switch (p) {
        case core::BayerPattern::BGGR: return Cfa{{{2, 1}, {1, 0}}};
        case core::BayerPattern::GRBG: return Cfa{{{1, 0}, {2, 1}}};
        case core::BayerPattern::GBRG: return Cfa{{{1, 2}, {0, 1}}};
        case core::BayerPattern::RGGB:
        case core::BayerPattern::None:
        default:                       return Cfa{{{0, 1}, {1, 2}}};
    }
}

core::BayerPattern pattern_from_cfa(const Cfa& cfa) {
    const int a = cfa.c[0][0];
    const int b = cfa.c[0][1];
    if (a == 0) return core::BayerPattern::RGGB;
    if (a == 2) return core::BayerPattern::BGGR;
    // a == G
    return (b == 0) ? core::BayerPattern::GRBG : core::BayerPattern::GBRG;
}

} // namespace

core::BayerPattern shift_bayer_pattern(core::BayerPattern base, int offset_x, int offset_y) {
    if (base == core::BayerPattern::None) return base;
    const Cfa src = make_cfa(base);
    const int ox = offset_x & 1;
    const int oy = offset_y & 1;
    Cfa dst{};
    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < 2; ++x) {
            dst.c[y][x] = src.c[(y + oy) & 1][(x + ox) & 1];
        }
    }
    return pattern_from_cfa(dst);
}

bool needs_debayer(const core::Frame& frame) {
    return frame.is_valid() && frame.channels == 1 && frame.bayer != core::BayerPattern::None &&
           frame.width >= 2 && frame.height >= 2;
}

core::Frame debayer_bilinear(const core::Frame& mosaic) {
    if (!needs_debayer(mosaic)) {
        return mosaic;
    }

    const int w = mosaic.width;
    const int h = mosaic.height;
    core::Frame out = core::Frame::create_allocated(w, h, 3, mosaic.bit_depth);
    out.seq = mosaic.seq;
    out.exposure_us = mosaic.exposure_us;
    out.gain = mosaic.gain;
    out.sensor_temp_c = mosaic.sensor_temp_c;
    out.binning = mosaic.binning;
    out.timestamp_utc = mosaic.timestamp_utc;
    out.clock_sync = mosaic.clock_sync;
    out.bayer = core::BayerPattern::None;

    const Cfa cfa = make_cfa(mosaic.bayer);
    const uint16_t* src = mosaic.pixels.get();
    uint16_t* dst = out.pixels.get();

    for (int y = 0; y < h; ++y) {
        const int y0 = std::max(0, y - 1);
        const int y1 = std::min(h - 1, y + 1);
        for (int x = 0; x < w; ++x) {
            const int x0 = std::max(0, x - 1);
            const int x1 = std::min(w - 1, x + 1);

            uint32_t sum[3] = {0, 0, 0};
            uint32_t cnt[3] = {0, 0, 0};
            for (int yy = y0; yy <= y1; ++yy) {
                const uint16_t* row = src + static_cast<size_t>(yy) * w;
                const int* crow = cfa.c[yy & 1];
                for (int xx = x0; xx <= x1; ++xx) {
                    const int c = crow[xx & 1];
                    sum[c] += row[xx];
                    ++cnt[c];
                }
            }

            const int own = cfa.c[y & 1][x & 1];
            uint16_t* px = dst + (static_cast<size_t>(y) * w + x) * 3;
            for (int c = 0; c < 3; ++c) {
                if (c == own) {
                    px[c] = src[static_cast<size_t>(y) * w + x];
                } else {
                    px[c] = cnt[c] ? static_cast<uint16_t>(sum[c] / cnt[c]) : 0;
                }
            }
        }
    }
    return out;
}

} // namespace sabit::imaging
