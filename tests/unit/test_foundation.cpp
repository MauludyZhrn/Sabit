#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <cmath>
#include <thread>
#include <chrono>

#include "src/core/error.h"
#include "src/core/result.h"
#include "src/core/clock.h"
#include "src/core/frame.h"
#include "src/core/ring_buffer.h"
#include "src/core/latest_slot.h"
#include "src/core/logger.h"
#include "src/camera/icamera.h"
#include "src/camera/sim/simulator_camera.h"

// Simple testing macros
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
using namespace sabit::camera;

// 1. Result<T> and Result<void> Tests
void test_result_success_and_error() {
    Result<int> res_ok = Result<int>::ok(42);
    TEST_ASSERT(res_ok.is_ok(), "res_ok should be ok");
    TEST_ASSERT(!res_ok.is_err(), "res_ok should not be err");
    TEST_ASSERT(static_cast<bool>(res_ok), "res_ok boolean conversion should be true");
    TEST_ASSERT(res_ok.value() == 42, "res_ok.value() should be 42");
    TEST_ASSERT(res_ok.value_or(100) == 42, "res_ok.value_or() should return 42");

    Result<int> res_err = Result<int>::err(ErrorCode::OutOfRange, "Value exceeds maximum");
    TEST_ASSERT(!res_err.is_ok(), "res_err should not be ok");
    TEST_ASSERT(res_err.is_err(), "res_err should be err");
    TEST_ASSERT(!static_cast<bool>(res_err), "res_err boolean conversion should be false");
    TEST_ASSERT(res_err.error().code == ErrorCode::OutOfRange, "Error code should match");
    TEST_ASSERT(res_err.value_or(100) == 100, "res_err.value_or() should return default 100");

    bool threw = false;
    try {
        [[maybe_unused]] int v = res_err.value();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    TEST_ASSERT(threw, "Accessing value() on error Result must throw runtime_error");

    Result<void> void_ok = Result<void>::ok();
    TEST_ASSERT(void_ok.is_ok(), "Result<void> should be ok");

    Result<void> void_err = Result<void>::err(ErrorCode::HardwareError, "Sensor disconnected");
    TEST_ASSERT(void_err.is_err(), "Result<void> error should be err");
    TEST_ASSERT(void_err.error().code == ErrorCode::HardwareError, "Error code should match");
}

// 2. Clock Tests
void test_clock_and_sync() {
    auto now1 = Clock::now_utc();
    auto now_us1 = Clock::now_us();
    auto steady1 = Clock::steady_now_us();

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    auto now2 = Clock::now_utc();
    auto now_us2 = Clock::now_us();
    auto steady2 = Clock::steady_now_us();

    TEST_ASSERT(now2 >= now1, "now_utc should be monotonic");
    TEST_ASSERT(now_us2 > now_us1, "now_us should advance");
    TEST_ASSERT(steady2 > steady1, "steady_now_us should advance");

    std::string iso = Clock::to_iso8601(now1);
    TEST_ASSERT(iso.size() >= 20, "ISO 8601 string length check");
    TEST_ASSERT(iso.back() == 'Z', "ISO 8601 string should end in Z (UTC)");

    Clock::TimePoint parsed = Clock::from_iso8601(iso);
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now1 - parsed).count();
    TEST_ASSERT(std::abs(diff) <= 1, "ISO 8601 roundtrip difference should be <= 1ms");

    ClockSync sync;
    sync.status = ClockSyncStatus::Synced;
    sync.last_sync_utc = now1;
    Clock::set_sync(sync);
    ClockSync read_sync = Clock::get_sync();
    TEST_ASSERT(read_sync.status == ClockSyncStatus::Synced, "Clock sync status should be Synced");
    TEST_ASSERT(read_sync.status_string().find("synced_") == 0, "Clock sync string should start with synced_");
}

// 3. Frame and FramePool Tests
void test_frame_and_pool() {
    Frame f = Frame::create_allocated(1920, 1080, 1, 16);
    TEST_ASSERT(f.is_valid(), "Frame should be valid");
    TEST_ASSERT(f.width == 1920, "Width should be 1920");
    TEST_ASSERT(f.height == 1080, "Height should be 1080");
    TEST_ASSERT(f.pixel_count() == 1920 * 1080, "Pixel count check");
    TEST_ASSERT(f.byte_size() == 1920 * 1080 * 2, "16-bit byte size check");

    // Write a test pixel
    f.pixels[0] = 54321;
    TEST_ASSERT(f.pixels[0] == 54321, "Pixel readback check");

    // FramePool tests
    auto pool = std::make_shared<FramePool>(1000, 4, 8);
    TEST_ASSERT(pool->available_count() == 4, "Initial available pool count should be 4");

    {
        auto buf1 = pool->acquire();
        TEST_ASSERT(buf1 != nullptr, "Acquired buffer should not be null");
        TEST_ASSERT(pool->available_count() == 3, "Available count should decrement to 3");

        buf1[0] = 12345;
        TEST_ASSERT(buf1[0] == 12345, "Buffer contents writable");
    }
    // When buf1 goes out of scope, it should return to the pool
    TEST_ASSERT(pool->available_count() == 4, "Buffer should be returned to pool upon destruction");
}

