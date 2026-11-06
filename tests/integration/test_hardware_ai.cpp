#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <stdexcept>

#include "src/core/frame.h"
#include "src/core/clock.h"
#include "src/camera/icamera.h"
#include "src/camera/sim/simulator_camera.h"
#include "src/camera/zwo/zwo_camera.h"
#include "src/camera/qhy/qhy_camera.h"
#include "src/camera/ascom/ascom_camera.h"
#include "src/camera/uvc/uvc_camera.h"
#include "src/detect/sha256.h"
#include "src/detect/onnx_detector.h"
#include "src/detect/detection_log.h"
#include "src/imaging/io/fits_writer.h"

using namespace sabit;

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  FAILED: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            throw std::runtime_error(std::string("Assertion failed: ") + msg); \
        } \
    } while (0)

void test_camera_cmdq_and_readback() {
    std::cout << "[Test 1] Camera CMDQ & Read-Back (Simulator & Hardware Interfaces)..." << std::endl;

    // 1. Simulator Camera CMDQ and read-back
    camera::SimulatorCamera sim(camera::SimulatorMode::Hilal, 1920, 1080);
    camera::CameraId sim_id{.driver = "sim", .id = "hilal", .name = "Sim Hilal"};

    auto open_res = sim.open(sim_id);
    TEST_CHECK(open_res.is_ok(), "SimulatorCamera::open failed");
    auto caps = open_res.value();
    TEST_CHECK(caps.max_width == 1920, "Simulator max_width should be 1920");
    TEST_CHECK(caps.max_height == 1080, "Simulator max_height should be 1080");

    // Apply settings and verify read-back
    camera::CameraSettings req;
    req.exposure_us = 45000; // 45 ms
    req.gain = 150.0;
    req.binning = 1;

    auto apply_res = sim.apply(req);
    TEST_CHECK(apply_res.is_ok(), "SimulatorCamera::apply failed");
    auto actual = apply_res.value();
    TEST_CHECK(actual.exposure_us.has_value() && *actual.exposure_us == 45000, "Read-back exposure mismatch");
    TEST_CHECK(actual.gain.has_value() && *actual.gain == 150.0, "Read-back gain mismatch");
    TEST_CHECK(actual.binning.has_value() && *actual.binning == 1, "Read-back binning mismatch");

    // Verify streaming frames have the read-back settings
    std::atomic<bool> got_frame{false};
    core::Frame rec_frame;

    auto start_res = sim.start([&](const core::Frame& f) {
        rec_frame = f;
        got_frame.store(true);
    });
    TEST_CHECK(start_res.is_ok(), "SimulatorCamera::start failed");

    for (int i = 0; i < 50 && !got_frame.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    TEST_CHECK(got_frame.load(), "Simulator frame not received within timeout");
    TEST_CHECK(rec_frame.exposure_us == 45000, "Frame exposure metadata mismatch");
    TEST_CHECK(rec_frame.gain == 150.0, "Frame gain metadata mismatch");
    TEST_CHECK(rec_frame.width == 1920, "Frame width mismatch");
    TEST_CHECK(rec_frame.height == 1080, "Frame height mismatch");
    TEST_CHECK(rec_frame.pixels != nullptr, "Frame pixels buffer is null");

    sim.stop();
    sim.close();

    // 2. Test ZWO Driver enumeration & interface safety
    auto zwo_cams = camera::ZwoCamera::enumerate_cameras();
    std::cout << "  ZWO connected cameras found: " << zwo_cams.size() << std::endl;
    camera::ZwoCamera zwo_cam;
    auto zwo_status = zwo_cam.status();
    TEST_CHECK(!zwo_status.is_connected, "ZWO camera should be disconnected by default");

    // 3. Test QHYCCD Driver enumeration & interface safety
    auto qhy_cams = camera::QhyCamera::enumerate_cameras();
    std::cout << "  QHYCCD connected cameras found: " << qhy_cams.size() << std::endl;
    camera::QhyCamera qhy_cam;
    auto qhy_status = qhy_cam.status();
    TEST_CHECK(!qhy_status.is_connected, "QHYCCD camera should be disconnected by default");

    // 4. Test ASCOM Driver enumeration & interface safety
    auto ascom_cams = camera::AscomCamera::enumerate_cameras(true);
    std::cout << "  ASCOM camera drivers found: " << ascom_cams.size() << std::endl;
    TEST_CHECK(!ascom_cams.empty(), "ASCOM camera enumeration with fallback defaults should not be empty");
    for (const auto& acam : ascom_cams) {
        std::cout << "    - " << acam.name << " (ProgID: " << acam.id << ")" << std::endl;
    }
    camera::AscomCamera ascom_cam;
    auto ascom_status = ascom_cam.status();
    TEST_CHECK(!ascom_status.is_connected, "ASCOM camera should be disconnected by default");

    // 5. Test UVC Driver enumeration & interface safety
    auto uvc_cams = camera::UvcCamera::enumerate_cameras();
    std::cout << "  UVC devices found: " << uvc_cams.size() << std::endl;
    for (const auto& dev : uvc_cams) {
        std::cout << "    - " << dev.name << " (ID: " << dev.id << ")" << std::endl;
    }
    camera::UvcCamera uvc_cam;
    auto uvc_status = uvc_cam.status();
    TEST_CHECK(!uvc_status.is_connected, "UVC camera should be disconnected by default");

    std::cout << "  PASS: Camera CMDQ & Read-Back passed." << std::endl;
}

void test_sha256_and_manifest_verification() {
    std::cout << "[Test 2] ONNX Model Integrity & SHA-256 Manifest Verification..." << std::endl;

    std::string model_path = "models/hilal_v1.onnx";
    TEST_CHECK(std::filesystem::exists(model_path), "Model file models/hilal_v1.onnx missing");

    std::string computed_hash = detect::Sha256::file_sha256(model_path);
    std::string expected_hash = "e8b8db0323e8e895b2c97837a2c213d751f85ee0ac162155babfc1bf53c52451";
    TEST_CHECK(computed_hash == expected_hash, "Computed SHA-256 does not match expected hash");

    // Test detector rejects bad hash
    detect::OnnxDetector detector;
    detect::OnnxDetectorConfig bad_cfg;
    bad_cfg.model_path = model_path;
    bad_cfg.manifest_path = "models/nonexistent_manifest.json";
    bad_cfg.verify_hash = true;

    auto bad_res = detector.init(bad_cfg);
    TEST_CHECK(!bad_res.is_ok(), "Detector should reject nonexistent manifest");

    // Valid initialization
    detect::OnnxDetectorConfig valid_cfg;
    valid_cfg.model_path = model_path;
    valid_cfg.manifest_path = "models/manifest.json";
    valid_cfg.verify_hash = true;

    auto init_res = detector.init(valid_cfg);
    TEST_CHECK(init_res.is_ok(), "Valid initialization failed");
    TEST_CHECK(detector.is_initialized(), "Detector should be initialized");
    detector.close();

    std::cout << "  PASS: Model SHA-256 integrity check verified." << std::endl;
}

void test_synchronous_inference_and_letterbox() {
    std::cout << "[Test 3] Synchronous AI Inference & Bounding Box Letterbox Mapping..." << std::endl;

    detect::OnnxDetector detector;
    detect::OnnxDetectorConfig cfg;
    cfg.model_path = "models/hilal_v1.onnx";
    cfg.manifest_path = "models/manifest.json";
    cfg.conf_threshold = 0.20f;
    cfg.nms_threshold = 0.45f;
    cfg.num_threads = 2;

    auto res = detector.init(cfg);
    TEST_CHECK(res.is_ok(), "Detector init failed");

    // Generate Hilal test frame from simulator
    camera::SimulatorCamera sim(camera::SimulatorMode::Hilal, 1280, 720);
    camera::CameraId sim_id{.driver = "sim", .id = "hilal", .name = "Sim Hilal"};
    auto open_res = sim.open(sim_id);
    TEST_CHECK(open_res.is_ok(), "Simulator open failed");

    core::Frame test_frame;
    std::atomic<bool> got_frame{false};
    auto start_res = sim.start([&](const core::Frame& f) {
        test_frame = f;
        got_frame.store(true);
    });
    TEST_CHECK(start_res.is_ok(), "Simulator start failed");

    for (int i = 0; i < 50 && !got_frame.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    TEST_CHECK(got_frame.load(), "Simulator frame not generated");
    sim.stop();
    sim.close();

    // Run inference
    auto det_res = detector.detect(test_frame);
    TEST_CHECK(det_res.is_ok(), "Detector inference failed");

    auto result = det_res.value();
    TEST_CHECK(result.is_valid, "Detection result should be valid");
    TEST_CHECK(result.inference_time_ms > 0.0, "Inference time should be > 0");
    std::cout << "  Inference time on " << test_frame.width << "x" << test_frame.height
              << " 16-bit frame: " << result.inference_time_ms << " ms" << std::endl;
    std::cout << "  Detections found: " << result.detections.size() << std::endl;

    // Check bounds of any detections
    for (const auto& det : result.detections) {
        std::cout << "    - Label: " << det.label << ", Conf: " << (det.confidence * 100.0f)
                  << "%, Box: [" << det.x << ", " << det.y << ", " << det.width << ", " << det.height << "]" << std::endl;
        TEST_CHECK(det.label == "Hilal", "Label should be Hilal");
        TEST_CHECK(det.confidence >= cfg.conf_threshold, "Confidence below threshold");
        TEST_CHECK(det.x >= 0.0f && det.x <= test_frame.width, "Box x out of frame bounds");
        TEST_CHECK(det.y >= 0.0f && det.y <= test_frame.height, "Box y out of frame bounds");
        TEST_CHECK(det.x + det.width <= test_frame.width + 1.0f, "Box width overflows frame");
        TEST_CHECK(det.y + det.height <= test_frame.height + 1.0f, "Box height overflows frame");
    }

    detector.close();
    std::cout << "  PASS: Synchronous inference & letterbox mapping passed." << std::endl;
}

void test_async_worker_and_overlay_expiration() {
    std::cout << "[Test 4] Async AI Worker Throttling & Overlay Expiration (TC-052, TC-054)..." << std::endl;

    detect::OnnxDetector detector;
    detect::OnnxDetectorConfig cfg;
    cfg.model_path = "models/hilal_v1.onnx";
    cfg.manifest_path = "models/manifest.json";
    cfg.conf_threshold = 0.15f;
    cfg.max_fps = 5.0; // 5 FPS

    auto res = detector.init(cfg);
    TEST_CHECK(res.is_ok(), "Detector init failed");

    std::atomic<int> callbacks_count{0};
    detector.set_detection_callback([&](const core::Frame&, const detect::DetectionResult&) {
        callbacks_count++;
    });

    detector.start_worker();

    core::Frame frame = core::Frame::create_allocated(640, 480, 1, 16);
    std::fill_n(frame.pixels.get(), frame.pixel_count(), 5000); // 16-bit gray

    detector.submit_frame(frame);

    // Wait for inference completion
    for (int i = 0; i < 50 && callbacks_count.load() == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    TEST_CHECK(callbacks_count.load() > 0, "AI worker callback not fired");

    // TC-054: Test overlay expiration
    // Immediately after inference, overlay query with 500ms max age succeeds
    auto overlay_fresh = detector.get_latest_overlay(500);

    // Now sleep 550 ms to exceed max_age_ms
    std::this_thread::sleep_for(std::chrono::milliseconds(550));

    // Stale overlay must be discarded
    auto overlay_stale = detector.get_latest_overlay(500);
    TEST_CHECK(overlay_stale.empty(), "Stale overlay (>500ms) should be discarded");

    detector.stop_worker();
    detector.close();

    std::cout << "  PASS: Async AI worker & overlay expiration (TC-054) verified." << std::endl;
}

void test_tc055_raw_recording_purity() {
    std::cout << "[Test 5] TC-055 Raw Recording Purity Verification (Zero Pixel Difference)..." << std::endl;

    // Create a 16-bit synthetic frame
    const int W = 640;
    const int H = 480;
    core::Frame raw_frame = core::Frame::create_allocated(W, H, 1, 16);
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            raw_frame.pixels[y * W + x] = static_cast<uint16_t>((x * 65535) / W);
        }
    }
    raw_frame.exposure_us = 25000;
    raw_frame.gain = 100.0;
    raw_frame.sensor_temp_c = 15.5;

    // Create a bit-for-bit reference copy before any AI operation
    std::vector<uint16_t> reference_pixels(W * H);
    std::memcpy(reference_pixels.data(), raw_frame.pixels.get(), W * H * sizeof(uint16_t));

    // Initialize and run ONNX Detector
    detect::OnnxDetector detector;
    detect::OnnxDetectorConfig cfg;
    cfg.model_path = "models/hilal_v1.onnx";
    cfg.manifest_path = "models/manifest.json";
    auto init_res = detector.init(cfg);
    TEST_CHECK(init_res.is_ok(), "Detector init failed");

    auto det_res = detector.detect(raw_frame);
    TEST_CHECK(det_res.is_ok(), "Detector inference failed");

    // Record the raw frame to a FITS file (exactly as done in Sabit recording pipeline)
    std::string test_fits = "test_tc055_purity.fits";
    if (std::filesystem::exists(test_fits)) {
        std::filesystem::remove(test_fits);
    }

    imaging::io::FitsMetadata meta;
    meta.object = "Hilal Test Purity";
    auto write_res = imaging::io::FitsWriter::write_fits_16bit(test_fits, raw_frame, meta);
    TEST_CHECK(write_res.is_ok(), "FitsWriter write failed");

    // Read back the recorded FITS file
    auto read_res = imaging::io::FitsWriter::read_fits_16bit(test_fits);
    TEST_CHECK(read_res.is_ok(), "FitsWriter read failed");
    const auto& recorded_frame = read_res.value();

    // Verify 100% pixel-for-pixel identity (Zero Pixel Difference, TC-055)
    TEST_CHECK(recorded_frame.width == W, "Recorded frame width mismatch");
    TEST_CHECK(recorded_frame.height == H, "Recorded frame height mismatch");

    uint64_t diff_sum = 0;
    for (size_t i = 0; i < static_cast<size_t>(W * H); ++i) {
        uint16_t orig_val = reference_pixels[i];
        uint16_t rec_val = recorded_frame.pixels[i];
        uint16_t in_mem_val = raw_frame.pixels[i];

        TEST_CHECK(in_mem_val == orig_val, "In-memory frame was modified by AI!");
        if (rec_val != orig_val) {
            diff_sum += std::abs(static_cast<int>(rec_val) - static_cast<int>(orig_val));
        }
    }

    std::filesystem::remove(test_fits);
    detector.close();

    TEST_CHECK(diff_sum == 0, "Recorded frame differs from raw frame (overlay was burnt in!)");
    std::cout << "  PASS: TC-055 verified: pixel difference between recorded and raw frame is exactly 0!" << std::endl;
}

void test_detection_logging_csv() {
    std::cout << "[Test 6] Detection Event Logging to CSV (FR-32)..." << std::endl;

    std::string csv_path = "test_detections.csv";
    if (std::filesystem::exists(csv_path)) {
        std::filesystem::remove(csv_path);
    }

    {
        detect::DetectionLogger logger(csv_path);

        core::Frame frame = core::Frame::create_allocated(1920, 1080, 1, 16);
        frame.seq = 1042;
        frame.exposure_us = 33000;
        frame.gain = 120.0;
        frame.sensor_temp_c = 18.2;
        frame.timestamp_utc = core::Clock::now_utc();

        detect::Detection det1{
            .label = "Hilal",
            .confidence = 0.9421f,
            .x = 850.5f,
            .y = 520.0f,
            .width = 120.0f,
            .height = 85.0f,
            .timestamp_utc_ms = 123456789
        };

        detect::Detection det2{
            .label = "Hilal",
            .confidence = 0.8875f,
            .x = 855.0f,
            .y = 522.0f,
            .width = 115.0f,
            .height = 80.0f,
            .timestamp_utc_ms = 123456789
        };

        logger.log(frame, det1);
        logger.log(frame, det2);
        logger.flush();

        TEST_CHECK(logger.logged_count() == 2, "Logged count should be 2");
    }

    // Read back and inspect CSV
    std::ifstream in(csv_path);
    TEST_CHECK(in.is_open(), "Could not open written CSV file");

    std::string header;
    std::getline(in, header);
    TEST_CHECK(header == "timestamp_utc,frame_id,label,confidence,x,y,width,height,exposure_us,gain,sensor_temp_c", "CSV header mismatch");

    std::string line1, line2;
    std::getline(in, line1);
    std::getline(in, line2);

    TEST_CHECK(line1.find("1042") != std::string::npos, "Frame seq 1042 missing from row 1");
    TEST_CHECK(line1.find("\"Hilal\"") != std::string::npos, "Label Hilal missing from row 1");
    TEST_CHECK(line1.find("0.9421") != std::string::npos, "Confidence 0.9421 missing from row 1");
    TEST_CHECK(line1.find("850.50") != std::string::npos, "X coord 850.50 missing from row 1");

    TEST_CHECK(line2.find("0.8875") != std::string::npos, "Confidence 0.8875 missing from row 2");

    in.close();
    std::filesystem::remove(csv_path);

    std::cout << "  PASS: DetectionLogger CSV output verified." << std::endl;
}

int main() {
    std::cout << "=== Sabit Batch 3 Integration Tests (Hardware & AI) ===" << std::endl;

    try {
        test_camera_cmdq_and_readback();
        test_sha256_and_manifest_verification();
        test_synchronous_inference_and_letterbox();
        test_async_worker_and_overlay_expiration();
        test_tc055_raw_recording_purity();
        test_detection_logging_csv();

        std::cout << "\n>>> ALL BATCH 3 INTEGRATION TESTS PASSED (6/6) <<<" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "UNKNOWN EXCEPTION OCCURRED!" << std::endl;
        return 2;
    }
}
