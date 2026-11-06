#include "src/ui/panels/processing_panel.h"
#include "src/ui/fluent/fluent_toggle_switch.h"
#include "src/ui/i18n/translator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

namespace sabit::ui {

ProcessingPanel::ProcessingPanel(QWidget* parent) : QWidget(parent) {
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(8);

    // 0. Preset Observasi Cepat (Hilal Siang & Hilal Senja)
    auto preset_grp = new QWidget(this);
    auto p_layout = new QHBoxLayout(preset_grp);
    p_layout->setContentsMargins(0, 0, 0, 0);
    p_layout->setSpacing(6);

    preset_siang_btn_ = new QPushButton(tr_str("preset_daylight"), this);
    preset_siang_btn_->setToolTip(tr_str("preset_daylight_tip"));
    preset_siang_btn_->setMinimumHeight(28);

    preset_senja_btn_ = new QPushButton(tr_str("preset_twilight"), this);
    preset_senja_btn_->setToolTip(tr_str("preset_twilight_tip"));
    preset_senja_btn_->setMinimumHeight(28);

    p_layout->addWidget(preset_siang_btn_);
    p_layout->addWidget(preset_senja_btn_);
    layout->addWidget(preset_grp);

    // 1. Stretch Controls
    auto stretch_grp = new QWidget(this);
    auto s_layout = new QVBoxLayout(stretch_grp);
    s_layout->setContentsMargins(0, 0, 0, 0);
    s_layout->setSpacing(4);

    auto_stretch_chk_ = new FluentToggleSwitch(tr_str("auto_stretch"), this);
    auto_stretch_chk_->setChecked(true);
    s_layout->addWidget(auto_stretch_chk_);

    b_lbl_ = new QLabel(QString("%1 0%").arg(tr_str("black_point")), this);
    b_lbl_->setObjectName("FieldLabel");
    black_slider_ = new QSlider(Qt::Horizontal, this);
    black_slider_->setRange(0, 1000);
    black_slider_->setValue(0);
    black_slider_->setEnabled(false);

    w_lbl_ = new QLabel(QString("%1 100%").arg(tr_str("white_point")), this);
    w_lbl_->setObjectName("FieldLabel");
    white_slider_ = new QSlider(Qt::Horizontal, this);
    white_slider_->setRange(0, 1000);
    white_slider_->setValue(1000);
    white_slider_->setEnabled(false);

    s_layout->addWidget(b_lbl_);
    s_layout->addWidget(black_slider_);
    s_layout->addWidget(w_lbl_);
    s_layout->addWidget(white_slider_);

    layout->addWidget(stretch_grp);

    // 2. Visual Enhancements (preview-only)
    clahe_chk_ = new FluentToggleSwitch(tr_str("clahe"), this);
    sharpen_chk_ = new FluentToggleSwitch(tr_str("sharpen"), this);
    invert_chk_ = new FluentToggleSwitch(tr_str("invert"), this);

    layout->addWidget(clahe_chk_);
    layout->addWidget(sharpen_chk_);
    layout->addWidget(invert_chk_);

    // 3. Stacking
    auto stack_box = new QHBoxLayout();
    stacking_chk_ = new FluentToggleSwitch(tr_str("stacking"), this);
    stack_method_combo_ = new QComboBox(this);
    stack_method_combo_->addItem(tr_str("stack_mean"), static_cast<int>(imaging::StackMethod::Mean));
    stack_method_combo_->addItem(tr_str("stack_sum"), static_cast<int>(imaging::StackMethod::Sum));
    stack_method_combo_->addItem(tr_str("stack_median"), static_cast<int>(imaging::StackMethod::Median));
    stack_method_combo_->addItem(tr_str("stack_sigma"), static_cast<int>(imaging::StackMethod::SigmaClip));

    reset_stack_btn_ = new QPushButton(tr_str("reset"), this);
    reset_stack_btn_->setMinimumWidth(48);
    reset_stack_btn_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    stack_box->addWidget(stacking_chk_);
    stack_box->addWidget(stack_method_combo_);
    stack_box->addWidget(reset_stack_btn_);
    layout->addLayout(stack_box);

    align_chk_ = new FluentToggleSwitch(tr_str("stack_align"), this);
    align_chk_->setToolTip(tr_str("stack_align_tip"));
    align_chk_->setChecked(true);
    layout->addWidget(align_chk_);

    // 3.1 Offline Stacking (File / Folder)
    auto offline_box = new QWidget(this);
    auto off_layout = new QVBoxLayout(offline_box);
    off_layout->setContentsMargins(0, 2, 0, 2);
    off_layout->setSpacing(4);

    auto off_btn_layout = new QHBoxLayout();
    off_btn_layout->setContentsMargins(0, 0, 0, 0);
    off_btn_layout->setSpacing(6);

    offline_stack_files_btn_ = new QPushButton(tr_str("offline_stack_files"), this);
    offline_stack_files_btn_->setToolTip(tr_str("offline_stack_files_tip"));
    offline_stack_files_btn_->setMinimumHeight(26);

    offline_stack_folder_btn_ = new QPushButton(tr_str("offline_stack_folder"), this);
    offline_stack_folder_btn_->setToolTip(tr_str("offline_stack_folder_tip"));
    offline_stack_folder_btn_->setMinimumHeight(26);

    off_btn_layout->addWidget(offline_stack_files_btn_);
    off_btn_layout->addWidget(offline_stack_folder_btn_);
    off_layout->addLayout(off_btn_layout);

    offline_stack_status_lbl_ = new QLabel(tr_str("offline_stack_idle"), this);
    offline_stack_status_lbl_->setObjectName("FieldLabel");
    offline_stack_status_lbl_->setWordWrap(true);
    off_layout->addWidget(offline_stack_status_lbl_);

    layout->addWidget(offline_box);

    // 4. Calibration Wizard (SCR-06)
    calibration_wizard_btn_ = new QPushButton(tr_str("calibration_wizard_btn"), this);
    calibration_wizard_btn_->setMinimumHeight(30);
    layout->addWidget(calibration_wizard_btn_);

    // Connections
    connect(preset_siang_btn_, &QPushButton::clicked, this, &ProcessingPanel::apply_hilal_siang_preset);
    connect(preset_senja_btn_, &QPushButton::clicked, this, &ProcessingPanel::apply_hilal_senja_preset);

    connect(auto_stretch_chk_, &QCheckBox::toggled, this, [this](bool checked) {
        black_slider_->setEnabled(!checked);
        white_slider_->setEnabled(!checked);
        emit auto_stretch_toggled(checked);
    });

    auto on_manual_slider = [this]() {
        double bp = black_slider_->value() / 1000.0;
        double wp = white_slider_->value() / 1000.0;
        if (bp >= wp) wp = bp + 0.001;
        b_lbl_->setText(QString("%1 %2%").arg(tr_str("black_point")).arg(static_cast<int>(bp * 100.0)));
        w_lbl_->setText(QString("%1 %2%").arg(tr_str("white_point")).arg(static_cast<int>(wp * 100.0)));
        emit manual_stretch_changed(bp, wp);
    };

    connect(black_slider_, &QSlider::valueChanged, this, on_manual_slider);
    connect(white_slider_, &QSlider::valueChanged, this, on_manual_slider);

    connect(clahe_chk_, &QCheckBox::toggled, this, &ProcessingPanel::clahe_toggled);
    connect(sharpen_chk_, &QCheckBox::toggled, this, &ProcessingPanel::sharpen_toggled);
    connect(invert_chk_, &QCheckBox::toggled, this, &ProcessingPanel::invert_toggled);

    connect(stacking_chk_, &QCheckBox::toggled, this, &ProcessingPanel::stacking_toggled);
    connect(align_chk_, &QCheckBox::toggled, this, &ProcessingPanel::stack_align_toggled);
    connect(stack_method_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        auto m = static_cast<imaging::StackMethod>(stack_method_combo_->itemData(idx).toInt());
        emit stack_method_changed(m);
    });
    connect(reset_stack_btn_, &QPushButton::clicked, this, &ProcessingPanel::reset_stack_clicked);

