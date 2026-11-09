#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cmath>

#include <QApplication>
#include <QWidget>

#include "src/core/frame.h"
#include "src/core/clock.h"
#include "src/app/session.h"
#include "src/app/app_controller.h"
#include "src/ui/theme/theme_manager.h"
#include "src/ui/i18n/translator.h"
#include "src/ui/fluent/fluent_badge.h"
#include "src/ui/widgets/marquee_label.h"
#include "src/ui/main_window.h"

using namespace sabit;

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  FAILED: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            throw std::runtime_error(std::string("Assertion failed: ") + msg); \
        } \
    } while (0)

void test_session_lifecycle() {
    std::cout << "[Test 1] Session Management & Atomic Manifest Lifecycle..." << std::endl;

    std::string test_dir = "Captures_Test";
    if (std::filesystem::exists(test_dir)) {
        std::filesystem::remove_all(test_dir);
    }

    app::Session session;
    camera::CameraCaps caps;
    caps.driver = "sim";
    caps.model = "Hilal Sim";
    caps.serial = "SIM-001";

    bool ok = session.start(test_dir, "Semarang", "Hilal", caps, "Tester");
    TEST_CHECK(ok, "Session start failed");
    TEST_CHECK(session.is_active(), "Session should be active");

    session.log_event("Camera initialized.");
    session.update_counts(100, 50, 5);
    session.write_manifest();

    // Verify manifest JSON exists and is valid
    std::string manifest_path = session.get_manifest_filepath();
    TEST_CHECK(std::filesystem::exists(manifest_path), "Manifest file missing");

    std::ifstream in(manifest_path);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    TEST_CHECK(content.find("\"schema\": 2") != std::string::npos, "Schema v2 missing");
    TEST_CHECK(content.find("\"location\": \"Semarang\"") != std::string::npos, "Location missing");
    TEST_CHECK(content.find("\"frames_captured\": 100") != std::string::npos, "Captured frames count missing");
    TEST_CHECK(content.find("\"frames_recorded\": 50") != std::string::npos, "Recorded frames count missing");
    TEST_CHECK(content.find("\"detections_count\": 5") != std::string::npos, "Detections count missing");

    session.finish();
    TEST_CHECK(!session.is_active(), "Session should be inactive after finish");

    std::filesystem::remove_all(test_dir);
    std::cout << "  PASS: Session lifecycle passed." << std::endl;
}

