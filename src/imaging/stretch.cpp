#include "src/imaging/stretch.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace sabit::imaging {

ImageStretcher::ImageStretcher(StretchConfig config) : config_(config) {}

void ImageStretcher::set_config(const StretchConfig& config) {
    config_ = config;
}

void ImageStretcher::reset_temporal_smoothing() {
    smooth_initialized_ = false;
}

Histogram ImageStretcher::compute_histogram(const core::Frame& frame) {
    Histogram h;
    h.bins65k.assign(65536, 0);
    h.bins256.fill(0);
    h.bins256_r.fill(0);
    h.bins256_g.fill(0);
    h.bins256_b.fill(0);

    if (!frame.is_valid()) return h;

    size_t count = frame.pixel_count();
    h.total_pixels = count;
    if (count == 0) return h;

    const uint16_t* px = frame.pixels.get();
    uint16_t min_v = 65535;
    uint16_t max_v = 0;
    double sum = 0.0;

    for (size_t i = 0; i < count; ++i) {
        uint16_t val = px[i];
        min_v = std::min(min_v, val);
        max_v = std::max(max_v, val);
        sum += val;

        ++h.bins65k[val];
        ++h.bins256[val >> 8];
    }

    if (frame.channels >= 3) {
        size_t num_px = static_cast<size_t>(frame.width) * static_cast<size_t>(frame.height);
        int ch = frame.channels;
        for (size_t p = 0; p < num_px; ++p) {
            size_t base = p * ch;
            ++h.bins256_r[px[base + 0] >> 8];
            ++h.bins256_g[px[base + 1] >> 8];
            ++h.bins256_b[px[base + 2] >> 8];
        }
    } else {
        h.bins256_r = h.bins256;
        h.bins256_g = h.bins256;
        h.bins256_b = h.bins256;
    }

    h.min_val = min_v;
    h.max_val = max_v;
    h.mean_val = sum / static_cast<double>(count);

    // Compute standard deviation and median
    double var_sum = 0.0;
    uint64_t median_threshold = count / 2;
    uint64_t accum = 0;
    bool found_median = false;

    for (size_t i = 0; i < 65536; ++i) {
        uint32_t bin_c = h.bins65k[i];
        if (bin_c > 0) {
            double diff = static_cast<double>(i) - h.mean_val;
            var_sum += bin_c * (diff * diff);

            accum += bin_c;
            if (!found_median && accum >= median_threshold) {
                h.median_val = static_cast<uint16_t>(i);
                found_median = true;
            }
        }
    }

    h.stddev_val = std::sqrt(var_sum / static_cast<double>(count));
    return h;
}

void ImageStretcher::update_auto_points(const Histogram& hist) {
    if (hist.total_pixels == 0) return;

    uint64_t black_thresh = static_cast<uint64_t>(hist.total_pixels * config_.auto_black_percentile);
    uint64_t white_thresh = static_cast<uint64_t>(hist.total_pixels * config_.auto_white_percentile);

    uint64_t accum = 0;
    float raw_bp = 0.0f;
    float raw_wp = 65535.0f;
    bool found_bp = false;

    for (size_t i = 0; i < 65536; ++i) {
        accum += hist.bins65k[i];
        if (!found_bp && accum >= black_thresh) {
            raw_bp = static_cast<float>(i);
            found_bp = true;
        }
        if (accum >= white_thresh) {
            raw_wp = static_cast<float>(i);
            break;
        }
    }

    if (raw_wp <= raw_bp) {
        raw_wp = raw_bp + 1.0f;
    }

    // Temporal smoothing (EMA) to prevent flickering on static scenes (TC-022)
    if (!smooth_initialized_) {
        smooth_black_ = raw_bp;
        smooth_white_ = raw_wp;
        smooth_initialized_ = true;
    } else {
        float alpha = config_.ema_alpha;
        smooth_black_ = alpha * raw_bp + (1.0f - alpha) * smooth_black_;
        smooth_white_ = alpha * raw_wp + (1.0f - alpha) * smooth_white_;
    }
}

