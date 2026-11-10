#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <filesystem>
#include <algorithm>

#include "src/core/frame.h"
#include "src/imaging/calibration.h"
#include "src/imaging/stacker.h"
#include "src/imaging/stretch.h"
#include "src/imaging/io/fits_writer.h"
#include "src/imaging/io/image_writer.h"
#include "src/imaging/io/avi_writer.h"
#include "src/imaging/io/mp4_writer.h"
#include "src/astro/site.h"
#include "src/astro/ephemeris.h"
#include "src/astro/criteria.h"
#include "src/astro/hijri.h"
#include "src/astro/plate_solver.h"

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [FAIL] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            ++g_tests_failed; \
            return; \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        std::cout << "[RUN ] " << #fn << "...\n"; \
        int before = g_tests_failed; \
        fn(); \
        if (g_tests_failed == before) { \
            std::cout << "  [PASS] " << #fn << "\n"; \
            ++g_tests_passed; \
        } \
    } while (0)

using namespace sabit::core;
using namespace sabit::imaging;
using namespace sabit::imaging::io;
using namespace sabit::astro;

// 1. TC-024 & TC-042: FITS 16-bit Lossless Gradient and Header Verification
void test_fits_16bit_lossless_roundtrip_and_headers() {
    int w = 256;
    int h = 256;
    size_t count = w * h; // 65,536 pixels: exact 1-to-1 mapping for 0..65535!

    Frame grad = Frame::create_allocated(w, h, 1, 16);
    grad.seq = 1042;
    grad.exposure_us = 250000; // 0.25 s
    grad.gain = 180.5;
    grad.sensor_temp_c = -10.5;
    grad.binning = 1;

    for (size_t i = 0; i < count; ++i) {
        grad.pixels[i] = static_cast<uint16_t>(i);
    }

    FitsMetadata in_meta;
    in_meta.object = "Hilal Test";
    in_meta.instrument = "Sabit ZWO ASI294";
    in_meta.site_latitude = -6.2088;
    in_meta.site_longitude = 106.8456;
    in_meta.site_elevation_m = 10.0;

    std::string test_fits_file = "test_gradient_65k.fits";
    auto write_res = FitsWriter::write_fits_16bit(test_fits_file, grad, in_meta);
    TEST_ASSERT(write_res.is_ok(), "FITS write should succeed");

    FitsMetadata out_meta;
    auto read_res = FitsWriter::read_fits_16bit(test_fits_file, &out_meta);
    TEST_ASSERT(read_res.is_ok(), "FITS read should succeed");

    Frame loaded = read_res.value();
    TEST_ASSERT(loaded.width == w, "Width should match");
    TEST_ASSERT(loaded.height == h, "Height should match");
    TEST_ASSERT(loaded.bit_depth == 16, "Bit depth should be 16");

    // TC-042 Header Checks
    TEST_ASSERT(loaded.exposure_us == 250000, "EXPTIME header roundtrip check");
    TEST_ASSERT(std::abs(loaded.gain - 180.5) < 0.01, "GAIN header roundtrip check");
    TEST_ASSERT(std::abs(loaded.sensor_temp_c - (-10.5)) < 0.01, "CCD-TEMP header roundtrip check");
    TEST_ASSERT(loaded.binning == 1, "XBINNING header check");
    TEST_ASSERT(std::abs(out_meta.site_latitude - (-6.2088)) < 0.001, "SITELAT header check");
    TEST_ASSERT(std::abs(out_meta.site_longitude - 106.8456) < 0.001, "SITELONG header check");

    // TC-024 100% Lossless check: all 65,536 levels must match exactly
    for (size_t i = 0; i < count; ++i) {
        if (loaded.pixels[i] != static_cast<uint16_t>(i)) {
            TEST_ASSERT(false, "TC-024: Pixel mismatch in 16-bit FITS roundtrip!");
        }
    }

    std::filesystem::remove(test_fits_file);
}

