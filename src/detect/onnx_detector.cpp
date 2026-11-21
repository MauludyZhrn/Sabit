#include "src/detect/onnx_detector.h"
#include "src/detect/sha256.h"
#include "src/core/logger.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincodec.h>
#define _stdcall __stdcall
#include "vendor/onnxruntime/onnxruntime_c_api.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace sabit::detect {

namespace {

float compute_iou(float x0_a, float y0_a, float x1_a, float y1_a,
                  float x0_b, float y0_b, float x1_b, float y1_b) {
    float inter_x0 = std::max(x0_a, x0_b);
    float inter_y0 = std::max(y0_a, y0_b);
    float inter_x1 = std::min(x1_a, x1_b);
    float inter_y1 = std::min(y1_a, y1_b);

    float inter_w = std::max(0.0f, inter_x1 - inter_x0);
    float inter_h = std::max(0.0f, inter_y1 - inter_y0);
    float inter_area = inter_w * inter_h;

    float area_a = std::max(0.0f, x1_a - x0_a) * std::max(0.0f, y1_a - y0_a);
    float area_b = std::max(0.0f, x1_b - x0_b) * std::max(0.0f, y1_b - y0_b);

    float union_area = area_a + area_b - inter_area;
    if (union_area <= 1e-6f) return 0.0f;
    return inter_area / union_area;
}

std::wstring to_wstring(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0);
    std::wstring wstr(static_cast<size_t>(size_needed), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), wstr.data(), size_needed);
    return wstr;
}

std::string normalize_lower(std::string s) {
    for (char& ch : s) {
        if (ch == '\\') ch = '/';
        else ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return s;
}

std::string extract_json_string(const std::string& block, const std::string& key, const std::string& def = "") {
    size_t kpos = block.find("\"" + key + "\"");
    if (kpos == std::string::npos) return def;
    size_t colon = block.find(':', kpos + key.size() + 2);
    if (colon == std::string::npos) return def;
    size_t q1 = block.find('"', colon + 1);
    if (q1 == std::string::npos) return def;
    size_t q2 = block.find('"', q1 + 1);
    if (q2 == std::string::npos) return def;
    return block.substr(q1 + 1, q2 - q1 - 1);
}

double extract_json_number(const std::string& block, const std::string& key, double def) {
    size_t kpos = block.find("\"" + key + "\"");
    if (kpos == std::string::npos) return def;
    size_t colon = block.find(':', kpos + key.size() + 2);
    if (colon == std::string::npos) return def;
    size_t start = block.find_first_not_of(" \t\r\n", colon + 1);
    if (start == std::string::npos) return def;
    try {
        return std::stod(block.substr(start));
    } catch (...) {
        return def;
    }
}

bool extract_json_bool(const std::string& block, const std::string& key, bool def) {
    size_t kpos = block.find("\"" + key + "\"");
    if (kpos == std::string::npos) return def;
    size_t colon = block.find(':', kpos + key.size() + 2);
    if (colon == std::string::npos) return def;
    size_t start = block.find_first_not_of(" \t\r\n", colon + 1);
    if (start == std::string::npos) return def;
    if (block.compare(start, 4, "true") == 0) return true;
    if (block.compare(start, 5, "false") == 0) return false;
    return def;
}

std::string extract_object_block(const std::string& json, const std::string& key) {
    size_t kpos = json.find("\"" + key + "\"");
    if (kpos == std::string::npos) return "";
    size_t brace = json.find('{', kpos);
    if (brace == std::string::npos) return "";
    int depth = 0;
    for (size_t i = brace; i < json.size(); ++i) {
        if (json[i] == '{') ++depth;
        else if (json[i] == '}') {
            --depth;
            if (depth == 0) {
                return json.substr(brace, i - brace + 1);
            }
        }
    }
    return "";
}

std::vector<std::string> extract_top_level_objects_in_array(const std::string& json, const std::string& key) {
    std::vector<std::string> blocks;
    size_t kpos = json.find("\"" + key + "\"");
    if (kpos == std::string::npos) return blocks;
    size_t arr_start = json.find('[', kpos);
    if (arr_start == std::string::npos) return blocks;

    int depth = 0;
    size_t obj_start = std::string::npos;
    for (size_t i = arr_start + 1; i < json.size(); ++i) {
        char ch = json[i];
        if (ch == ']' && depth == 0) break;
        if (ch == '{') {
            if (depth == 0) obj_start = i;
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0 && obj_start != std::string::npos) {
                blocks.push_back(json.substr(obj_start, i - obj_start + 1));
                obj_start = std::string::npos;
            }
        }
    }
    return blocks;
}

std::vector<ModelProfile> default_builtin_profiles() {
    ModelProfile p_s;
    p_s.id = "yolov8s";
    p_s.display_name = "YOLOv8s Small";
    p_s.ui_label = "YOLOv8s (Primary - Recommended)";
    p_s.architecture = "YOLOv8s";
    p_s.model_path = "models/hilal/yolov8s/hilal_yolov8s_640.onnx";
    p_s.sha256 = "c83c1f3b1f3626d7de84c5f25a87ce010a992cac1aba577828f523db3fcde0bc";
    p_s.input_name = "images";
    p_s.output_name = "output0";
    p_s.input_size = 640;
    p_s.pad_value = 114;
    p_s.confidence_threshold = 0.30f;
    p_s.nms_iou = 0.45f;
    p_s.embedded_nms = false;

    ModelProfile p_n;
    p_n.id = "yolov8n";
    p_n.display_name = "YOLOv8n Nano";
    p_n.ui_label = "YOLOv8n (Lightweight Alternative)";
    p_n.architecture = "YOLOv8n";
    p_n.model_path = "models/hilal/yolov8n/best.onnx";
    p_n.sha256 = "0f3281bfbf96dad2860ee1747e833349f9db878aa48cede60d09cd75fc93fc79";
    p_n.input_name = "images";
    p_n.output_name = "output0";
    p_n.input_size = 640;
    p_n.pad_value = 114;
    p_n.confidence_threshold = 0.75f;
    p_n.nms_iou = 0.45f;
    p_n.embedded_nms = false;

    return {p_s, p_n};
}

} // namespace

