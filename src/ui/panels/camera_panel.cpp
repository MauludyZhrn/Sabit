#include "src/ui/panels/camera_panel.h"
#include "src/ui/fluent/fluent_toggle_switch.h"
#include "src/ui/theme/theme_manager.h"
#include "src/ui/i18n/translator.h"
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace sabit::ui {

CameraPanel::CameraPanel(QWidget* parent) : QWidget(parent) {
    apply_debounce_timer_ = new QTimer(this);
    apply_debounce_timer_->setSingleShot(true);
    apply_debounce_timer_->setInterval(30);
    connect(apply_debounce_timer_, &QTimer::timeout, this, &CameraPanel::emit_current_settings);

    create_exposure_ui();
    create_gain_wb_ui();

    // Default container layout (if used as a single widget)
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(exposure_container_);
    layout->addWidget(gain_wb_container_);
}

void CameraPanel::create_exposure_ui() {
    exposure_container_ = new QWidget(this);
    auto layout = new QVBoxLayout(exposure_container_);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    driver_info_lbl_ = new MarqueeLabel("Driver: Standby (No Camera)", exposure_container_);
    driver_info_lbl_->setObjectName("FieldLabel");
    layout->addWidget(driver_info_lbl_);

    auto row1 = new QHBoxLayout();
    row1->setContentsMargins(0, 0, 0, 0);

    exp_val_lbl_ = new QLabel(QString("%1: 33 ms").arg(tr_str("exposure")), exposure_container_);
    exp_val_lbl_->setObjectName("FieldValue");

    auto_exp_chk_ = new FluentToggleSwitch(tr_str("auto"), exposure_container_);
    auto_exp_chk_->setToolTip(tr_str("auto_exp_tooltip"));

    row1->addWidget(exp_val_lbl_);
    row1->addStretch();
    row1->addWidget(auto_exp_chk_);
    layout->addLayout(row1);

    exp_slider_ = new QSlider(Qt::Horizontal, exposure_container_);
    exp_slider_->setRange(1, 2000); // 1 ms to 2000 ms
    exp_slider_->setValue(33);
    layout->addWidget(exp_slider_);

    connect(exp_slider_, &QSlider::valueChanged, this, [this](int v) {
        if (v >= 1000) {
            exp_val_lbl_->setText(QString("%1: %2 s").arg(tr_str("exposure")).arg(QString::number(v / 1000.0, 'f', 2)));
        } else {
            exp_val_lbl_->setText(QString("%1: %2 ms").arg(tr_str("exposure")).arg(v));
        }
        on_controls_edited();
    });

    connect(exp_slider_, &QSlider::sliderReleased, this, [this]() {
        if (apply_debounce_timer_ && apply_debounce_timer_->isActive()) {
            apply_debounce_timer_->stop();
        }
        emit_current_settings();
    });

    connect(auto_exp_chk_, &QCheckBox::toggled, this, [this](bool checked) {
        exp_slider_->setEnabled(!checked && current_caps_.capabilities.can_control_exposure);
        if (apply_debounce_timer_ && apply_debounce_timer_->isActive()) {
            apply_debounce_timer_->stop();
        }
        emit_current_settings();
    });

    dslr_crop_chk_ = new FluentToggleSwitch(tr_str("dslr_32_crop"), exposure_container_);
    dslr_crop_chk_->setToolTip(tr_str("dslr_32_crop_tooltip"));
    dslr_crop_chk_->setVisible(false);
    layout->addWidget(dslr_crop_chk_);

    connect(dslr_crop_chk_, &QCheckBox::toggled, this, [this](bool) {
        if (apply_debounce_timer_ && apply_debounce_timer_->isActive()) {
            apply_debounce_timer_->stop();
        }
        emit_current_settings();
    });
}

void CameraPanel::create_gain_wb_ui() {
    gain_wb_container_ = new QWidget(this);
    auto layout = new QVBoxLayout(gain_wb_container_);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // 1. Gain Header Row
    auto gain_top_row = new QHBoxLayout();
    gain_top_row->setContentsMargins(0, 0, 0, 0);

    gain_val_lbl_ = new QLabel(QString("%1: 50 dB").arg(tr_str("gain")), gain_wb_container_);
    gain_val_lbl_->setObjectName("FieldValue");

    auto_gain_chk_ = new FluentToggleSwitch(tr_str("auto"), gain_wb_container_);
    auto_gain_chk_->setToolTip(tr_str("auto_gain_tooltip"));

    gain_top_row->addWidget(gain_val_lbl_);
    gain_top_row->addStretch();
    gain_top_row->addWidget(auto_gain_chk_);
    layout->addLayout(gain_top_row);

    // 2. Gain Slider (Full Width)
    gain_slider_ = new QSlider(Qt::Horizontal, gain_wb_container_);
    gain_slider_->setRange(0, 600);
    gain_slider_->setValue(50);
    layout->addWidget(gain_slider_);

    // 3. W-Red Row (Label + Slider)
    auto w_red_row = new QHBoxLayout();
    w_red_row->setContentsMargins(0, 0, 0, 0);
    w_red_row->setSpacing(8);

    w_red_lbl_ = new QLabel(QString("%1 (50%)").arg(tr_str("w_red")), gain_wb_container_);
    w_red_lbl_->setObjectName("FieldLabel");
    w_red_lbl_->setFixedWidth(84);

    w_red_slider_ = new QSlider(Qt::Horizontal, gain_wb_container_);
    w_red_slider_->setRange(0, 100);
    w_red_slider_->setValue(50);

    w_red_row->addWidget(w_red_lbl_);
    w_red_row->addWidget(w_red_slider_, 1);
    layout->addLayout(w_red_row);

    // 4. W-Blue Row (Label + Slider)
    auto w_blue_row = new QHBoxLayout();
    w_blue_row->setContentsMargins(0, 0, 0, 0);
    w_blue_row->setSpacing(8);

    w_blue_lbl_ = new QLabel(QString("%1 (50%)").arg(tr_str("w_blue")), gain_wb_container_);
    w_blue_lbl_->setObjectName("FieldLabel");
    w_blue_lbl_->setFixedWidth(84);

    w_blue_slider_ = new QSlider(Qt::Horizontal, gain_wb_container_);
    w_blue_slider_->setRange(0, 100);
    w_blue_slider_->setValue(50);

    w_blue_row->addWidget(w_blue_lbl_);
    w_blue_row->addWidget(w_blue_slider_, 1);
    layout->addLayout(w_blue_row);

    connect(gain_slider_, &QSlider::valueChanged, this, [this](int v) {
        gain_val_lbl_->setText(QString("%1: %2 dB").arg(tr_str("gain")).arg(v));
        on_controls_edited();
    });

    connect(gain_slider_, &QSlider::sliderReleased, this, [this]() {
        if (apply_debounce_timer_ && apply_debounce_timer_->isActive()) {
            apply_debounce_timer_->stop();
        }
        emit_current_settings();
    });

    connect(auto_gain_chk_, &QCheckBox::toggled, this, [this](bool checked) {
        gain_slider_->setEnabled(!checked);
        if (apply_debounce_timer_ && apply_debounce_timer_->isActive()) {
            apply_debounce_timer_->stop();
        }
        emit_current_settings();
    });

    connect(w_red_slider_, &QSlider::valueChanged, this, [this](int v) {
        if (w_red_lbl_) w_red_lbl_->setText(QString("%1 (%2%)").arg(tr_str("w_red")).arg(v));
        on_controls_edited();
    });

    connect(w_red_slider_, &QSlider::sliderReleased, this, [this]() {
        if (apply_debounce_timer_ && apply_debounce_timer_->isActive()) {
            apply_debounce_timer_->stop();
        }
        emit_current_settings();
    });

    connect(w_blue_slider_, &QSlider::valueChanged, this, [this](int v) {
        if (w_blue_lbl_) w_blue_lbl_->setText(QString("%1 (%2%)").arg(tr_str("w_blue")).arg(v));
        on_controls_edited();
    });

    connect(w_blue_slider_, &QSlider::sliderReleased, this, [this]() {
        if (apply_debounce_timer_ && apply_debounce_timer_->isActive()) {
            apply_debounce_timer_->stop();
        }
        emit_current_settings();
    });
}

void CameraPanel::retranslate_ui() {
    if (auto_exp_chk_) {
        auto_exp_chk_->setText(tr_str("auto"));
        auto_exp_chk_->setToolTip(tr_str("auto_exp_tooltip"));
    }
    if (exp_slider_ && exp_val_lbl_) {
        int v = exp_slider_->value();
        if (v >= 1000) {
            exp_val_lbl_->setText(QString("%1: %2 s").arg(tr_str("exposure")).arg(QString::number(v / 1000.0, 'f', 2)));
        } else {
            exp_val_lbl_->setText(QString("%1: %2 ms").arg(tr_str("exposure")).arg(v));
        }
    }
    if (auto_gain_chk_) {
        auto_gain_chk_->setText(tr_str("auto"));
        auto_gain_chk_->setToolTip(tr_str("auto_gain_tooltip"));
    }
    if (gain_slider_ && gain_val_lbl_) {
        gain_val_lbl_->setText(QString("%1: %2 dB").arg(tr_str("gain")).arg(gain_slider_->value()));
    }
    if (w_red_lbl_ && w_red_slider_) {
        w_red_lbl_->setText(QString("%1 (%2%)").arg(tr_str("w_red")).arg(w_red_slider_->value()));
    }
    if (w_blue_lbl_ && w_blue_slider_) {
        w_blue_lbl_->setText(QString("%1 (%2%)").arg(tr_str("w_blue")).arg(w_blue_slider_->value()));
    }
    if (dslr_crop_chk_) {
        dslr_crop_chk_->setText(tr_str("dslr_32_crop"));
        dslr_crop_chk_->setToolTip(tr_str("dslr_32_crop_tooltip"));
    }
}

void CameraPanel::reset_to_defaults() {
    if (apply_debounce_timer_ && apply_debounce_timer_->isActive()) {
        apply_debounce_timer_->stop();
    }
    updating_ui_ = true;
    bool is_uvc = (current_caps_.driver == "uvc");
    int def_exp_ms = is_uvc ? 33 : 100;
    int def_gain = is_uvc ? 0 : 100;

    exp_slider_->setValue(def_exp_ms);
    exp_val_lbl_->setText(QString("%1: %2 ms").arg(tr_str("exposure")).arg(def_exp_ms));
    auto_exp_chk_->setChecked(false);
    exp_slider_->setEnabled(current_caps_.capabilities.can_control_exposure);

    gain_slider_->setValue(std::clamp(def_gain, gain_slider_->minimum(), gain_slider_->maximum()));
    gain_val_lbl_->setText(QString("%1: %2 dB").arg(tr_str("gain")).arg(gain_slider_->value()));
    auto_gain_chk_->setChecked(false);
    gain_slider_->setEnabled(current_caps_.capabilities.can_control_gain);

    w_red_slider_->setValue(50);
    w_blue_slider_->setValue(50);
    if (w_red_lbl_) w_red_lbl_->setText(QString("%1 (50%)").arg(tr_str("w_red")));
    if (w_blue_lbl_) w_blue_lbl_->setText(QString("%1 (50%)").arg(tr_str("w_blue")));
    updating_ui_ = false;

    emit_current_settings();
}

void CameraPanel::apply_preset(int64_t exp_us, double gain) {
    if (apply_debounce_timer_ && apply_debounce_timer_->isActive()) {
        apply_debounce_timer_->stop();
    }
    updating_ui_ = true;
    int exp_ms = static_cast<int>(exp_us / 1000);
    if (exp_slider_) {
        exp_slider_->setValue(exp_ms);
        exp_slider_->setEnabled(current_caps_.capabilities.can_control_exposure);
    }
    if (exp_val_lbl_) {
        if (exp_ms >= 1000) {
            exp_val_lbl_->setText(QString("%1: %2 s").arg(tr_str("exposure")).arg(QString::number(exp_ms / 1000.0, 'f', 2)));
        } else {
            exp_val_lbl_->setText(QString("%1: %2 ms").arg(tr_str("exposure")).arg(exp_ms));
        }
    }
    if (auto_exp_chk_) {
        auto_exp_chk_->setChecked(false);
    }

    if (gain_slider_) {
        gain_slider_->setValue(static_cast<int>(gain));
        gain_slider_->setEnabled(current_caps_.capabilities.can_control_gain);
    }
    if (gain_val_lbl_) {
        gain_val_lbl_->setText(QString("%1: %2 dB").arg(tr_str("gain")).arg(static_cast<int>(gain)));
    }
    if (auto_gain_chk_) {
        auto_gain_chk_->setChecked(false);
    }

    updating_ui_ = false;
    emit_current_settings();
}

void CameraPanel::set_caps(const camera::CameraCaps& caps) {
    if (apply_debounce_timer_ && apply_debounce_timer_->isActive()) {
        apply_debounce_timer_->stop();
    }
    updating_ui_ = true;
    is_connected_ = true;
    current_caps_ = caps;

    int min_exp_ms = std::max<int>(1, static_cast<int>(caps.min_exposure_us / 1000));
    int max_exp_ms = std::clamp<int>(static_cast<int>(caps.max_exposure_us / 1000), min_exp_ms + 1, 60000);
    exp_slider_->setRange(min_exp_ms, max_exp_ms);
    exp_slider_->setEnabled(true);

    int min_g = static_cast<int>(caps.min_gain);
    int max_g = std::max<int>(min_g + 1, static_cast<int>(caps.max_gain));
    gain_slider_->setRange(min_g, max_g);
    gain_slider_->setEnabled(true);

    auto_exp_chk_->setEnabled(caps.capabilities.can_auto_exposure);
    auto_exp_chk_->setVisible(caps.capabilities.can_auto_exposure);
    auto_gain_chk_->setEnabled(caps.capabilities.can_auto_gain);
    auto_gain_chk_->setVisible(caps.capabilities.can_auto_gain);

    if (dslr_crop_chk_) {
        dslr_crop_chk_->setVisible(caps.capabilities.can_crop_dslr_aspect);
        dslr_crop_chk_->setChecked(false);
    }

    if (driver_info_lbl_) {
        QString drv = QString::fromStdString(caps.driver).toUpper();
        QString mode = caps.is_color ? "RGB Color" : "Mono 16-bit";
        QString hw_str = caps.capabilities.has_hardware_exposure ? "HW-Exp" : "SW-Trim";
        driver_info_lbl_->setText(QString("%1 [%2 | %3 | %4 | %5x%6]")
            .arg(QString::fromStdString(caps.model))
            .arg(drv)
            .arg(mode)
            .arg(hw_str)
            .arg(caps.max_width)
            .arg(caps.max_height));
    }

    int def_exp = (caps.driver == "uvc") ? 33 : 100;
    int def_gain = (caps.driver == "uvc") ? 0 : 100;

    exp_slider_->setValue(std::clamp(def_exp, min_exp_ms, max_exp_ms));
    exp_val_lbl_->setText(QString("%1: %2 ms").arg(tr_str("exposure")).arg(exp_slider_->value()));
    auto_exp_chk_->setChecked(false);
    exp_slider_->setEnabled(caps.capabilities.can_control_exposure);

    gain_slider_->setValue(std::clamp(def_gain, min_g, max_g));
    gain_val_lbl_->setText(QString("%1: %2 dB").arg(tr_str("gain")).arg(gain_slider_->value()));
    auto_gain_chk_->setChecked(false);
    gain_slider_->setEnabled(caps.capabilities.can_control_gain);

    if (caps.is_color) {
        if (w_red_slider_) {
            w_red_slider_->setEnabled(true);
            w_red_slider_->setValue(50);
        }
        if (w_blue_slider_) {
            w_blue_slider_->setEnabled(true);
            w_blue_slider_->setValue(50);
        }
        if (w_red_lbl_) {
            w_red_lbl_->setEnabled(true);
            w_red_lbl_->setText(QString("%1 (50%)").arg(tr_str("w_red")));
        }
        if (w_blue_lbl_) {
            w_blue_lbl_->setEnabled(true);
            w_blue_lbl_->setText(QString("%1 (50%)").arg(tr_str("w_blue")));
        }
    } else {
        if (w_red_slider_) w_red_slider_->setEnabled(false);
        if (w_blue_slider_) w_blue_slider_->setEnabled(false);
        if (w_red_lbl_) {
            w_red_lbl_->setEnabled(false);
            w_red_lbl_->setText(QString("%1 (Mono)").arg(tr_str("w_red")));
        }
        if (w_blue_lbl_) {
            w_blue_lbl_->setEnabled(false);
            w_blue_lbl_->setText(QString("%1 (Mono)").arg(tr_str("w_blue")));
        }
    }

    updating_ui_ = false;
}

void CameraPanel::set_disconnected() {
    if (apply_debounce_timer_ && apply_debounce_timer_->isActive()) {
        apply_debounce_timer_->stop();
    }
    updating_ui_ = true;
    is_connected_ = false;
    current_caps_ = {};

    if (driver_info_lbl_) {
        driver_info_lbl_->setText("Driver: Standby (No Camera Connected)");
    }

    if (exp_slider_) {
        exp_slider_->setRange(1, 2000);
        exp_slider_->setValue(100);
        exp_slider_->setEnabled(false);
    }
    if (exp_val_lbl_) {
        exp_val_lbl_->setText(QString("%1: 100 ms").arg(tr_str("exposure")));
    }
    if (auto_exp_chk_) {
        auto_exp_chk_->setChecked(false);
        auto_exp_chk_->setEnabled(false);
    }

    if (gain_slider_) {
        gain_slider_->setRange(0, 600);
        gain_slider_->setValue(0);
        gain_slider_->setEnabled(false);
    }
    if (gain_val_lbl_) {
        gain_val_lbl_->setText(QString("%1: 0 dB").arg(tr_str("gain")));
    }
    if (auto_gain_chk_) {
        auto_gain_chk_->setChecked(false);
        auto_gain_chk_->setEnabled(false);
    }
    if (dslr_crop_chk_) {
        dslr_crop_chk_->setChecked(false);
        dslr_crop_chk_->setVisible(false);
    }

    if (w_red_slider_) {
        w_red_slider_->setValue(50);
        w_red_slider_->setEnabled(false);
    }
    if (w_blue_slider_) {
        w_blue_slider_->setValue(50);
        w_blue_slider_->setEnabled(false);
    }
    if (w_red_lbl_) {
        w_red_lbl_->setEnabled(false);
        w_red_lbl_->setText(QString("%1 (50%)").arg(tr_str("w_red")));
    }
    if (w_blue_lbl_) {
        w_blue_lbl_->setEnabled(false);
        w_blue_lbl_->setText(QString("%1 (50%)").arg(tr_str("w_blue")));
    }

    updating_ui_ = false;
}

void CameraPanel::on_controls_edited() {
    if (updating_ui_ || !is_connected_) return;
    if (apply_debounce_timer_) {
        apply_debounce_timer_->start(30);
    } else {
        emit_current_settings();
    }
}

void CameraPanel::emit_current_settings() {
    if (updating_ui_ || !is_connected_) return;

    int64_t exp_us = static_cast<int64_t>(exp_slider_->value()) * 1000LL;
    double gain = static_cast<double>(gain_slider_->value());
    int bin = 1;

    camera::CameraSettings s;
    s.exposure_us = exp_us;
    s.gain = gain;
    s.binning = bin;
    s.bit_depth = 16;
    if (w_red_slider_) s.wb_red = w_red_slider_->value();
    if (w_blue_slider_) s.wb_blue = w_blue_slider_->value();
    s.auto_exposure = (auto_exp_chk_ && auto_exp_chk_->isVisible()) ? auto_exp_chk_->isChecked() : false;
    s.auto_gain = (auto_gain_chk_ && auto_gain_chk_->isVisible()) ? auto_gain_chk_->isChecked() : false;
    if (dslr_crop_chk_ && dslr_crop_chk_->isVisible()) s.crop_dslr_pillarbox = dslr_crop_chk_->isChecked();

    emit full_settings_changed(s);
}

void CameraPanel::update_readback(const camera::CameraStatus& status, int64_t exp_us, double gain, int binning) {
    (void)status;
    (void)binning;

    if (!is_connected_ || updating_ui_) return;

    // Only sync slider positions from hardware readback when Auto Exposure / Auto Gain is explicitly ON.
    // When Auto is OFF (manual control), sliders must never move by themselves.
    const bool is_auto_exp = (auto_exp_chk_ && auto_exp_chk_->isVisible() && auto_exp_chk_->isChecked());
    if (is_auto_exp && exp_slider_ && !exp_slider_->isSliderDown()) {
        int exp_ms = static_cast<int>(exp_us / 1000);
        if (std::abs(exp_slider_->value() - exp_ms) > 2) {
            updating_ui_ = true;
            exp_slider_->setValue(exp_ms);
            if (exp_ms >= 1000) {
                exp_val_lbl_->setText(QString("%1: %2 s").arg(tr_str("exposure")).arg(QString::number(exp_ms / 1000.0, 'f', 2)));
            } else {
                exp_val_lbl_->setText(QString("%1: %2 ms").arg(tr_str("exposure")).arg(exp_ms));
            }
            updating_ui_ = false;
        }
    }

    const bool is_auto_gain = (auto_gain_chk_ && auto_gain_chk_->isVisible() && auto_gain_chk_->isChecked());
    if (is_auto_gain && gain_slider_ && !gain_slider_->isSliderDown()) {
        int g_val = static_cast<int>(gain);
        if (std::abs(gain_slider_->value() - g_val) > 1) {
            updating_ui_ = true;
            gain_slider_->setValue(g_val);
            gain_val_lbl_->setText(QString("%1: %2 dB").arg(tr_str("gain")).arg(g_val));
            updating_ui_ = false;
        }
    }
}

} // namespace sabit::ui