// 2. TIFF 16-bit and PGM 16-bit Lossless I/O
void test_image_writer_tiff_and_pgm() {
    int w = 120, h = 80;
    Frame f = Frame::create_allocated(w, h, 1, 16);
    for (size_t i = 0; i < f.pixel_count(); ++i) {
        f.pixels[i] = static_cast<uint16_t>(i * 6.5);
    }

    // TIFF test
    std::string tiff_path = "test_io.tif";
    auto tiff_w = ImageWriter::write_tiff_16bit(tiff_path, f);
    TEST_ASSERT(tiff_w.is_ok(), "TIFF write should succeed");

    auto tiff_r = ImageWriter::read_tiff_16bit(tiff_path);
    TEST_ASSERT(tiff_r.is_ok(), "TIFF read should succeed");
    Frame loaded_tiff = tiff_r.value();
    TEST_ASSERT(loaded_tiff.pixels[100] == f.pixels[100], "TIFF pixel match");
    std::filesystem::remove(tiff_path);

    // PGM test
    std::string pgm_path = "test_io.pgm";
    auto pgm_w = ImageWriter::write_pgm_16bit(pgm_path, f);
    TEST_ASSERT(pgm_w.is_ok(), "PGM write should succeed");

    auto pgm_r = ImageWriter::read_pgm_16bit(pgm_path);
    TEST_ASSERT(pgm_r.is_ok(), "PGM read should succeed");
    Frame loaded_pgm = pgm_r.value();
    TEST_ASSERT(loaded_pgm.pixels[100] == f.pixels[100], "PGM pixel match");
    std::filesystem::remove(pgm_path);

    // 3-channel RGB FITS and TIFF roundtrip test
    Frame rgb = Frame::create_allocated(64, 48, 3, 16);
    for (size_t i = 0; i < static_cast<size_t>(64 * 48); ++i) {
        rgb.pixels[i * 3 + 0] = static_cast<uint16_t>(1000 + (i % 500) * 10);
        rgb.pixels[i * 3 + 1] = static_cast<uint16_t>(20000 + (i % 500) * 20);
        rgb.pixels[i * 3 + 2] = static_cast<uint16_t>(40000 + (i % 500) * 30);
    }

    std::string rgb_fits_path = "test_rgb.fits";
    FitsMetadata rgb_meta;
    rgb_meta.object = "RGB Hilal";
    auto rf_w = FitsWriter::write_fits_16bit(rgb_fits_path, rgb, rgb_meta);
    TEST_ASSERT(rf_w.is_ok(), "3-channel RGB FITS write should succeed");
    auto rf_r = FitsWriter::read_fits_16bit(rgb_fits_path);
    TEST_ASSERT(rf_r.is_ok(), "3-channel RGB FITS read should succeed");
    TEST_ASSERT(rf_r.value().channels == 3, "3-channel FITS should restore channels=3");
    TEST_ASSERT(rf_r.value().pixels[15 * 3 + 0] == rgb.pixels[15 * 3 + 0], "RGB FITS Red channel match");
    TEST_ASSERT(rf_r.value().pixels[15 * 3 + 1] == rgb.pixels[15 * 3 + 1], "RGB FITS Green channel match");
    TEST_ASSERT(rf_r.value().pixels[15 * 3 + 2] == rgb.pixels[15 * 3 + 2], "RGB FITS Blue channel match");
    std::filesystem::remove(rgb_fits_path);

    std::string rgb_tiff_path = "test_rgb.tif";
    auto rt_w = ImageWriter::write_tiff_16bit(rgb_tiff_path, rgb);
    TEST_ASSERT(rt_w.is_ok(), "3-channel RGB TIFF write should succeed");
    auto rt_r = ImageWriter::read_tiff_16bit(rgb_tiff_path);
    TEST_ASSERT(rt_r.is_ok(), "3-channel RGB TIFF read should succeed");
    TEST_ASSERT(rt_r.value().channels == 3, "3-channel TIFF should restore channels=3");
    TEST_ASSERT(rt_r.value().pixels[25 * 3 + 0] == rgb.pixels[25 * 3 + 0], "RGB TIFF Red channel match");
    TEST_ASSERT(rt_r.value().pixels[25 * 3 + 1] == rgb.pixels[25 * 3 + 1], "RGB TIFF Green channel match");
    TEST_ASSERT(rt_r.value().pixels[25 * 3 + 2] == rgb.pixels[25 * 3 + 2], "RGB TIFF Blue channel match");
    std::filesystem::remove(rgb_tiff_path);

    // PNG & JPEG test
    std::string png_path = "test_rgb.png";
    auto png_res = ImageWriter::write_png(png_path, rgb);
    TEST_ASSERT(png_res.is_ok(), "PNG write should succeed");
    TEST_ASSERT(std::filesystem::exists(png_path) && std::filesystem::file_size(png_path) > 100, "PNG file should exist");
    std::filesystem::remove(png_path);

    std::string jpg_path = "test_rgb.jpg";
    auto jpg_res = ImageWriter::write_jpg(jpg_path, rgb, 92);
    TEST_ASSERT(jpg_res.is_ok(), "JPEG write should succeed");
    TEST_ASSERT(std::filesystem::exists(jpg_path) && std::filesystem::file_size(jpg_path) > 100, "JPEG file should exist");
    std::filesystem::remove(jpg_path);
}

