#ifndef SABIT_APP_SESSION_H
#define SABIT_APP_SESSION_H

#include <string>
#include <mutex>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include "src/camera/icamera.h"

namespace sabit::app {

struct SessionInfo {
    std::string session_id;
    std::string folder_name;
    std::string folder_path;
    std::string location{"Semarang"};
    std::string target{"Hilal"};
    std::string observer{"Sabit Observer"};
    std::string start_time_utc;
    std::string end_time_utc;
    uint64_t frames_captured{0};
    uint64_t frames_recorded{0};
    uint64_t detections_count{0};
    uint64_t disk_usage_bytes{0};
    std::string camera_driver;
    std::string camera_model;
    std::string camera_serial;
    bool is_active{false};
};

class Session {
public:
    Session();
    ~Session();

    bool start(
        const std::string& base_captures_dir,
        const std::string& location,
        const std::string& target,
        const camera::CameraCaps& caps,
        const std::string& observer = "Sabit Observer"
    );

    void finish();

    void update_counts(uint64_t captured, uint64_t recorded, uint64_t detections);
    void write_manifest();
    void log_event(const std::string& message);

    std::string begin_capture_event(const std::string& capture_type, int target_count, const std::string& format);
    void end_capture_event();

    [[nodiscard]] std::string get_frame_filepath(const std::string& ext, uint64_t seq) const;
    [[nodiscard]] std::string get_log_filepath() const;
    [[nodiscard]] std::string get_detections_csv_filepath() const;
    [[nodiscard]] std::string get_manifest_filepath() const;
    [[nodiscard]] std::string get_logs_dir() const;
    [[nodiscard]] std::string get_captures_dir() const;
    [[nodiscard]] std::string get_current_event_dir() const;

    [[nodiscard]] SessionInfo info() const;
    [[nodiscard]] bool is_active() const;

    bool export_diagnostic_zip(const std::string& zip_dest_path, bool mask_coordinates = false) const;

private:
    void calculate_disk_usage();

    mutable std::mutex mutex_;
    SessionInfo info_{};
    std::ofstream log_stream_;
    std::string logs_dir_;
    std::string captures_dir_;
    std::string current_event_dir_;
    std::string current_event_prefix_;
};

} // namespace sabit::app

#endif // SABIT_APP_SESSION_H