OnnxDetector::OnnxDetector() = default;

OnnxDetector::~OnnxDetector() {
    close();
}

std::string OnnxDetector::resolve_path(const std::string& path) const {
    if (path.empty()) return path;
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        return path;
    }
    char exe_buf[MAX_PATH]{};
    if (GetModuleFileNameA(nullptr, exe_buf, MAX_PATH) > 0) {
        std::filesystem::path app_dir = std::filesystem::path(exe_buf).parent_path();
        std::filesystem::path cand = app_dir / path;
        if (std::filesystem::exists(cand, ec)) {
            return cand.string();
        }
    }
    return path;
}

std::vector<ModelProfile> OnnxDetector::load_profiles(const std::string& profiles_path) {
    std::vector<ModelProfile> profiles = default_builtin_profiles();

    std::string resolved = profiles_path;
    std::error_code ec;
    if (!std::filesystem::exists(resolved, ec)) {
        char exe_buf[MAX_PATH]{};
        if (GetModuleFileNameA(nullptr, exe_buf, MAX_PATH) > 0) {
            auto cand = std::filesystem::path(exe_buf).parent_path() / profiles_path;
            if (std::filesystem::exists(cand, ec)) {
                resolved = cand.string();
            }
        }
    }

    std::ifstream ifs(resolved);
    if (!ifs.is_open()) {
        return profiles;
    }

    std::stringstream ss;
    ss << ifs.rdbuf();
    std::string json = ss.str();

    std::string models_obj = extract_object_block(json, "models");
    if (models_obj.empty()) {
        return profiles;
    }

    for (auto& prof : profiles) {
        std::string blk = extract_object_block(models_obj, prof.id);
        if (blk.empty()) continue;

        prof.display_name = extract_json_string(blk, "display_name", prof.display_name);
        prof.ui_label = extract_json_string(blk, "ui_label", prof.ui_label);
        prof.architecture = extract_json_string(blk, "architecture", prof.architecture);
        prof.model_path = extract_json_string(blk, "model_path", prof.model_path);
        prof.sha256 = extract_json_string(blk, "sha256", prof.sha256);
        prof.input_name = extract_json_string(blk, "input_name", prof.input_name);
        prof.output_name = extract_json_string(blk, "output_name", prof.output_name);
        prof.input_size = static_cast<int>(extract_json_number(blk, "input_size", prof.input_size));
        prof.pad_value = static_cast<int>(extract_json_number(blk, "pad_value", prof.pad_value));
        prof.confidence_threshold = static_cast<float>(extract_json_number(blk, "confidence_threshold", prof.confidence_threshold));
        prof.nms_iou = static_cast<float>(extract_json_number(blk, "nms_iou", prof.nms_iou));
        prof.embedded_nms = extract_json_bool(blk, "embedded_nms", prof.embedded_nms);
    }

    return profiles;
}

std::optional<ModelProfile> OnnxDetector::find_profile(const std::string& model_id, const std::string& profiles_path) {
    auto profiles = load_profiles(profiles_path);
    std::string needle = normalize_lower(model_id);
    for (const auto& p : profiles) {
        if (normalize_lower(p.id) == needle ||
            normalize_lower(p.architecture) == needle ||
            normalize_lower(p.model_path) == needle) {
            return p;
        }
    }
    return std::nullopt;
}

bool OnnxDetector::verify_model_hash(const std::string& resolved_model_path, const std::string& expected_sha256) const {
    if (!config_.verify_hash) return true;

    std::string target_hash = expected_sha256;
    if (target_hash.empty() && !config_.manifest_path.empty()) {
        std::string resolved_manifest = resolve_path(config_.manifest_path);
        std::ifstream f(resolved_manifest);
        if (!f.is_open()) {
            SABIT_LOG_WARN("Manifest file not found: ", config_.manifest_path);
            return false;
        }

        std::stringstream buffer;
        buffer << f.rdbuf();
        std::string content = buffer.str();

        std::string req_norm = normalize_lower(resolved_model_path);
        std::string req_fname = normalize_lower(std::filesystem::path(resolved_model_path).filename().string());

        auto blocks = extract_top_level_objects_in_array(content, "models");
        for (const auto& blk : blocks) {
            std::string f_entry = normalize_lower(extract_json_string(blk, "file"));
            std::string p_entry = normalize_lower(extract_json_string(blk, "model_path"));
            if ((!f_entry.empty() && (req_norm.find(f_entry) != std::string::npos || req_fname == std::filesystem::path(f_entry).filename().string())) ||
                (!p_entry.empty() && req_norm.find(p_entry) != std::string::npos)) {
                target_hash = extract_json_string(blk, "sha256");
                break;
            }
        }
        if (target_hash.empty()) {
            target_hash = extract_json_string(content, "sha256");
        }
    }

    if (target_hash.empty()) {
        SABIT_LOG_WARN("No expected SHA-256 hash found for model: ", resolved_model_path);
        return false;
    }

    std::string actual_hash = Sha256::file_sha256(resolved_model_path);
    if (actual_hash != target_hash) {
        SABIT_LOG_ERROR("ONNX Model hash mismatch! Expected: ", target_hash, ", got: ", actual_hash);
        return false;
    }

    SABIT_LOG_INFO("[AI] ONNX SHA-256 Verified: Model=", std::filesystem::path(resolved_model_path).filename().string(),
                   " SHA256=", actual_hash.substr(0, 12));
    return true;
}