// 3. TC-025 & TC-026: Calibration Dark Clamping and Flat Vignetting Flattening
void test_calibration_dark_and_flat() {
    int w = 100, h = 100;
    size_t count = w * h;

    // Create Dark frame with fixed pedestal 1500
    Frame dark1 = Frame::create_allocated(w, h, 1, 16);
    Frame dark2 = Frame::create_allocated(w, h, 1, 16);
    for (size_t i = 0; i < count; ++i) {
        dark1.pixels[i] = 1500;
        dark2.pixels[i] = 1500;
    }
    // One hot pixel in dark at index 50
    dark1.pixels[50] = 3000;
    dark2.pixels[50] = 3000;

    auto dark_res = CalibrationProcessor::create_master_dark({dark1, dark2});
    TEST_ASSERT(dark_res.is_ok(), "Master dark creation should succeed");
    MasterDark master_dark = dark_res.value();
    TEST_ASSERT(std::abs(master_dark.data[0] - 1500.0f) < 0.01f, "Master dark mean value check");

    // TC-026: Dark subtraction clamping test (dark > pixel must NOT underflow/wrap around)
    CalibrationProcessor cal;
    cal.set_master_dark(master_dark);

    Frame faint_frame = Frame::create_allocated(w, h, 1, 16);
    // Pixel 0 is 1000 (fainter than dark 1500)
    faint_frame.pixels[0] = 1000;
    // Pixel 1 is 2000 (brighter than dark 1500)
    faint_frame.pixels[1] = 2000;

    cal.apply(faint_frame);

    TEST_ASSERT(faint_frame.pixels[0] == 0, "TC-026: Dark > Pixel must clamp to 0, no wrap-around underflow");
    TEST_ASSERT(faint_frame.pixels[1] == 500, "TC-026: Normal dark subtraction check (2000 - 1500 = 500)");

    // TC-025: Master Flat flattening synthetic vignetting (residual deviation <= 2%)
    // Generate synthetic vignetted flat field: intensity falls off from center by 40%
    Frame flat1 = Frame::create_allocated(w, h, 1, 16);
    double cx = w * 0.5, cy = h * 0.5;
    double max_r_sq = cx * cx + cy * cy;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            double r_sq = (x - cx) * (x - cx) + (y - cy) * (y - cy);
            double vig = 1.0 - 0.40 * (r_sq / max_r_sq); // 40% radial drop
            flat1.pixels[y * w + x] = static_cast<uint16_t>(40000.0 * vig);
        }
    }

    auto flat_res = CalibrationProcessor::create_master_flat({flat1});
    TEST_ASSERT(flat_res.is_ok(), "Master flat creation should succeed");
    MasterFlat master_flat = flat_res.value();

    cal.clear_dark();
    cal.set_master_flat(master_flat);

    // Apply flat to an identical vignetted light frame (ideal flat field test)
    Frame light_frame = Frame::create_allocated(w, h, 1, 16);
    for (size_t i = 0; i < count; ++i) {
        light_frame.pixels[i] = flat1.pixels[i];
    }

    cal.apply(light_frame);

    // Measure residual deviation across the image
    double sum = 0.0;
    for (size_t i = 0; i < count; ++i) {
        sum += light_frame.pixels[i];
    }
    double mean_calibrated = sum / count;

    double max_dev_percent = 0.0;
    for (size_t i = 0; i < count; ++i) {
        double dev = std::abs(light_frame.pixels[i] - mean_calibrated) / mean_calibrated;
        max_dev_percent = std::max(max_dev_percent, dev * 100.0);
    }

    std::cout << "    [Flat Calibration] Max vignetting deviation after flat: " << max_dev_percent << "%\n";
    TEST_ASSERT(max_dev_percent <= 2.0, "TC-025: Residual vignetting deviation must be <= 2%");
}