// 4. RingBuffer Drop-Oldest Policy and Drop Counting (TC-046)
void test_ring_buffer_drop_oldest() {
    RingBuffer<int> rb(3);
    TEST_ASSERT(rb.capacity() == 3, "Capacity should be 3");
    TEST_ASSERT(rb.is_empty(), "Buffer should initially be empty");

    // Push 1, 2, 3
    TEST_ASSERT(!rb.push(1), "Push 1 should not drop");
    TEST_ASSERT(!rb.push(2), "Push 2 should not drop");
    TEST_ASSERT(!rb.push(3), "Push 3 should not drop");
    TEST_ASSERT(rb.is_full(), "Buffer should be full");
    TEST_ASSERT(rb.dropped_count() == 0, "No drops yet");

    // Push 4: should drop 1 (oldest)
    bool dropped = rb.push(4);
    TEST_ASSERT(dropped, "Push 4 should drop oldest");
    TEST_ASSERT(rb.dropped_count() == 1, "Dropped count should be 1");

    // Push 5: should drop 2 (oldest)
    dropped = rb.push(5);
    TEST_ASSERT(dropped, "Push 5 should drop oldest");
    TEST_ASSERT(rb.dropped_count() == 2, "Dropped count should be 2");

    // Reading should yield 3, 4, 5 in FIFO order
    auto item1 = rb.pop(std::chrono::milliseconds(10));
    TEST_ASSERT(item1.has_value() && *item1 == 3, "First popped item should be 3");

    auto item2 = rb.pop(std::chrono::milliseconds(10));
    TEST_ASSERT(item2.has_value() && *item2 == 4, "Second popped item should be 4");

    auto item3 = rb.pop(std::chrono::milliseconds(10));
    TEST_ASSERT(item3.has_value() && *item3 == 5, "Third popped item should be 5");

    TEST_ASSERT(rb.is_empty(), "Buffer should now be empty");
}

// 5. Multi-threaded Invariant Test: written + dropped == sent (TC-046)
void test_ring_buffer_multithreaded_invariant() {
    const int kTotalFrames = 500;
    RingBuffer<int> rb(10); // Small buffer to force drops

    std::atomic<int> received_count{0};

    // Consumer thread
    std::thread consumer([&]() {
        while (true) {
            auto item = rb.pop(std::chrono::milliseconds(20));
            if (item.has_value()) {
                ++received_count;
            } else if (rb.size() == 0 && received_count + rb.dropped_count() >= kTotalFrames) {
                break;
            }
        }
    });

    // Fast Producer thread
    for (int i = 0; i < kTotalFrames; ++i) {
        rb.push(i);
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }

    consumer.join();

    uint64_t dropped = rb.dropped_count();
    int received = received_count.load();
    std::cout << "    [Invariant Check] Sent: " << kTotalFrames
              << ", Received: " << received
              << ", Dropped: " << dropped << "\n";
    TEST_ASSERT(received + dropped == kTotalFrames, "TC-046 Invariant: received + dropped must equal sent frames");
}

// 6. LatestSlot Overwrite and Non-blocking Tests (FR-08, TC-021)
void test_latest_slot() {
    LatestSlot<int> slot;
    TEST_ASSERT(slot.is_empty(), "Slot should initially be empty");

    slot.put(10);
    TEST_ASSERT(slot.has_new(), "Slot should have new item");
    TEST_ASSERT(!slot.is_empty(), "Slot is not empty");

    // Overwrite with 20 and 30 without reading 10
    slot.put(20);
    slot.put(30);

    auto val = slot.get_latest();
    TEST_ASSERT(val.has_value() && *val == 30, "Latest item should be 30");
    TEST_ASSERT(!slot.has_new(), "has_new() should be false after get_latest()");

    auto taken = slot.take_latest();
    TEST_ASSERT(taken.has_value() && *taken == 30, "Taken item should be 30");
    TEST_ASSERT(slot.is_empty(), "Slot should be empty after take_latest()");
}