core::Result<void> OnnxDetector::ensure_runtime_initialized() {
    if (ort_ && env_ && session_options_ && mem_info_) {
        return core::Result<void>::ok();
    }

    std::string dll_resolved = resolve_path(config_.dll_path);
    HMODULE hMod = LoadLibraryA(dll_resolved.c_str());
    if (!hMod) {
        hMod = LoadLibraryA("onnxruntime.dll");
    }
    if (!hMod) {
        return core::Result<void>::err(core::Error(core::ErrorCode::DriverMissing, "Failed to load onnxruntime.dll"));
    }
    dll_handle_ = reinterpret_cast<void*>(hMod);

    typedef const OrtApiBase* (*FnOrtGetApiBase)(void);
    auto pGetApiBase = reinterpret_cast<FnOrtGetApiBase>(reinterpret_cast<void*>(GetProcAddress(hMod, "OrtGetApiBase")));
    if (!pGetApiBase) {
        return core::Result<void>::err(core::Error(core::ErrorCode::HardwareError, "OrtGetApiBase not found in onnxruntime.dll"));
    }

    const OrtApiBase* api_base = pGetApiBase();
    ort_ = api_base->GetApi(ORT_API_VERSION);
    if (!ort_) {
        return core::Result<void>::err(core::Error(core::ErrorCode::Unsupported, "Failed to obtain OrtApi v17"));
    }

    OrtStatus* st = ort_->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "SabitDetector", &env_);
    if (st) {
        std::string err_msg = ort_->GetErrorMessage(st);
        ort_->ReleaseStatus(st);
        return core::Result<void>::err(core::Error(core::ErrorCode::HardwareError, "CreateEnv failed: " + err_msg));
    }

    st = ort_->CreateSessionOptions(&session_options_);
    if (st) {
        std::string err_msg = ort_->GetErrorMessage(st);
        ort_->ReleaseStatus(st);
        return core::Result<void>::err(core::Error(core::ErrorCode::HardwareError, "CreateSessionOptions failed: " + err_msg));
    }

    if (OrtStatus* st_t = ort_->SetIntraOpNumThreads(session_options_, config_.num_threads)) {
        ort_->ReleaseStatus(st_t);
    }
    if (OrtStatus* st_o = ort_->SetSessionGraphOptimizationLevel(session_options_, ORT_ENABLE_ALL)) {
        ort_->ReleaseStatus(st_o);
    }

    // Check CUDAExecutionProvider only if requested AND both provider & CUDA driver DLLs exist
    active_provider_ = "CPUExecutionProvider";
    if (config_.prefer_cuda && ort_->GetAvailableProviders && ort_->ReleaseAvailableProviders) {
        char** providers = nullptr;
        int num_providers = 0;
        if (OrtStatus* sp = ort_->GetAvailableProviders(&providers, &num_providers); !sp && providers) {
            bool has_cuda_ep = false;
            for (int i = 0; i < num_providers; ++i) {
                if (providers[i] && std::string(providers[i]) == "CUDAExecutionProvider") {
                    has_cuda_ep = true;
                    break;
                }
            }
            if (OrtStatus* rel_st = ort_->ReleaseAvailableProviders(providers, num_providers)) {
                ort_->ReleaseStatus(rel_st);
            }

            if (has_cuda_ep) {
                HMODULE hCudaDriver = LoadLibraryA("nvcuda.dll");
                HMODULE hOrtCuda = LoadLibraryA("onnxruntime_providers_cuda.dll");
                if (hCudaDriver && hOrtCuda) {
                    OrtCUDAProviderOptions cuda_opts{};
                    cuda_opts.device_id = 0;
                    OrtStatus* sc = ort_->SessionOptionsAppendExecutionProvider_CUDA(session_options_, &cuda_opts);
                    if (!sc) {
                        active_provider_ = "CUDAExecutionProvider";
                    } else {
                        ort_->ReleaseStatus(sc);
                    }
                }
                if (hOrtCuda) FreeLibrary(hOrtCuda);
                if (hCudaDriver) FreeLibrary(hCudaDriver);
            }
        } else if (sp) {
            ort_->ReleaseStatus(sp);
        }
    }

    st = ort_->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &mem_info_);
    if (st) {
        std::string err_msg = ort_->GetErrorMessage(st);
        ort_->ReleaseStatus(st);
        return core::Result<void>::err(core::Error(core::ErrorCode::HardwareError, "CreateCpuMemoryInfo failed: " + err_msg));
    }

    return core::Result<void>::ok();
}

bool OnnxDetector::inspect_session_contracts() {
    if (!ort_ || !session_) return false;

    OrtAllocator* allocator = nullptr;
    if (OrtStatus* sa = ort_->GetAllocatorWithDefaultOptions(&allocator); sa || !allocator) {
        if (sa) ort_->ReleaseStatus(sa);
        return false;
    }

    size_t num_in = 0;
    if (OrtStatus* sc = ort_->SessionGetInputCount(session_, &num_in); sc || num_in == 0) {
        if (sc) ort_->ReleaseStatus(sc);
        return false;
    }
    char* in_name = nullptr;
    if (OrtStatus* sn = ort_->SessionGetInputName(session_, 0, allocator, &in_name); !sn && in_name) {
        detected_input_name_ = in_name;
        if (OrtStatus* sa_free = ort_->AllocatorFree(allocator, in_name)) {
            ort_->ReleaseStatus(sa_free);
        }
    } else if (sn) {
        ort_->ReleaseStatus(sn);
    }

    OrtTypeInfo* in_type = nullptr;
    if (OrtStatus* sti = ort_->SessionGetInputTypeInfo(session_, 0, &in_type); !sti && in_type) {
        const OrtTensorTypeAndShapeInfo* tinfo = nullptr;
        if (OrtStatus* scast = ort_->CastTypeInfoToTensorInfo(in_type, &tinfo); !scast && tinfo) {
            size_t ndims = 0;
            if (OrtStatus* sd = ort_->GetDimensionsCount(tinfo, &ndims); !sd && ndims > 0) {
                std::vector<int64_t> dims(ndims, 0);
                if (OrtStatus* sdims = ort_->GetDimensions(tinfo, dims.data(), ndims); !sdims) {
                    input_shape_ = std::move(dims);
                } else {
                    ort_->ReleaseStatus(sdims);
                }
            } else if (sd) {
                ort_->ReleaseStatus(sd);
            }
        } else if (scast) {
            ort_->ReleaseStatus(scast);
        }
        ort_->ReleaseTypeInfo(in_type);
    } else if (sti) {
        ort_->ReleaseStatus(sti);
    }

    size_t num_out = 0;
    if (OrtStatus* so = ort_->SessionGetOutputCount(session_, &num_out); so || num_out == 0) {
        if (so) ort_->ReleaseStatus(so);
        return false;
    }
    char* out_name = nullptr;
    if (OrtStatus* son = ort_->SessionGetOutputName(session_, 0, allocator, &out_name); !son && out_name) {
        detected_output_name_ = out_name;
        if (OrtStatus* sa_free = ort_->AllocatorFree(allocator, out_name)) {
            ort_->ReleaseStatus(sa_free);
        }
    } else if (son) {
        ort_->ReleaseStatus(son);
    }

    OrtTypeInfo* out_type = nullptr;
    if (OrtStatus* sto = ort_->SessionGetOutputTypeInfo(session_, 0, &out_type); !sto && out_type) {
        const OrtTensorTypeAndShapeInfo* tinfo = nullptr;
        if (OrtStatus* scast = ort_->CastTypeInfoToTensorInfo(out_type, &tinfo); !scast && tinfo) {
            size_t ndims = 0;
            if (OrtStatus* sd = ort_->GetDimensionsCount(tinfo, &ndims); !sd && ndims > 0) {
                std::vector<int64_t> dims(ndims, 0);
                if (OrtStatus* sdims = ort_->GetDimensions(tinfo, dims.data(), ndims); !sdims) {
                    output_shape_ = std::move(dims);
                } else {
                    ort_->ReleaseStatus(sdims);
                }
            } else if (sd) {
                ort_->ReleaseStatus(sd);
            }
        } else if (scast) {
            ort_->ReleaseStatus(scast);
        }
        ort_->ReleaseTypeInfo(out_type);
    } else if (sto) {
        ort_->ReleaseStatus(sto);
    }

    return true;
}