// 4. TC-028 & TC-029: Stacker Sigma-Clip Outlier Rejection, Sum, and RAM Limit
void test_stacker_sigma_clip_sum_and_ram_limit() {
    int w = 50, h = 50;
    size_t count = w * h;

    // TC-028: Sigma-Clip test: 10 frames with background = 1000.
    // In frame 5, pixel 100 is an extreme hot-pixel outlier = 65535.
    StackerConfig sc;
    sc.method = StackMethod::SigmaClip;
    sc.max_frames = 10;
    sc.k_sigma = 2.0f;
    LiveStacker stacker(sc);

    Frame result;
    for (int f = 0; f < 10; ++f) {
        Frame frame = Frame::create_allocated(w, h, 1, 16);
        for (size_t i = 0; i < count; ++i) {
            frame.pixels[i] = 1000;
        }
        if (f == 5) {
            frame.pixels[100] = 65535; // Injected hot pixel outlier
        }
        result = stacker.process_frame(frame);
    }

    // The outlier at pixel 100 should be rejected by sigma-clip, so result[100] == 1000
    uint16_t outlier_pixel_res = result.pixels[100];
    std::cout << "    [Sigma-Clip Check] Outlier pixel 100 stacked value: " << outlier_pixel_res << "\n";
    TEST_ASSERT(outlier_pixel_res == 1000, "TC-028: Sigma-Clip must reject 65535 outlier!");

    // TC-028 Sum test: Sum must NOT divide by N
    sc.method = StackMethod::Sum;
    sc.max_frames = 4;
    LiveStacker sum_stacker(sc);
    Frame sum_res;
    for (int f = 0; f < 4; ++f) {
        Frame frame = Frame::create_allocated(w, h, 1, 16);
        for (size_t i = 0; i < count; ++i) {
            frame.pixels[i] = 2000;
        }
        sum_res = sum_stacker.process_frame(frame);
    }
    // 4 frames x 2000 = 8000 (mean would be 2000)
    TEST_ASSERT(sum_res.pixels[0] == 8000, "TC-028: Sum stacker must accumulate without dividing by N");

    // TC-029: Stacker RAM limit enforcement
    // Set RAM limit to very small: 1 MB. Each frame is 5000 bytes.
    // Max frames = 1000, but 1 MB / 5000 ~ 200 frames limit.
    StackerConfig ram_sc;
    ram_sc.stack_ram_mb = 1; // 1 MB
    ram_sc.max_frames = 500;
    LiveStacker ram_stacker(ram_sc);

    for (int f = 0; f < 300; ++f) {
        ram_stacker.process_frame(Frame::create_allocated(w, h, 1, 16));
    }
    StackerStatus st = ram_stacker.status();
    TEST_ASSERT(st.current_ram_bytes <= 1 * 1024 * 1024, "TC-029: Stacker memory must respect stack_ram_mb limit");
    TEST_ASSERT(st.ram_limit_reached, "TC-029: Stacker must report ram_limit_reached status");
}