// 7. Simulator Camera Tests (FR-03, TC-003, TC-024)
void test_simulator_camera() {
    SimulatorCamera cam(SimulatorMode::Hilal, 640, 480);

    CameraId id{"sim", "hilal", "Hilal Simulator"};
    auto open_res = cam.open(id);
    TEST_ASSERT(open_res.is_ok(), "Simulator open should succeed");

    CameraCaps caps = cam.caps();
    TEST_ASSERT(caps.max_width == 640, "Caps max_width check");
    TEST_ASSERT(caps.max_height == 480, "Caps max_height check");

    // Settings validation tests
    CameraSettings bad_exp;
    bad_exp.exposure_us = 999999999999LL; // Way too large
    auto bad_exp_res = cam.apply(bad_exp);
    TEST_ASSERT(bad_exp_res.is_err(), "Out of range exposure must be rejected");
    TEST_ASSERT(bad_exp_res.error().code == ErrorCode::OutOfRange, "Expected OutOfRange error");

    CameraSettings bad_gain;
    bad_gain.gain = 9999.0;
    auto bad_gain_res = cam.apply(bad_gain);
    TEST_ASSERT(bad_gain_res.is_err(), "Out of range gain must be rejected");

    CameraSettings bad_bin;
    bad_bin.binning = 3; // Non-standard binning
    auto bad_bin_res = cam.apply(bad_bin);
    TEST_ASSERT(bad_bin_res.is_err(), "Binning 3 must be rejected");

    CameraSettings good_settings;
    good_settings.exposure_us = 50000;
    good_settings.gain = 200.0;
    good_settings.binning = 2;
    good_settings.bit_depth = 16;
    auto good_res = cam.apply(good_settings);
    TEST_ASSERT(good_res.is_ok(), "Valid settings must be accepted");
    TEST_ASSERT(good_res.value().exposure_us == 50000, "Readback exposure check");
    TEST_ASSERT(good_res.value().gain == 200.0, "Readback gain check");

    // TC-024: Gradient 16-bit coverage test (0..65535)
    cam.set_mode(SimulatorMode::Gradient);
    good_settings.binning = 1;
    cam.apply(good_settings);

    std::vector<Frame> captured_frames;
    std::mutex capture_mutex;
    std::condition_variable capture_cv;

    cam.set_target_fps(30.0);
    auto start_res = cam.start([&](const Frame& f) {
        std::lock_guard<std::mutex> lock(capture_mutex);
        captured_frames.push_back(f);
        capture_cv.notify_all();
    });
    TEST_ASSERT(start_res.is_ok(), "Simulator start should succeed");

    // Wait for at least 3 frames
    {
        std::unique_lock<std::mutex> lock(capture_mutex);
        capture_cv.wait_for(lock, std::chrono::seconds(2), [&]() {
            return captured_frames.size() >= 3;
        });
    }

    cam.stop();

    TEST_ASSERT(captured_frames.size() >= 3, "Should have captured at least 3 frames");
    TEST_ASSERT(captured_frames[0].bit_depth == 16, "Frame must be 16-bit");
    TEST_ASSERT(captured_frames[1].seq > captured_frames[0].seq, "Frame seq must increase monotonically");

    // Verify 16-bit gradient minimum and maximum values
    const uint16_t* px = captured_frames[0].pixels.get();
    size_t count = captured_frames[0].pixel_count();
    uint16_t min_val = 65535;
    uint16_t max_val = 0;
    for (size_t i = 0; i < count; ++i) {
        min_val = std::min(min_val, px[i]);
        max_val = std::max(max_val, px[i]);
    }

    TEST_ASSERT(min_val == 0, "TC-024: Gradient minimum value must be exactly 0");
    TEST_ASSERT(max_val == 65535, "TC-024: Gradient maximum value must be exactly 65535");

    cam.close();
    TEST_ASSERT(!cam.status().is_connected, "Camera status should be disconnected after close");
}

int main() {
    std::cout << "========================================================\n";
    std::cout << " Sabit Foundation C++20 Unit Test Suite\n";
    std::cout << "========================================================\n";

    RUN_TEST(test_result_success_and_error);
    RUN_TEST(test_clock_and_sync);
    RUN_TEST(test_frame_and_pool);
    RUN_TEST(test_ring_buffer_drop_oldest);
    RUN_TEST(test_ring_buffer_multithreaded_invariant);
    RUN_TEST(test_latest_slot);
    RUN_TEST(test_simulator_camera);

    std::cout << "========================================================\n";
    std::cout << " Summary: " << g_tests_passed << " passed, " << g_tests_failed << " failed.\n";
    std::cout << "========================================================\n";

    return (g_tests_failed == 0) ? 0 : 1;
}