std::vector<uint8_t> ImageStretcher::convert_to_preview_8bit(const core::Frame& frame, Histogram* out_hist) {
    if (!frame.is_valid()) return {};

    Histogram hist = compute_histogram(frame);
    if (out_hist) {
        *out_hist = hist;
    }

    float bp = config_.black_point;
    float wp = config_.white_point;

    if (config_.auto_stretch) {
        update_auto_points(hist);
        bp = smooth_black_;
        wp = smooth_white_;
    }

    if (wp <= bp) {
        wp = bp + 1.0f;
    }

    float inv_range = 1.0f / (wp - bp);
    float gamma = (config_.gamma > 0.05f) ? config_.gamma : 1.0f;
    bool need_gamma = std::abs(gamma - 1.0f) > 0.01f;

    size_t count = frame.pixel_count();
    const uint16_t* src = frame.pixels.get();
    std::vector<uint8_t> dst(count);

    for (size_t i = 0; i < count; ++i) {
        float norm = (static_cast<float>(src[i]) - bp) * inv_range;
        norm = std::clamp(norm, 0.0f, 1.0f);
        if (need_gamma) {
            norm = std::pow(norm, gamma);
        }
        dst[i] = static_cast<uint8_t>(norm * 255.0f + 0.5f);
    }

    // Optional preview-only enhancements
    if (config_.clahe_enabled) {
        apply_clahe(dst.data(), frame.width, frame.height, frame.channels);
    }

    if (config_.sharpen_enabled) {
        apply_sharpen(dst.data(), frame.width, frame.height, frame.channels);
    }

    if (config_.invert_enabled) {
        for (size_t i = 0; i < count; ++i) {
            dst[i] = 255 - dst[i];
        }
    }

    return dst;
}