// 5. TC-022: Histogram, 16-to-8 Bit Stretch, and Anti-Flicker EMA
void test_stretch_histogram_and_ema() {
    int w = 100, h = 100;
    Frame f = Frame::create_allocated(w, h, 1, 16);
    for (size_t i = 0; i < f.pixel_count(); ++i) {
        f.pixels[i] = static_cast<uint16_t>(1000 + i * 5);
    }

    Histogram hist = ImageStretcher::compute_histogram(f);
    TEST_ASSERT(hist.total_pixels == 10000, "Histogram total pixels check");
    TEST_ASSERT(hist.min_val == 1000, "Histogram min check");
    TEST_ASSERT(hist.bins256[1000 >> 8] > 0, "256-bin histogram check");

    StretchConfig sc;
    sc.auto_stretch = true;
    sc.ema_alpha = 0.2f;
    ImageStretcher stretcher(sc);

    auto prev1 = stretcher.convert_to_preview_8bit(f);
    TEST_ASSERT(!prev1.empty(), "Preview output should not be empty");
    TEST_ASSERT(prev1[0] <= 10, "Stretched shadow should be dark");
    TEST_ASSERT(prev1.back() >= 240, "Stretched highlight should be bright");

    // TC-022: Consecutive identical frames with EMA should produce stable, identical stretch points
    auto prev2 = stretcher.convert_to_preview_8bit(f);
    TEST_ASSERT(prev2[0] == prev1[0], "TC-022: Static scene with EMA must not flicker");
}

// 6. TC-066: Site Validation and Indonesian Presets
void test_astro_site_validation_and_presets() {
    auto site_valid = Site::create("Semarang", -6.9932, 110.4203, 25.0);
    TEST_ASSERT(site_valid.is_ok(), "Valid Semarang site should succeed");

    // TC-066: Invalid latitude and longitude rejected
    auto site_bad_lat = Site::create("Bad Lat", 95.0, 110.0);
    TEST_ASSERT(site_bad_lat.is_err(), "TC-066: Latitude > 90 must be rejected");

    auto site_bad_lon = Site::create("Bad Lon", -6.0, 200.0);
    TEST_ASSERT(site_bad_lon.is_err(), "TC-066: Longitude > 180 must be rejected");

    const auto& presets = Site::indonesian_presets();
    TEST_ASSERT(presets.size() >= 7, "Indonesian presets should have >= 7 locations");
    TEST_ASSERT(presets[0].name == "Jakarta", "Preset 0 is Jakarta");
}

// 7. TC-060, TC-061, TC-062: Ephemeris Calculations (Sunset, Moonset, Ijtima)
void test_ephemeris_events_and_phenomena() {
    Site jkt = Site::indonesian_presets()[0]; // Jakarta

    // Test date: 2026-03-19 (Ramadhan / Syawal 1447 H period)
    auto times = EphemerisCalculator::compute_times(jkt, 2026, 3, 19);

    std::string sunset_iso = Clock::to_iso8601(times.sunset_utc);
    std::string moonset_iso = Clock::to_iso8601(times.moonset_utc);
    std::string ijtima_iso = Clock::to_iso8601(times.ijtima_utc);

    std::cout << "    [Ephemeris 2026-03-19 Jakarta] Sunset UTC: " << sunset_iso
              << ", Moonset UTC: " << moonset_iso
              << ", Ijtima UTC: " << ijtima_iso << "\n";

    TEST_ASSERT(!sunset_iso.empty(), "Sunset time calculated");
    TEST_ASSERT(!moonset_iso.empty(), "Moonset time calculated");
    TEST_ASSERT(!ijtima_iso.empty(), "Ijtima time calculated");

    // In Jakarta, sunset is around 11:00 to 11:15 UTC (18:00 to 18:15 WIB)
    // Extract hour from UTC string: "YYYY-MM-DDTHH:MM:SS"
    int sunset_hour_utc = std::stoi(sunset_iso.substr(11, 2));
    TEST_ASSERT(sunset_hour_utc == 10 || sunset_hour_utc == 11, "TC-062: Sunset UTC hour in Jakarta should be ~11:00 UTC");

    // Phenomena at sunset
    MoonPhenomena p = EphemerisCalculator::compute_phenomena(jkt, times.sunset_utc);
    TEST_ASSERT(p.sun.altitude_deg < 0.5, "Sun altitude at sunset should be close to horizon");
    TEST_ASSERT(p.elongation_deg >= 0.0 && p.elongation_deg <= 180.0, "Elongation range check");
}