void test_controller_pipeline_and_recording() {
    std::cout << "[Test 2] AppController 5-Thread Pipeline & Recording..." << std::endl;

    app::AppController controller;

    // Connect to Simulator Camera
    camera::CameraId sim_id{.driver = "sim", .id = "hilal", .name = "Sim Hilal"};
    auto conn_res = controller.connect_camera(sim_id);
    TEST_CHECK(conn_res.is_ok(), "Failed to connect simulator camera");

    // Wait for initial preview frames to arrive in slot
    std::optional<app::PreviewData> prev;
    for (int i = 0; i < 50; ++i) {
        prev = controller.get_latest_preview();
        if (prev.has_value() && !prev->rgb_pixels.empty()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    TEST_CHECK(prev.has_value(), "Preview frame not received from processing thread");
    TEST_CHECK(!prev->rgb_pixels.empty(), "Preview RGB buffer is empty");
    TEST_CHECK(!prev->histogram_256.empty(), "Preview histogram is empty");

    // Test Live Stacking
    controller.enable_stacking(true);
    controller.set_stack_method(imaging::StackMethod::Mean);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    controller.reset_stacker();
    controller.enable_stacking(false);

    // Test Visual Enhancements (Preview-Only)
    controller.enable_clahe(true);
    controller.enable_unsharp_mask(true);
    controller.enable_invert(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    controller.enable_clahe(false);
    controller.enable_unsharp_mask(false);
    controller.enable_invert(false);

    // Test Recording to FITS
    controller.start_session("Semarang", "TestHilal", "Tester");
    TEST_CHECK(controller.is_session_active(), "Session should be active");

    controller.start_recording("fits", 3); // Record 3 frames
    TEST_CHECK(controller.is_recording(), "Controller should be recording");

    for (int i = 0; i < 100 && controller.is_recording(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    controller.stop_recording();

    auto status = controller.get_system_status();
    TEST_CHECK(status.frames_recorded >= 1, "At least 1 frame should have been recorded");
    TEST_CHECK(status.camera_connected, "Camera should still be connected");

    controller.end_session();
    controller.disconnect_camera();

    std::cout << "  PASS: AppController pipeline & recording passed." << std::endl;
}

void test_theme_and_translator() {
    std::cout << "[Test 3] Theme Manager & Bilingual Translator..." << std::endl;

    auto& tm = ui::ThemeManager::instance();

    // Verify all 3 stylesheets generate valid QSS
    QString dark_qss = tm.generate_stylesheet(ui::ThemeType::Dark);
    TEST_CHECK(!dark_qss.isEmpty(), "Dark stylesheet empty");
    TEST_CHECK(dark_qss.contains("#0B0B14") && dark_qss.contains("#8B5CF6"), "Dark Sabit palette missing");

    QString red_qss = tm.generate_stylesheet(ui::ThemeType::NightRed);
    TEST_CHECK(!red_qss.isEmpty(), "Night Red stylesheet empty");
    TEST_CHECK(red_qss.contains("#050000"), "Night Red background missing");
    TEST_CHECK(red_qss.contains("#FF2A2A"), "Night Red accent missing");

    QString light_qss = tm.generate_stylesheet(ui::ThemeType::Light);
    TEST_CHECK(!light_qss.isEmpty(), "Light stylesheet empty");
    TEST_CHECK(light_qss.contains("#F8F9FE") && light_qss.contains("#6366F1"), "Light Theme palette missing");

    // Test Translator
    auto& tr = ui::Translator::instance();
    tr.set_language(ui::Language::Indonesian);
    TEST_CHECK(ui::tr_str("connect_camera") == "Hubungkan Kamera", "Indonesian translation mismatch");
    TEST_CHECK(ui::tr_str("driver_setup") == "Pengaturan Driver...", "Indonesian driver_setup translation mismatch");
    TEST_CHECK(ui::tr_str("sunset") == "Matahari Terbenam (Maghrib)", "Indonesian sunset translation mismatch");

    tr.set_language(ui::Language::English);
    TEST_CHECK(ui::tr_str("connect_camera") == "Connect Camera", "English translation mismatch");
    TEST_CHECK(ui::tr_str("driver_setup") == "Driver Setup...", "English driver_setup translation mismatch");
    TEST_CHECK(ui::tr_str("sunset") == "Sunset (Maghrib)", "English sunset translation mismatch");

    std::cout << "  PASS: ThemeManager and Translator verified." << std::endl;
}

void test_ui_offscreen_lifecycle(int argc, char** argv) {
    std::cout << "[Test 4] Qt6 Widgets UI Offscreen Lifecycle..." << std::endl;

    // Use offscreen platform so tests can run in headless CI or without a display
    qputenv("QT_QPA_PLATFORM", "offscreen");

    QApplication app(argc, argv);

    app::AppController controller;
    ui::MainWindow win(controller);
    TEST_CHECK(win.minimumWidth() == 1150, "Window minimum width should be 1150");
    TEST_CHECK(win.minimumHeight() == 720, "Window minimum height should be 720");
    TEST_CHECK(ui::ThemeManager::instance().current_theme() == ui::ThemeType::Dark ||
               ui::ThemeManager::instance().current_theme() == ui::ThemeType::NightRed, "Default theme should be Dark or NightRed");
    win.show();

    // Process events for 200 ms to verify timers, panels, and widgets tick without issue
    for (int i = 0; i < 10; ++i) {
        app.processEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    win.close();
    std::cout << "  PASS: Qt6 Widgets UI offscreen lifecycle passed." << std::endl;

    // Test FluentBadge & MarqueeLabel with active QApplication
    std::cout << "[Test 5] FluentBadge Marquee & Running Text..." << std::endl;
    ui::FluentBadge badge("CAM: Sabit Synthetic Simulator Camera with Very Long Driver Name", ui::BadgeTone::Success, true);
    badge.set_max_width(175);
    TEST_CHECK(badge.max_width() == 175, "Max width should be 175");
    TEST_CHECK(badge.sizeHint().width() == 175, "SizeHint width should be clamped to 175");
    TEST_CHECK(badge.minimumSizeHint().width() == 175, "MinimumSizeHint should match 175");

    ui::MarqueeLabel mlabel("Driver: Long Camera Device Name [1920x1080 | RAW16 | DirectShow]");
    TEST_CHECK(!mlabel.text().isEmpty(), "MarqueeLabel text check");
    mlabel.resize(100, 20);
    std::cout << "  PASS: FluentBadge and MarqueeLabel running text verified." << std::endl;

    // Test 6: SharpCap-style Camera Enumeration, Preprocessing, and Interactive Stretch
    std::cout << "[Test 6] Camera Enumeration & SharpCap Controls..." << std::endl;
    auto cams = controller.enumerate_cameras(true);
    TEST_CHECK(cams.size() >= 3, "Should enumerate at least Test Cameras");
    for (const auto& c : cams) {
        std::cout << "    Found Camera: [" << c.driver << "] " << c.name << " (" << c.id << ")" << std::endl;
    }

    // Connect Test Camera 2 (Gradient) and test Gamma, EV Shift, Flip, Background Subtraction
    camera::CameraId grad_id{.driver = "sim", .id = "gradient", .name = "Test Camera 2 (High Speed)"};
    auto conn_res = controller.connect_camera(grad_id);
    TEST_CHECK(conn_res.is_ok(), "Failed to connect Test Camera 2");
    TEST_CHECK(controller.last_connected_camera().has_value(), "last_connected_camera should be recorded");

    controller.set_gamma(1.80);
    controller.set_exp_gain_shift(1.50);
    controller.set_banding_suppression(35);
    controller.set_background_subtraction(2);
    controller.set_remove_satellite_trails(true);
    controller.set_flip_mode(3);
    controller.set_mask_over_exposed(true);
    controller.set_ca_reduction(true);
    controller.set_timestamp_frames(true);
    controller.set_fps_limit(30.0);

    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    auto prev = controller.get_latest_preview();
    TEST_CHECK(prev.has_value(), "Preview should be produced with preprocessing enabled");
    TEST_CHECK(std::abs(prev->stretch_gamma - 1.80f) < 0.05f, "Preview gamma should reflect 1.80");
    TEST_CHECK(prev->timestamp_frames == true, "Preview timestamp_frames flag should be true");

    controller.disconnect_camera();
    std::cout << "  PASS: SharpCap controls & enumeration verified." << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "=== Sabit Batch 4 Tests (App Controller & UI) ===" << std::endl;

    try {
        test_session_lifecycle();
        test_controller_pipeline_and_recording();
        test_theme_and_translator();
        test_ui_offscreen_lifecycle(argc, argv);

        std::cout << "\n>>> ALL BATCH 4 TESTS PASSED (5/5) <<<" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "UNKNOWN EXCEPTION!" << std::endl;
        return 2;
    }
}