void ImageStretcher::apply_clahe(uint8_t* img, int w, int h, int channels) const {
    if (channels >= 3) {
        // Compute luminance channel, run CLAHE on luminance, and scale R/G/B preserving chromaticity
        size_t num_px = static_cast<size_t>(w) * static_cast<size_t>(h);
        std::vector<uint8_t> lum(num_px);
        std::vector<uint8_t> lum_orig(num_px);
        for (size_t i = 0; i < num_px; ++i) {
            size_t base = i * channels;
            uint8_t y_val = static_cast<uint8_t>(
                (static_cast<uint32_t>(img[base + 0]) * 77 +
                 static_cast<uint32_t>(img[base + 1]) * 150 +
                 static_cast<uint32_t>(img[base + 2]) * 29) >> 8);
            lum[i] = y_val;
            lum_orig[i] = y_val;
        }
        apply_clahe(lum.data(), w, h, 1);
        for (size_t i = 0; i < num_px; ++i) {
            size_t base = i * channels;
            uint8_t orig_y = lum_orig[i];
            uint8_t new_y = lum[i];
            if (orig_y > 0) {
                float scale = static_cast<float>(new_y) / static_cast<float>(orig_y);
                img[base + 0] = static_cast<uint8_t>(std::clamp(img[base + 0] * scale, 0.0f, 255.0f));
                img[base + 1] = static_cast<uint8_t>(std::clamp(img[base + 1] * scale, 0.0f, 255.0f));
                img[base + 2] = static_cast<uint8_t>(std::clamp(img[base + 2] * scale, 0.0f, 255.0f));
            } else {
                img[base + 0] = new_y;
                img[base + 1] = new_y;
                img[base + 2] = new_y;
            }
        }
        return;
    }

    int tiles_x = std::max(2, config_.clahe_tiles_x);
    int tiles_y = std::max(2, config_.clahe_tiles_y);

    int tile_w = w / tiles_x;
    int tile_h = h / tiles_y;
    if (tile_w < 4 || tile_h < 4) return;

    int total_tiles = tiles_x * tiles_y;
    std::vector<std::array<uint8_t, 256>> cdfs(total_tiles);

    for (int ty = 0; ty < tiles_y; ++ty) {
        int y0 = ty * tile_h;
        int y1 = (ty == tiles_y - 1) ? h : (ty + 1) * tile_h;

        for (int tx = 0; tx < tiles_x; ++tx) {
            int x0 = tx * tile_w;
            int x1 = (tx == tiles_x - 1) ? w : (tx + 1) * tile_w;
            int tile_area = (x1 - x0) * (y1 - y0);

            std::array<uint32_t, 256> hist{};
            for (int y = y0; y < y1; ++y) {
                for (int x = x0; x < x1; ++x) {
                    ++hist[img[y * w + x]];
                }
            }

            // Clip histogram
            uint32_t clip_limit = static_cast<uint32_t>(config_.clahe_clip_limit * (tile_area / 256.0f));
            clip_limit = std::max(1u, clip_limit);

            uint32_t excess = 0;
            for (int i = 0; i < 256; ++i) {
                if (hist[i] > clip_limit) {
                    excess += hist[i] - clip_limit;
                    hist[i] = clip_limit;
                }
            }

            uint32_t bonus = excess / 256;
            uint32_t remainder = excess % 256;
            for (int i = 0; i < 256; ++i) {
                hist[i] += bonus;
                if (static_cast<uint32_t>(i) < remainder) {
                    ++hist[i];
                }
            }

            // Build CDF
            auto& cdf = cdfs[ty * tiles_x + tx];
            uint32_t cum = 0;
            float scale = 255.0f / static_cast<float>(tile_area);
            for (int i = 0; i < 256; ++i) {
                cum += hist[i];
                cdf[i] = static_cast<uint8_t>(std::clamp(cum * scale, 0.0f, 255.0f));
            }
        }
    }

    // Bilinear interpolation between tile centers
    std::vector<uint8_t> out(w * h);
    for (int y = 0; y < h; ++y) {
        float ty_f = (y - tile_h * 0.5f) / tile_h;
        int ty1 = static_cast<int>(std::floor(ty_f));
        int ty2 = ty1 + 1;
        float dy = ty_f - ty1;

        ty1 = std::clamp(ty1, 0, tiles_y - 1);
        ty2 = std::clamp(ty2, 0, tiles_y - 1);

        for (int x = 0; x < w; ++x) {
            float tx_f = (x - tile_w * 0.5f) / tile_w;
            int tx1 = static_cast<int>(std::floor(tx_f));
            int tx2 = tx1 + 1;
            float dx = tx_f - tx1;

            tx1 = std::clamp(tx1, 0, tiles_x - 1);
            tx2 = std::clamp(tx2, 0, tiles_x - 1);

            uint8_t val = img[y * w + x];
            float v11 = cdfs[ty1 * tiles_x + tx1][val];
            float v12 = cdfs[ty1 * tiles_x + tx2][val];
            float v21 = cdfs[ty2 * tiles_x + tx1][val];
            float v22 = cdfs[ty2 * tiles_x + tx2][val];

            float top = (1.0f - dx) * v11 + dx * v12;
            float bot = (1.0f - dx) * v21 + dx * v22;
            float interpolated = (1.0f - dy) * top + dy * bot;

            out[y * w + x] = static_cast<uint8_t>(std::clamp(interpolated, 0.0f, 255.0f));
        }
    }

    std::copy(out.begin(), out.end(), img);
}

void ImageStretcher::apply_sharpen(uint8_t* img, int w, int h, int channels) const {
    if (w < 3 || h < 3 || channels < 1) return;
    float strength = config_.sharpen_strength;
    size_t total_len = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(channels);
    std::vector<uint8_t> out(img, img + total_len);

    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            size_t c_idx = (static_cast<size_t>(y) * w + x) * channels;
            size_t u_idx = (static_cast<size_t>(y - 1) * w + x) * channels;
            size_t d_idx = (static_cast<size_t>(y + 1) * w + x) * channels;
            size_t l_idx = (static_cast<size_t>(y) * w + (x - 1)) * channels;
            size_t r_idx = (static_cast<size_t>(y) * w + (x + 1)) * channels;
            for (int c = 0; c < channels; ++c) {
                int center = img[c_idx + c];
                int neighbors = img[u_idx + c] + img[d_idx + c] + img[l_idx + c] + img[r_idx + c];
                float high_pass = center - (neighbors * 0.25f);
                float sharpened = center + strength * high_pass;
                out[c_idx + c] = static_cast<uint8_t>(std::clamp(sharpened, 0.0f, 255.0f));
            }
        }
    }

    std::copy(out.begin(), out.end(), img);
}

} // namespace sabit::imaging