// 8. TC-063: MABIMS Baru Boundary Test (3.00 deg and 6.40 deg)
void test_criteria_mabims_baru_boundaries() {
    // TC-063:
    // Alt 2.99 deg, Elong 6.40 deg -> FAIL
    auto r1 = HilalCriteria::evaluate_mabims_baru(2.99, 6.40);
    TEST_ASSERT(!r1.is_visible, "TC-063: Alt 2.99 deg must fail MABIMS Baru");
    TEST_ASSERT(r1.zone == "Fail", "Zone should be Fail");

    // Alt 3.00 deg, Elong 6.39 deg -> FAIL
    auto r2 = HilalCriteria::evaluate_mabims_baru(3.00, 6.39);
    TEST_ASSERT(!r2.is_visible, "TC-063: Elong 6.39 deg must fail MABIMS Baru");

    // Alt 3.00 deg, Elong 6.40 deg -> PASS
    auto r3 = HilalCriteria::evaluate_mabims_baru(3.00, 6.40);
    TEST_ASSERT(r3.is_visible, "TC-063: Alt 3.00 deg and Elong 6.40 deg must pass MABIMS Baru");
    TEST_ASSERT(r3.zone == "Pass", "Zone should be Pass");

    // Alt 3.50 deg, Elong 6.80 deg -> PASS
    auto r4 = HilalCriteria::evaluate_mabims_baru(3.50, 6.80);
    TEST_ASSERT(r4.is_visible, "Alt 3.50 deg and Elong 6.80 deg must pass MABIMS Baru");
}

// 9. TC-064: Odeh Criteria Polynomial Classification
void test_criteria_odeh_polynomial() {
    // Danjon limit: W <= 0.05' -> Zone D
    auto r_danjon = HilalCriteria::evaluate_odeh(10.0, 0.04);
    TEST_ASSERT(!r_danjon.is_visible, "TC-064: W <= 0.05' must fail Danjon limit");
    TEST_ASSERT(r_danjon.zone == "D", "Zone should be D");

    // W = 0.5', ARCV = 12.0 deg -> Very high V -> Zone A (Naked eye)
    auto r_zoneA = HilalCriteria::evaluate_odeh(12.0, 0.5);
    TEST_ASSERT(r_zoneA.is_visible, "High ARCV and W must pass Odeh");
    TEST_ASSERT(r_zoneA.zone == "A", "Zone should be A");

    // W = 0.2', ARCV = 7.0 deg -> Zone B or C
    auto r_zoneBC = HilalCriteria::evaluate_odeh(7.0, 0.2);
    TEST_ASSERT(r_zoneBC.is_visible, "Moderate ARCV and W should pass optical aid");
    TEST_ASSERT(r_zoneBC.zone == "B" || r_zoneBC.zone == "C", "Zone should be B or C");
}

// 10. Hijri Calendar Conversion and Indonesian Formatting
void test_hijri_calendar() {
    // Test conversion for 2026-11-10 (Jumadil Akhir 1448 H deadline!)
    HijriDate hd = HijriCalendar::from_gregorian(2026, 11, 10);
    std::cout << "    [Hijri Conversion 2026-11-10] Result: " << hd.to_string_id() << "\n";
    TEST_ASSERT(hd.year == 1448, "Hijri year in Nov 2026 should be 1448 H");
    TEST_ASSERT(hd.month == 5 || hd.month == 6, "Hijri month in Nov 2026 should be Jumadil Ula or Jumadil Akhir");
    TEST_ASSERT(!hd.month_name_id().empty(), "Indonesian month name should not be empty");
}