core::Result<void> OnnxDetector::loadModel(const std::string& model_path, const std::string& expected_sha256) {
    std::string resolved_model = resolve_path(model_path);
    std::error_code ec;
    if (!std::filesystem::exists(resolved_model, ec)) {
        return core::Result<void>::err(core::Error(core::ErrorCode::NotFound, "ONNX model file not found: " + model_path));
    }

    if (!verify_model_hash(resolved_model, expected_sha256)) {
        return core::Result<void>::err(core::Error(core::ErrorCode::InvalidState, "ONNX model SHA-256 hash mismatch with manifest"));
    }

    std::lock_guard<std::mutex> lock(session_mutex_);

    std::string canonical_next = normalize_lower(std::filesystem::weakly_canonical(resolved_model, ec).string());
    if (canonical_next.empty()) canonical_next = normalize_lower(resolved_model);

    if (session_ && is_initialized_.load() && loaded_model_path_ == canonical_next) {
        SABIT_LOG_INFO("[AI] Model already loaded (skipping duplicate session): ", resolved_model);
        return core::Result<void>::ok();
    }

    auto rt_res = ensure_runtime_initialized();
    if (!rt_res.is_ok()) {
        return rt_res;
    }

    if (session_) {
        ort_->ReleaseSession(session_);
        session_ = nullptr;
        is_initialized_.store(false);
        loaded_model_path_.clear();
    }

    std::wstring w_model_path = to_wstring(resolved_model);
    OrtStatus* st = ort_->CreateSession(env_, w_model_path.c_str(), session_options_, &session_);
    if (st) {
        std::string err_msg = ort_->GetErrorMessage(st);
        ort_->ReleaseStatus(st);
        return core::Result<void>::err(core::Error(core::ErrorCode::NotFound, "CreateSession failed on " + resolved_model + ": " + err_msg));
    }

    inspect_session_contracts();
    config_.model_path = resolved_model;
    loaded_model_path_ = canonical_next;
    is_initialized_.store(true);

    SABIT_LOG_INFO("[AI] Loaded Model: File=", std::filesystem::path(resolved_model).filename().string(),
                   " Provider=", active_provider_,
                   " Input=", detected_input_name_,
                   " Output=", detected_output_name_);
    return core::Result<void>::ok();
}

core::Result<ModelProfile> OnnxDetector::setActiveModel(const std::string& model_id_or_path) {
    auto prof_opt = find_profile(model_id_or_path, config_.profiles_path);
    ModelProfile profile;
    if (prof_opt.has_value()) {
        profile = *prof_opt;
    } else {
        profile.id = model_id_or_path;
        profile.display_name = std::filesystem::path(model_id_or_path).filename().string();
        profile.ui_label = profile.display_name;
        profile.architecture = "CustomONNX";
        profile.model_path = model_id_or_path;
        profile.sha256.clear();
        profile.confidence_threshold = conf_threshold_.load();
        profile.nms_iou = nms_threshold_.load();
    }

    auto res = loadModel(profile.model_path, profile.sha256);
    if (!res.is_ok()) {
        return core::Result<ModelProfile>::err(res.error());
    }

    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        active_profile_ = profile;
        if (!detected_input_name_.empty()) active_profile_.input_name = detected_input_name_;
        if (!detected_output_name_.empty()) active_profile_.output_name = detected_output_name_;
    }

    setConfidenceThreshold(profile.confidence_threshold);
    setNmsThreshold(profile.nms_iou);

    {
        std::lock_guard<std::mutex> lock(result_mutex_);
        latest_result_ = DetectionResult{};
    }

    SABIT_LOG_INFO("[AI] Active Model Switched: Model=", profile.display_name,
                   " Conf=", profile.confidence_threshold,
                   " NMS=", profile.nms_iou,
                   " Provider=", active_provider_);
    return core::Result<ModelProfile>::ok(profile);
}

void OnnxDetector::releaseModel() {
    std::lock_guard<std::mutex> lock(session_mutex_);
    is_initialized_.store(false);
    loaded_model_path_.clear();
    if (ort_ && session_) {
        ort_->ReleaseSession(session_);
        session_ = nullptr;
    }
}