    connect(offline_stack_files_btn_, &QPushButton::clicked, this, &ProcessingPanel::offline_stack_files_requested);
    connect(offline_stack_folder_btn_, &QPushButton::clicked, this, &ProcessingPanel::offline_stack_folder_requested);

    connect(calibration_wizard_btn_, &QPushButton::clicked, this, &ProcessingPanel::open_calibration_wizard_requested);
}

void ProcessingPanel::apply_hilal_siang_preset() {
    auto_stretch_chk_->setChecked(true);
    clahe_chk_->setChecked(true);
    sharpen_chk_->setChecked(true);
    invert_chk_->setChecked(false);
    emit preset_daylight_requested();
}

void ProcessingPanel::apply_hilal_senja_preset() {
    auto_stretch_chk_->setChecked(false);
    black_slider_->setEnabled(true);
    white_slider_->setEnabled(true);
    black_slider_->setValue(157); // 40/255 ~ 15.7% -> 157/1000
    white_slider_->setValue(824); // 210/255 ~ 82.4% -> 824/1000
    b_lbl_->setText(QString("%1 %2%").arg(tr_str("black_point")).arg(15));
    w_lbl_->setText(QString("%1 %2%").arg(tr_str("white_point")).arg(82));
    clahe_chk_->setChecked(true);
    sharpen_chk_->setChecked(false);
    invert_chk_->setChecked(false);
    emit manual_stretch_changed(0.157, 0.824);
    emit preset_twilight_requested();
}