// 11. Offline Batch Stacking
void test_offline_batch_stacking() {
    std::vector<Frame> frames;
    for (int i = 1; i <= 4; ++i) {
        Frame f = Frame::create_allocated(10, 10, 1, 16);
        std::fill_n(f.pixels.get(), 100, static_cast<uint16_t>(i * 1000));
        frames.push_back(std::move(f));
    }

    auto res_mean = LiveStacker::stack_frames(frames, StackMethod::Mean);
    TEST_ASSERT(res_mean.is_ok(), "Offline mean stacking should succeed");
    TEST_ASSERT(res_mean.value().pixels[0] == 2500, "Mean of 1000, 2000, 3000, 4000 should be 2500");

    auto res_median = LiveStacker::stack_frames(frames, StackMethod::Median);
    TEST_ASSERT(res_median.is_ok(), "Offline median stacking should succeed");
    TEST_ASSERT(res_median.value().pixels[0] >= 2000 && res_median.value().pixels[0] <= 3000, "Median should be in valid range");
}

// 12. AVI Video Writer
void test_avi_video_writer() {
    std::string test_path = "test_scratch/test_video.avi";
    std::filesystem::remove(test_path);

    io::AviWriter writer;
    auto open_res = writer.open(test_path, 32, 32, 30.0);
    TEST_ASSERT(open_res.is_ok(), "AviWriter open should succeed");
    TEST_ASSERT(writer.is_open(), "AviWriter should be marked open");

    Frame f = Frame::create_allocated(32, 32, 3, 16);
    std::fill_n(f.pixels.get(), 32 * 32 * 3, static_cast<uint16_t>(32000));

    for (int i = 0; i < 5; ++i) {
        auto w_res = writer.write_frame(f);
        TEST_ASSERT(w_res.is_ok(), "Writing frame to AVI should succeed");
    }

    TEST_ASSERT(writer.frame_count() == 5, "Frame count should be 5");
    writer.close();
    TEST_ASSERT(!writer.is_open(), "AviWriter should be closed");

    TEST_ASSERT(std::filesystem::exists(test_path), "AVI file should exist on disk");
    TEST_ASSERT(std::filesystem::file_size(test_path) > 1000, "AVI file size should be > 1KB");

    // Clean up
    std::filesystem::remove(test_path);
}

// 13. 2D FFT Phase Correlation Motion Alignment
void test_phase_correlation_alignment() {
    auto make_spot_frame = [](int cx, int cy) {
        Frame f = Frame::create_allocated(32, 32, 1, 16);
        std::fill_n(f.pixels.get(), 32 * 32, static_cast<uint16_t>(100));
        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                int x = std::clamp(cx + dx, 0, 31);
                int y = std::clamp(cy + dy, 0, 31);
                double dist2 = dx * dx + dy * dy;
                uint16_t val = static_cast<uint16_t>(50000.0 * std::exp(-dist2 / 2.0));
                f.pixels[y * 32 + x] = std::max(f.pixels[y * 32 + x], val);
            }
        }
        return f;
    };

    Frame ref = make_spot_frame(14, 15);
    Frame shifted = make_spot_frame(18, 12); // shifted by dx = +4, dy = -3

    StackerConfig cfg;
    cfg.method = StackMethod::Mean;
    cfg.align_enabled = true;
    LiveStacker stacker(cfg);

    stacker.process_frame(ref);
    Frame aligned_stack = stacker.process_frame(shifted);
    auto st = stacker.status();

    TEST_ASSERT(st.align_offset_x == 4 && st.align_offset_y == -3, "Phase correlation should recover exact shift (+4, -3)");
    TEST_ASSERT(aligned_stack.pixels[15 * 32 + 14] > 40000, "Aligned stacked peak at (14,15) should remain sharp (>40000)");
}

