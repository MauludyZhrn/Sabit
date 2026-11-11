#ifndef SABIT_IMAGING_DEBAYER_H
#define SABIT_IMAGING_DEBAYER_H

#include "src/core/frame.h"

namespace sabit::imaging {

// Shift a Bayer pattern by a sensor offset (e.g. ASCOM BayerOffsetX/Y or an odd ROI origin).
// The returned pattern describes the colour layout starting at the shifted (0,0) pixel.
core::BayerPattern shift_bayer_pattern(core::BayerPattern base, int offset_x, int offset_y);

// Returns true when the frame is a single-channel colour mosaic that must be demosaiced
// before it can be shown as RGB.
[[nodiscard]] bool needs_debayer(const core::Frame& frame);

// Bilinear demosaic of a 16-bit Bayer mosaic into an interleaved 3-channel RGB frame.
// Metadata (seq, exposure, gain, timestamp, ...) is copied; the output has bayer=None.
// If the input does not need debayering, the input frame is returned unchanged.
core::Frame debayer_bilinear(const core::Frame& mosaic);

} // namespace sabit::imaging

#endif // SABIT_IMAGING_DEBAYER_H