core::Result<void> OnnxDetector::init(const OnnxDetectorConfig& config) {
    close();
    config_ = config;
    conf_threshold_.store(config.conf_threshold);
    nms_threshold_.store(config.nms_threshold);
    max_fps_.store(config.max_fps);

    // Match profile from profiles_path if possible
    ModelProfile matched;
    std::string req_norm = normalize_lower(config.model_path);
    auto profiles = load_profiles(config.profiles_path);
    bool found_profile = false;
    for (const auto& p : profiles) {
        if (normalize_lower(p.id) == normalize_lower(config.model_id) ||
            (!req_norm.empty() && (normalize_lower(p.model_path) == req_norm ||
             req_norm.find(std::filesystem::path(normalize_lower(p.model_path)).filename().string()) != std::string::npos))) {
            matched = p;
            found_profile = true;
            break;
        }
    }

    if (found_profile && req_norm == normalize_lower(matched.model_path)) {
        active_profile_ = matched;
    } else {
        active_profile_ = ModelProfile{};
        active_profile_.id = config.model_id.empty() ? "custom" : config.model_id;
        active_profile_.display_name = found_profile && config.model_path == matched.model_path
            ? matched.display_name
            : std::filesystem::path(config.model_path).filename().string();
        active_profile_.ui_label = active_profile_.display_name;
        active_profile_.model_path = config.model_path;
        active_profile_.sha256.clear();
        active_profile_.confidence_threshold = config.conf_threshold;
        active_profile_.nms_iou = config.nms_threshold;
    }

    auto res = loadModel(config_.model_path, active_profile_.sha256);
    if (!res.is_ok()) {
        close();
        return res;
    }

    return core::Result<void>::ok();
}

void OnnxDetector::close() {
    stop_worker();
    releaseModel();

    std::lock_guard<std::mutex> lock(session_mutex_);
    if (ort_) {
        if (mem_info_) { ort_->ReleaseMemoryInfo(mem_info_); mem_info_ = nullptr; }
        if (session_options_) { ort_->ReleaseSessionOptions(session_options_); session_options_ = nullptr; }
        if (env_) { ort_->ReleaseEnv(env_); env_ = nullptr; }
    }

    if (dll_handle_) {
        FreeLibrary(reinterpret_cast<HMODULE>(dll_handle_));
        dll_handle_ = nullptr;
    }
    ort_ = nullptr;
    active_provider_ = "CPUExecutionProvider";
}

bool OnnxDetector::is_initialized() const {
    return is_initialized_.load();
}

ModelProfile OnnxDetector::active_profile() const {
    std::lock_guard<std::mutex> lock(session_mutex_);
    return active_profile_;
}

std::string OnnxDetector::active_model_id() const {
    std::lock_guard<std::mutex> lock(session_mutex_);
    return active_profile_.id;
}

std::string OnnxDetector::active_execution_provider() const {
    std::lock_guard<std::mutex> lock(session_mutex_);
    return active_provider_;
}

std::vector<int64_t> OnnxDetector::input_tensor_shape() const {
    std::lock_guard<std::mutex> lock(session_mutex_);
    return input_shape_;
}

std::vector<int64_t> OnnxDetector::output_tensor_shape() const {
    std::lock_guard<std::mutex> lock(session_mutex_);
    return output_shape_;
}