// 14. MP4 Video Writer
void test_mp4_video_writer() {
    std::string test_path = "test_scratch/test_video.mp4";
    std::filesystem::remove(test_path);

    io::Mp4Writer writer;
    auto open_res = writer.open(test_path, 32, 32, 30.0);
    TEST_ASSERT(open_res.is_ok(), "Mp4Writer open should succeed");
    TEST_ASSERT(writer.is_open(), "Mp4Writer should be marked open");

    Frame f = Frame::create_allocated(32, 32, 3, 16);
    std::fill_n(f.pixels.get(), 32 * 32 * 3, static_cast<uint16_t>(28000));

    for (int i = 0; i < 5; ++i) {
        auto w_res = writer.write_frame(f);
        TEST_ASSERT(w_res.is_ok(), "Writing frame to MP4 should succeed");
    }

    TEST_ASSERT(writer.frame_count() == 5, "MP4 frame count should be 5");
    writer.close();
    TEST_ASSERT(!writer.is_open(), "Mp4Writer should be closed");

    TEST_ASSERT(std::filesystem::exists(test_path), "MP4 file should exist on disk");
    TEST_ASSERT(std::filesystem::file_size(test_path) > 1000, "MP4 file size should be > 1KB");

    std::filesystem::remove(test_path);
}

// 15. Astrometric Plate Solver (Star Centroid Detection + Geometric Triangle Matching)
void test_astrometric_plate_solver() {
    PlateSolveConfig cfg;
    cfg.hint_ra_deg = 83.8221;
    cfg.hint_dec_deg = -5.3911;
    Frame star_field = PlateSolver::generate_synthetic_star_field(1280, 720, cfg);
    auto stars = PlateSolver::detect_stars(star_field, 4.0);
    TEST_ASSERT(stars.size() >= 4, "PlateSolver should detect >= 4 stars in synthetic Orion field");

    auto sol = PlateSolver::solve(star_field, cfg);
    TEST_ASSERT(sol.solved, "PlateSolver should solve synthetic Orion star field");
    TEST_ASSERT(std::abs(sol.center_ra_deg - 83.8221) < 2.0, "Solved RA should be within 2.0 deg of Orion center");
    TEST_ASSERT(std::abs(sol.center_dec_deg - (-5.3911)) < 2.0, "Solved Dec should be within 2.0 deg of Orion center");
    TEST_ASSERT(!sol.format_ra_hms().empty(), "RA HMS formatting check");
    TEST_ASSERT(!sol.format_dec_dms().empty(), "Dec DMS formatting check");
}

int main() {
    std::cout << "========================================================\n";
    std::cout << " Sabit Batch 2: Imaging & Astro Unit Test Suite\n";
    std::cout << "========================================================\n";

    RUN_TEST(test_fits_16bit_lossless_roundtrip_and_headers);
    RUN_TEST(test_image_writer_tiff_and_pgm);
    RUN_TEST(test_calibration_dark_and_flat);
    RUN_TEST(test_stacker_sigma_clip_sum_and_ram_limit);
    RUN_TEST(test_stretch_histogram_and_ema);
    RUN_TEST(test_astro_site_validation_and_presets);
    RUN_TEST(test_ephemeris_events_and_phenomena);
    RUN_TEST(test_criteria_mabims_baru_boundaries);
    RUN_TEST(test_criteria_odeh_polynomial);
    RUN_TEST(test_hijri_calendar);
    RUN_TEST(test_offline_batch_stacking);
    RUN_TEST(test_avi_video_writer);
    RUN_TEST(test_phase_correlation_alignment);
    RUN_TEST(test_mp4_video_writer);
    RUN_TEST(test_astrometric_plate_solver);

    std::cout << "========================================================\n";
    std::cout << " Summary: " << g_tests_passed << " passed, " << g_tests_failed << " failed.\n";
    std::cout << "========================================================\n";

    return (g_tests_failed == 0) ? 0 : 1;
}
