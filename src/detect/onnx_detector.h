#ifndef SABIT_DETECT_ONNX_DETECTOR_H
#define SABIT_DETECT_ONNX_DETECTOR_H

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <functional>
#include <chrono>

#include "src/core/frame.h"
#include "src/core/result.h"
#include "src/detect/detection.h"

// Forward declaration of ONNX Runtime C types
struct OrtEnv;
struct OrtSession;
struct OrtSessionOptions;
struct OrtMemoryInfo;
struct OrtApi;

namespace sabit::detect {

struct ModelProfile {
    std::string id{"yolov8s"};
    std::string display_name{"YOLOv8s Small"};
    std::string ui_label{"YOLOv8s (Primary - Recommended)"};
    std::string architecture{"YOLOv8s"};
    std::string model_path{"models/hilal/yolov8s/hilal_yolov8s_640.onnx"};
    std::string sha256{"c83c1f3b1f3626d7de84c5f25a87ce010a992cac1aba577828f523db3fcde0bc"};
    std::string input_name{"images"};
    std::string output_name{"output0"};
    int input_size{640};
    int pad_value{114};
    float confidence_threshold{0.30f};
    float nms_iou{0.45f};
    bool embedded_nms{false};
};

struct LetterboxMeta {
    int orig_w{0};
    int orig_h{0};
    int input_w{640};
    int input_h{640};
    int new_unpad_w{640};
    int new_unpad_h{640};
    float scale{1.0f};
    float dw{0.0f};
    float dh{0.0f};
    int top{0};
    int bottom{0};
    int left{0};
    int right{0};
};

struct RawCandidate {
    float x0{0.0f};
    float y0{0.0f};
    float x1{0.0f};
    float y1{0.0f};
    float confidence{0.0f};
    int class_id{0};
};

struct OnnxDetectorConfig {
    std::string model_path{"models/hilal/yolov8s/hilal_yolov8s_640.onnx"};
    std::string manifest_path{"models/manifest.json"};
    std::string profiles_path{"models/hilal/model_profiles.json"};
    std::string model_id{"yolov8s"};
    std::string dll_path{"onnxruntime.dll"};
    bool verify_hash{true};
    float conf_threshold{0.30f};
    float nms_threshold{0.45f};
    int num_threads{2};
    double max_fps{3.0};
    bool prefer_cuda{false};
};

class OnnxDetector {
public:
    OnnxDetector();
    ~OnnxDetector();

    core::Result<void> init(const OnnxDetectorConfig& config);
    void close();

    [[nodiscard]] bool is_initialized() const;

    // Reusable dual-model ONNX lifecycle & pipeline API
    core::Result<void> loadModel(const std::string& model_path, const std::string& expected_sha256 = "");
    core::Result<ModelProfile> setActiveModel(const std::string& model_id_or_path);
    void releaseModel();

    void setConfidenceThreshold(float conf) { conf_threshold_.store(conf); }
    void setNmsThreshold(float nms) { nms_threshold_.store(nms); }

    core::Result<std::vector<float>> preprocessImage(const core::Frame& frame, LetterboxMeta& out_meta) const;
    core::Result<std::vector<float>> runInference(const std::vector<float>& input_tensor, double* out_inference_ms = nullptr);
    [[nodiscard]] std::vector<RawCandidate> decodeOutput(const float* raw_output, size_t num_anchors, float conf_thresh) const;
    [[nodiscard]] std::vector<RawCandidate> applyNMS(const std::vector<RawCandidate>& candidates, float iou_thresh) const;
    [[nodiscard]] std::vector<Detection> restoreBoundingBoxes(const std::vector<RawCandidate>& kept, const LetterboxMeta& meta, int64_t timestamp_utc_ms = 0) const;

    // Static profile & image helpers
    [[nodiscard]] static std::vector<ModelProfile> load_profiles(const std::string& profiles_path = "models/hilal/model_profiles.json");
    [[nodiscard]] static std::optional<ModelProfile> find_profile(const std::string& model_id, const std::string& profiles_path = "models/hilal/model_profiles.json");
    [[nodiscard]] static core::Result<core::Frame> load_image_file(const std::string& image_path);

    // Active metadata & tensor queries
    [[nodiscard]] ModelProfile active_profile() const;
    [[nodiscard]] std::string active_model_id() const;
    [[nodiscard]] std::string active_execution_provider() const;
    [[nodiscard]] std::vector<int64_t> input_tensor_shape() const;
    [[nodiscard]] std::vector<int64_t> output_tensor_shape() const;

    // Run direct synchronous inference on a frame
    core::Result<DetectionResult> detect(const core::Frame& frame);

    // Dynamic thresholds (existing API compatibility)
    void set_conf_threshold(float conf) { setConfidenceThreshold(conf); }
    [[nodiscard]] float conf_threshold() const { return conf_threshold_.load(); }

    void set_nms_threshold(float nms) { setNmsThreshold(nms); }
    [[nodiscard]] float nms_threshold() const { return nms_threshold_.load(); }

    void set_max_fps(double fps) { max_fps_.store(fps); }
    [[nodiscard]] double max_fps() const { return max_fps_.load(); }

    // Async Threaded AI Execution
    void start_worker();
    void stop_worker();
    void submit_frame(const core::Frame& frame);

    // Retrieve latest detections for UI overlay (discards if > max_age_ms, default 500ms)
    [[nodiscard]] std::vector<Detection> get_latest_overlay(int64_t max_age_ms = 500) const;

    // Callback on detection
    void set_detection_callback(std::function<void(const core::Frame&, const DetectionResult&)> cb) {
        on_detection_ = std::move(cb);
    }

private:
    core::Result<void> ensure_runtime_initialized();
    std::string resolve_path(const std::string& path) const;
    bool verify_model_hash(const std::string& resolved_model_path, const std::string& expected_sha256) const;
    bool inspect_session_contracts();
    void worker_loop();

    OnnxDetectorConfig config_{};
    ModelProfile active_profile_{};
    std::string loaded_model_path_;
    std::string active_provider_{"CPUExecutionProvider"};
    std::vector<int64_t> input_shape_{1, 3, 640, 640};
    std::vector<int64_t> output_shape_{1, 5, 8400};
    std::string detected_input_name_{"images"};
    std::string detected_output_name_{"output0"};

    std::atomic<float> conf_threshold_{0.30f};
    std::atomic<float> nms_threshold_{0.45f};
    std::atomic<double> max_fps_{3.0};

    void* dll_handle_{nullptr};
    const OrtApi* ort_{nullptr};
    OrtEnv* env_{nullptr};
    OrtSessionOptions* session_options_{nullptr};
    OrtSession* session_{nullptr};
    OrtMemoryInfo* mem_info_{nullptr};

    mutable std::mutex session_mutex_;
    std::atomic<bool> is_initialized_{false};

    // Threading & Latest Slot
    std::thread worker_thread_;
    std::atomic<bool> is_running_{false};
    mutable std::mutex frame_mutex_;
    std::condition_variable frame_cv_;
    std::optional<core::Frame> pending_frame_;

    mutable std::mutex result_mutex_;
    DetectionResult latest_result_{};
    std::chrono::steady_clock::time_point latest_result_time_{};

    std::function<void(const core::Frame&, const DetectionResult&)> on_detection_;
};

} // namespace sabit::detect

#endif // SABIT_DETECT_ONNX_DETECTOR_H