core::Result<std::vector<float>> OnnxDetector::preprocessImage(const core::Frame& frame, LetterboxMeta& out_meta) const {
    if (!frame.is_valid()) {
        return core::Result<std::vector<float>>::err(core::Error(core::ErrorCode::Unsupported, "Invalid input frame"));
    }

    int net_w = 640;
    int net_h = 640;
    int pad_val = 114;
    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        if (input_shape_.size() == 4 && input_shape_[2] > 0 && input_shape_[3] > 0) {
            net_h = static_cast<int>(input_shape_[2]);
            net_w = static_cast<int>(input_shape_[3]);
        } else if (active_profile_.input_size > 0) {
            net_w = active_profile_.input_size;
            net_h = active_profile_.input_size;
        }
        pad_val = active_profile_.pad_value;
    }

    const int orig_w = frame.width;
    const int orig_h = frame.height;
    const int channels = frame.channels;

    const float scale = std::min(static_cast<float>(net_h) / orig_h, static_cast<float>(net_w) / orig_w);
    const int new_unpad_w = std::clamp(static_cast<int>(std::lround(orig_w * scale)), 1, net_w);
    const int new_unpad_h = std::clamp(static_cast<int>(std::lround(orig_h * scale)), 1, net_h);
    const float dw = (net_w - new_unpad_w) * 0.5f;
    const float dh = (net_h - new_unpad_h) * 0.5f;

    const int top = std::clamp(static_cast<int>(std::lround(dh - 0.1f)), 0, net_h - new_unpad_h);
    const int left = std::clamp(static_cast<int>(std::lround(dw - 0.1f)), 0, net_w - new_unpad_w);
    const int bottom = net_h - new_unpad_h - top;
    const int right = net_w - new_unpad_w - left;

    out_meta = LetterboxMeta{
        .orig_w = orig_w,
        .orig_h = orig_h,
        .input_w = net_w,
        .input_h = net_h,
        .new_unpad_w = new_unpad_w,
        .new_unpad_h = new_unpad_h,
        .scale = scale,
        .dw = dw,
        .dh = dh,
        .top = top,
        .bottom = bottom,
        .left = left,
        .right = right
    };

    const float pad_norm = static_cast<float>(pad_val) / 255.0f;
    const size_t plane_size = static_cast<size_t>(net_w) * static_cast<size_t>(net_h);
    std::vector<float> input_tensor(3 * plane_size, pad_norm);

    const float inv_norm = (frame.bit_depth <= 8) ? (1.0f / 255.0f) : (1.0f / 65535.0f);
    const uint16_t* src_px = frame.pixels.get();

    if (orig_w == new_unpad_w && orig_h == new_unpad_h) {
        // Exact 1:1 copy without resampling
        for (int y = 0; y < new_unpad_h; ++y) {
            size_t dst_row = static_cast<size_t>(top + y) * net_w + left;
            size_t src_row = static_cast<size_t>(y) * orig_w;
            for (int x = 0; x < new_unpad_w; ++x) {
                size_t dst_idx = dst_row + x;
                if (channels == 1) {
                    float v = src_px[src_row + x] * inv_norm;
                    input_tensor[0 * plane_size + dst_idx] = v;
                    input_tensor[1 * plane_size + dst_idx] = v;
                    input_tensor[2 * plane_size + dst_idx] = v;
                } else {
                    size_t s = (src_row + x) * channels;
                    input_tensor[0 * plane_size + dst_idx] = src_px[s + 0] * inv_norm;
                    input_tensor[1 * plane_size + dst_idx] = src_px[s + 1] * inv_norm;
                    input_tensor[2 * plane_size + dst_idx] = src_px[s + 2] * inv_norm;
                }
            }
        }
    } else {
        // Bilinear interpolation matching OpenCV INTER_LINEAR area mapping
        const float x_ratio = static_cast<float>(orig_w) / static_cast<float>(new_unpad_w);
        const float y_ratio = static_cast<float>(orig_h) / static_cast<float>(new_unpad_h);

        for (int y = 0; y < new_unpad_h; ++y) {
            float fy = (y + 0.5f) * y_ratio - 0.5f;
            fy = std::clamp(fy, 0.0f, static_cast<float>(orig_h - 1));
            int y0 = static_cast<int>(std::floor(fy));
            int y1 = std::min(y0 + 1, orig_h - 1);
            float wy1 = fy - y0;
            float wy0 = 1.0f - wy1;

            size_t dst_row = static_cast<size_t>(top + y) * net_w + left;

            for (int x = 0; x < new_unpad_w; ++x) {
                float fx = (x + 0.5f) * x_ratio - 0.5f;
                fx = std::clamp(fx, 0.0f, static_cast<float>(orig_w - 1));
                int x0 = static_cast<int>(std::floor(fx));
                int x1 = std::min(x0 + 1, orig_w - 1);
                float wx1 = fx - x0;
                float wx0 = 1.0f - wx1;

                size_t dst_idx = dst_row + x;
                if (channels == 1) {
                    float p00 = src_px[static_cast<size_t>(y0) * orig_w + x0] * inv_norm;
                    float p01 = src_px[static_cast<size_t>(y0) * orig_w + x1] * inv_norm;
                    float p10 = src_px[static_cast<size_t>(y1) * orig_w + x0] * inv_norm;
                    float p11 = src_px[static_cast<size_t>(y1) * orig_w + x1] * inv_norm;
                    float val = wy0 * (wx0 * p00 + wx1 * p01) + wy1 * (wx0 * p10 + wx1 * p11);
                    input_tensor[0 * plane_size + dst_idx] = val;
                    input_tensor[1 * plane_size + dst_idx] = val;
                    input_tensor[2 * plane_size + dst_idx] = val;
                } else {
                    size_t s00 = (static_cast<size_t>(y0) * orig_w + x0) * channels;
                    size_t s01 = (static_cast<size_t>(y0) * orig_w + x1) * channels;
                    size_t s10 = (static_cast<size_t>(y1) * orig_w + x0) * channels;
                    size_t s11 = (static_cast<size_t>(y1) * orig_w + x1) * channels;
                    for (int c = 0; c < 3; ++c) {
                        float p00 = src_px[s00 + c] * inv_norm;
                        float p01 = src_px[s01 + c] * inv_norm;
                        float p10 = src_px[s10 + c] * inv_norm;
                        float p11 = src_px[s11 + c] * inv_norm;
                        float val = wy0 * (wx0 * p00 + wx1 * p01) + wy1 * (wx0 * p10 + wx1 * p11);
                        input_tensor[static_cast<size_t>(c) * plane_size + dst_idx] = val;
                    }
                }
            }
        }
    }

    return core::Result<std::vector<float>>::ok(std::move(input_tensor));
}

core::Result<std::vector<float>> OnnxDetector::runInference(const std::vector<float>& input_tensor, double* out_inference_ms) {
    std::lock_guard<std::mutex> lock(session_mutex_);

    if (!is_initialized_.load() || !session_ || !ort_ || !mem_info_) {
        return core::Result<std::vector<float>>::err(core::Error(core::ErrorCode::InvalidState, "Detector is not initialized"));
    }

    int64_t net_h = (input_shape_.size() == 4 && input_shape_[2] > 0) ? input_shape_[2] : 640;
    int64_t net_w = (input_shape_.size() == 4 && input_shape_[3] > 0) ? input_shape_[3] : 640;
    size_t expected_elems = static_cast<size_t>(1 * 3 * net_h * net_w);
    if (input_tensor.size() != expected_elems) {
        return core::Result<std::vector<float>>::err(core::Error(core::ErrorCode::OutOfRange, "Preprocessed tensor element count mismatch"));
    }

    int64_t input_shape[4] = {1, 3, net_h, net_w};
    OrtValue* input_val = nullptr;
    OrtStatus* st = ort_->CreateTensorWithDataAsOrtValue(
        mem_info_,
        const_cast<float*>(input_tensor.data()),
        input_tensor.size() * sizeof(float),
        input_shape,
        4,
        ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
        &input_val
    );
    if (st) {
        std::string err_msg = ort_->GetErrorMessage(st);
        ort_->ReleaseStatus(st);
        return core::Result<std::vector<float>>::err(core::Error(core::ErrorCode::HardwareError, "CreateTensor failed: " + err_msg));
    }

    const std::string in_name_str = detected_input_name_.empty() ? "images" : detected_input_name_;
    const std::string out_name_str = detected_output_name_.empty() ? "output0" : detected_output_name_;
    const char* input_names[] = {in_name_str.c_str()};
    const char* output_names[] = {out_name_str.c_str()};
    OrtValue* output_val = nullptr;

    auto t_start = std::chrono::steady_clock::now();
    st = ort_->Run(
        session_,
        nullptr,
        input_names,
        const_cast<const OrtValue**>(&input_val),
        1,
        output_names,
        1,
        &output_val
    );
    auto t_end = std::chrono::steady_clock::now();
    if (out_inference_ms) {
        *out_inference_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    }

    ort_->ReleaseValue(input_val);

    if (st) {
        std::string err_msg = ort_->GetErrorMessage(st);
        ort_->ReleaseStatus(st);
        return core::Result<std::vector<float>>::err(core::Error(core::ErrorCode::HardwareError, "Inference Run failed: " + err_msg));
    }

    size_t out_elem_count = 5 * 8400;
    OrtTensorTypeAndShapeInfo* out_info = nullptr;
    if (OrtStatus* sinfo = ort_->GetTensorTypeAndShape(output_val, &out_info); !sinfo && out_info) {
        size_t cnt = 0;
        if (OrtStatus* scnt = ort_->GetTensorShapeElementCount(out_info, &cnt); !scnt && cnt > 0) {
            out_elem_count = cnt;
        } else if (scnt) {
            ort_->ReleaseStatus(scnt);
        }
        ort_->ReleaseTensorTypeAndShapeInfo(out_info);
    } else if (sinfo) {
        ort_->ReleaseStatus(sinfo);
    }

    float* out_data = nullptr;
    st = ort_->GetTensorMutableData(output_val, reinterpret_cast<void**>(&out_data));
    if (st || !out_data) {
        if (st) ort_->ReleaseStatus(st);
        ort_->ReleaseValue(output_val);
        return core::Result<std::vector<float>>::err(core::Error(core::ErrorCode::HardwareError, "GetTensorMutableData failed"));
    }

    std::vector<float> output_copy(out_data, out_data + out_elem_count);
    ort_->ReleaseValue(output_val);

    return core::Result<std::vector<float>>::ok(std::move(output_copy));
}