void ProcessingPanel::set_offline_status(const QString& status) {
    if (offline_stack_status_lbl_) {
        offline_stack_status_lbl_->setText(status);
    }
}

imaging::StackMethod ProcessingPanel::current_stack_method() const {
    if (!stack_method_combo_) return imaging::StackMethod::Mean;
    int idx = stack_method_combo_->currentIndex();
    return static_cast<imaging::StackMethod>(stack_method_combo_->itemData(idx).toInt());
}

bool ProcessingPanel::is_align_enabled() const {
    return align_chk_ ? align_chk_->isChecked() : true;
}

void ProcessingPanel::retranslate_ui() {
    if (preset_siang_btn_) {
        preset_siang_btn_->setText(tr_str("preset_daylight"));
        preset_siang_btn_->setToolTip(tr_str("preset_daylight_tip"));
    }
    if (preset_senja_btn_) {
        preset_senja_btn_->setText(tr_str("preset_twilight"));
        preset_senja_btn_->setToolTip(tr_str("preset_twilight_tip"));
    }
    if (auto_stretch_chk_) auto_stretch_chk_->setText(tr_str("auto_stretch"));
    if (b_lbl_ && black_slider_) {
        double bp = black_slider_->value() / 1000.0;
        b_lbl_->setText(QString("%1 %2%").arg(tr_str("black_point")).arg(static_cast<int>(bp * 100.0)));
    }
    if (w_lbl_ && white_slider_) {
        double wp = white_slider_->value() / 1000.0;
        w_lbl_->setText(QString("%1 %2%").arg(tr_str("white_point")).arg(static_cast<int>(wp * 100.0)));
    }
    if (clahe_chk_) clahe_chk_->setText(tr_str("clahe"));
    if (sharpen_chk_) sharpen_chk_->setText(tr_str("sharpen"));
    if (invert_chk_) invert_chk_->setText(tr_str("invert"));
    if (stacking_chk_) stacking_chk_->setText(tr_str("stacking"));
    if (align_chk_) {
        align_chk_->setText(tr_str("stack_align"));
        align_chk_->setToolTip(tr_str("stack_align_tip"));
    }
    if (stack_method_combo_) {
        int cur_idx = stack_method_combo_->currentIndex();
        stack_method_combo_->setItemText(0, tr_str("stack_mean"));
        stack_method_combo_->setItemText(1, tr_str("stack_sum"));
        stack_method_combo_->setItemText(2, tr_str("stack_median"));
        stack_method_combo_->setItemText(3, tr_str("stack_sigma"));
        stack_method_combo_->setCurrentIndex(cur_idx);
    }
    if (reset_stack_btn_) reset_stack_btn_->setText(tr_str("reset"));
    if (offline_stack_files_btn_) {
        offline_stack_files_btn_->setText(tr_str("offline_stack_files"));
        offline_stack_files_btn_->setToolTip(tr_str("offline_stack_files_tip"));
    }
    if (offline_stack_folder_btn_) {
        offline_stack_folder_btn_->setText(tr_str("offline_stack_folder"));
        offline_stack_folder_btn_->setToolTip(tr_str("offline_stack_folder_tip"));
    }
    if (calibration_wizard_btn_) calibration_wizard_btn_->setText(tr_str("calibration_wizard_btn"));
}

} // namespace sabit::ui