std::vector<RawCandidate> OnnxDetector::decodeOutput(const float* raw_output, size_t num_anchors, float conf_thresh) const {
    std::vector<RawCandidate> candidates;
    if (!raw_output || num_anchors == 0) return candidates;

    for (size_t i = 0; i < num_anchors; ++i) {
        float conf = raw_output[4 * num_anchors + i];
        if (conf >= conf_thresh) {
            float cx = raw_output[0 * num_anchors + i];
            float cy = raw_output[1 * num_anchors + i];
            float w  = raw_output[2 * num_anchors + i];
            float h  = raw_output[3 * num_anchors + i];

            RawCandidate c;
            c.x0 = cx - w * 0.5f;
            c.y0 = cy - h * 0.5f;
            c.x1 = cx + w * 0.5f;
            c.y1 = cy + h * 0.5f;
            c.confidence = conf;
            c.class_id = 0;
            candidates.push_back(c);
        }
    }
    return candidates;
}

std::vector<RawCandidate> OnnxDetector::applyNMS(const std::vector<RawCandidate>& candidates, float iou_thresh) const {
    std::vector<RawCandidate> sorted = candidates;
    std::sort(sorted.begin(), sorted.end(), [](const RawCandidate& a, const RawCandidate& b) {
        return a.confidence > b.confidence;
    });

    std::vector<bool> suppressed(sorted.size(), false);
    std::vector<RawCandidate> kept;
    kept.reserve(sorted.size());

    for (size_t i = 0; i < sorted.size(); ++i) {
        if (suppressed[i]) continue;
        kept.push_back(sorted[i]);

        for (size_t j = i + 1; j < sorted.size(); ++j) {
            if (suppressed[j]) continue;
            float iou = compute_iou(
                sorted[i].x0, sorted[i].y0, sorted[i].x1, sorted[i].y1,
                sorted[j].x0, sorted[j].y0, sorted[j].x1, sorted[j].y1
            );
            if (iou > iou_thresh) {
                suppressed[j] = true;
            }
        }
    }

    return kept;
}

std::vector<Detection> OnnxDetector::restoreBoundingBoxes(
    const std::vector<RawCandidate>& kept,
    const LetterboxMeta& meta,
    int64_t timestamp_utc_ms) const {

    std::vector<Detection> detections;
    detections.reserve(kept.size());
    const float inv_scale = (meta.scale > 1e-6f) ? (1.0f / meta.scale) : 1.0f;

    for (const auto& c : kept) {
        float unpad_x0 = (c.x0 - meta.dw) * inv_scale;
        float unpad_y0 = (c.y0 - meta.dh) * inv_scale;
        float unpad_x1 = (c.x1 - meta.dw) * inv_scale;
        float unpad_y1 = (c.y1 - meta.dh) * inv_scale;

        float clamped_x0 = std::clamp(unpad_x0, 0.0f, static_cast<float>(meta.orig_w));
        float clamped_y0 = std::clamp(unpad_y0, 0.0f, static_cast<float>(meta.orig_h));
        float clamped_x1 = std::clamp(unpad_x1, 0.0f, static_cast<float>(meta.orig_w));
        float clamped_y1 = std::clamp(unpad_y1, 0.0f, static_cast<float>(meta.orig_h));

        Detection det{};
        det.label = "Hilal";
        det.confidence = c.confidence;
        det.x = clamped_x0;
        det.y = clamped_y0;
        det.width = std::max(0.0f, clamped_x1 - clamped_x0);
        det.height = std::max(0.0f, clamped_y1 - clamped_y0);
        det.timestamp_utc_ms = timestamp_utc_ms;
        detections.push_back(det);
    }

    return detections;
}

core::Result<DetectionResult> OnnxDetector::detect(const core::Frame& frame) {
    if (!is_initialized_.load()) {
        return core::Result<DetectionResult>::err(core::Error(core::ErrorCode::InvalidState, "Detector is not initialized"));
    }

    LetterboxMeta meta{};
    auto prep_res = preprocessImage(frame, meta);
    if (!prep_res.is_ok()) {
        return core::Result<DetectionResult>::err(prep_res.error());
    }

    double inference_ms = 0.0;
    auto inf_res = runInference(prep_res.value(), &inference_ms);
    if (!inf_res.is_ok()) {
        return core::Result<DetectionResult>::err(inf_res.error());
    }

    const auto& out_vec = inf_res.value();
    size_t num_anchors = 8400;
    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        if (output_shape_.size() == 3 && output_shape_[1] >= 5 && output_shape_[2] > 0) {
            num_anchors = static_cast<size_t>(output_shape_[2]);
        } else if (out_vec.size() >= 5 && out_vec.size() % 5 == 0) {
            num_anchors = out_vec.size() / 5;
        }
    }

    const float cur_conf_thresh = conf_threshold_.load();
    const float cur_nms_thresh = nms_threshold_.load();

    auto candidates = decodeOutput(out_vec.data(), num_anchors, cur_conf_thresh);
    auto kept = applyNMS(candidates, cur_nms_thresh);

    const int64_t ts_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        frame.timestamp_utc.time_since_epoch()).count();

    DetectionResult res{};
    res.is_valid = true;
    res.frame_index = frame.seq;
    res.timestamp_utc_ms = ts_ms;
    res.inference_time_ms = inference_ms;
    res.detections = restoreBoundingBoxes(kept, meta, ts_ms);

    return core::Result<DetectionResult>::ok(res);
}

core::Result<core::Frame> OnnxDetector::load_image_file(const std::string& image_path) {
    std::error_code ec;
    if (!std::filesystem::exists(image_path, ec)) {
        return core::Result<core::Frame>::err(core::Error(core::ErrorCode::NotFound, "Image file not found: " + image_path));
    }

    HRESULT hr_com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool did_init_com = SUCCEEDED(hr_com);

    IWICImagingFactory* factory = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory)
    );
    if (FAILED(hr) || !factory) {
        if (did_init_com) CoUninitialize();
        return core::Result<core::Frame>::err(core::Error(core::ErrorCode::HardwareError, "Failed to create WICImagingFactory"));
    }

    std::wstring wpath = to_wstring(image_path);
    IWICBitmapDecoder* decoder = nullptr;
    hr = factory->CreateDecoderFromFilename(wpath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr) || !decoder) {
        factory->Release();
        if (did_init_com) CoUninitialize();
        return core::Result<core::Frame>::err(core::Error(core::ErrorCode::NotFound, "WIC failed to decode image: " + image_path));
    }

    IWICBitmapFrameDecode* frame_dec = nullptr;
    hr = decoder->GetFrame(0, &frame_dec);
    if (FAILED(hr) || !frame_dec) {
        decoder->Release();
        factory->Release();
        if (did_init_com) CoUninitialize();
        return core::Result<core::Frame>::err(core::Error(core::ErrorCode::HardwareError, "WIC GetFrame failed"));
    }

    UINT w = 0;
    UINT h = 0;
    frame_dec->GetSize(&w, &h);

    IWICFormatConverter* converter = nullptr;
    hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr) || !converter) {
        frame_dec->Release();
        decoder->Release();
        factory->Release();
        if (did_init_com) CoUninitialize();
        return core::Result<core::Frame>::err(core::Error(core::ErrorCode::HardwareError, "WIC CreateFormatConverter failed"));
    }

    hr = converter->Initialize(
        frame_dec,
        GUID_WICPixelFormat24bppRGB,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0f,
        WICBitmapPaletteTypeCustom
    );
    if (FAILED(hr)) {
        converter->Release();
        frame_dec->Release();
        decoder->Release();
        factory->Release();
        if (did_init_com) CoUninitialize();
        return core::Result<core::Frame>::err(core::Error(core::ErrorCode::Unsupported, "WIC RGB24 conversion failed"));
    }

    const UINT stride = w * 3;
    std::vector<uint8_t> rgb8(static_cast<size_t>(stride) * h, 0);
    hr = converter->CopyPixels(nullptr, stride, static_cast<UINT>(rgb8.size()), rgb8.data());

    converter->Release();
    frame_dec->Release();
    decoder->Release();
    factory->Release();
    if (did_init_com) CoUninitialize();

    if (FAILED(hr)) {
        return core::Result<core::Frame>::err(core::Error(core::ErrorCode::HardwareError, "WIC CopyPixels failed"));
    }

    core::Frame f = core::Frame::create_allocated(static_cast<int>(w), static_cast<int>(h), 3, 8);
    f.timestamp_utc = core::Clock::now_utc();
    for (size_t i = 0; i < rgb8.size(); ++i) {
        f.pixels[i] = static_cast<uint16_t>(rgb8[i]);
    }
    return core::Result<core::Frame>::ok(std::move(f));
}

void OnnxDetector::start_worker() {
    if (is_running_.load()) return;
    is_running_.store(true);
    worker_thread_ = std::thread(&OnnxDetector::worker_loop, this);
}

void OnnxDetector::stop_worker() {
    if (is_running_.load()) {
        is_running_.store(false);
        frame_cv_.notify_all();
        if (worker_thread_.joinable()) {
            worker_thread_.join();
        }
    }
}

void OnnxDetector::submit_frame(const core::Frame& frame) {
    if (!is_running_.load()) return;
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        pending_frame_ = frame;
    }
    frame_cv_.notify_one();
}

std::vector<Detection> OnnxDetector::get_latest_overlay(int64_t max_age_ms) const {
    std::lock_guard<std::mutex> lock(result_mutex_);
    if (!latest_result_.is_valid) return {};

    auto now = std::chrono::steady_clock::now();
    auto age_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - latest_result_time_).count();
    if (age_ms > max_age_ms) {
        return {};
    }
    return latest_result_.detections;
}

void OnnxDetector::worker_loop() {
    auto last_inference_time = std::chrono::steady_clock::now() - std::chrono::seconds(1);

    while (is_running_.load()) {
        core::Frame frame;
        {
            std::unique_lock<std::mutex> lock(frame_mutex_);
            frame_cv_.wait(lock, [this]() {
                return !is_running_.load() || pending_frame_.has_value();
            });

            if (!is_running_.load()) break;

            if (pending_frame_.has_value()) {
                frame = std::move(*pending_frame_);
                pending_frame_.reset();
            }
        }

        double fps = std::max(0.1, max_fps_.load());
        int min_interval_ms = static_cast<int>(1000.0 / fps);

        auto now = std::chrono::steady_clock::now();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_inference_time).count();
        if (elapsed_ms < min_interval_ms) {
            std::this_thread::sleep_for(std::chrono::milliseconds(min_interval_ms - elapsed_ms));
        }
        last_inference_time = std::chrono::steady_clock::now();

        auto res = detect(frame);
        if (res.is_ok()) {
            {
                std::lock_guard<std::mutex> lock(result_mutex_);
                latest_result_ = res.value();
                latest_result_time_ = std::chrono::steady_clock::now();
            }

            if (on_detection_) {
                on_detection_(frame, res.value());
            }
        }
    }
}

} // namespace sabit::detect
