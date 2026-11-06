#ifndef SABIT_UI_PROCESSING_PANEL_H
#define SABIT_UI_PROCESSING_PANEL_H

#include <QWidget>
#include <QCheckBox>
#include <QSlider>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "src/imaging/stacker.h"

namespace sabit::ui {

class ProcessingPanel : public QWidget {
    Q_OBJECT
public:
    explicit ProcessingPanel(QWidget* parent = nullptr);
    ~ProcessingPanel() override = default;

    void apply_hilal_siang_preset();
    void apply_hilal_senja_preset();
    void set_offline_status(const QString& status);
    [[nodiscard]] imaging::StackMethod current_stack_method() const;
    [[nodiscard]] bool is_align_enabled() const;

    void retranslate_ui();

signals:
    void preset_daylight_requested();
    void preset_twilight_requested();
    void auto_stretch_toggled(bool enabled);
    void manual_stretch_changed(double black_pt, double white_pt);
    void clahe_toggled(bool enabled);
    void sharpen_toggled(bool enabled);
    void invert_toggled(bool enabled);
    void stacking_toggled(bool enabled);
    void stack_align_toggled(bool enabled);
    void stack_method_changed(imaging::StackMethod method);
    void reset_stack_clicked();
    void offline_stack_files_requested();
    void offline_stack_folder_requested();
    void open_calibration_wizard_requested();

private:
    // Presets
    QPushButton* preset_siang_btn_{nullptr};
    QPushButton* preset_senja_btn_{nullptr};

    // Stretch
    QPushButton* calibration_wizard_btn_{nullptr};
    QCheckBox* auto_stretch_chk_{nullptr};
    QLabel* b_lbl_{nullptr};
    QSlider* black_slider_{nullptr};
    QLabel* w_lbl_{nullptr};
    QSlider* white_slider_{nullptr};
    QCheckBox* clahe_chk_{nullptr};
    QCheckBox* sharpen_chk_{nullptr};
    QCheckBox* invert_chk_{nullptr};

    // Stacking
    QCheckBox* stacking_chk_{nullptr};
    QCheckBox* align_chk_{nullptr};
    QComboBox* stack_method_combo_{nullptr};
    QPushButton* reset_stack_btn_{nullptr};

    // Offline Stacking
    QPushButton* offline_stack_files_btn_{nullptr};
    QPushButton* offline_stack_folder_btn_{nullptr};
    QLabel* offline_stack_status_lbl_{nullptr};
};

} // namespace sabit::ui

#endif // SABIT_UI_PROCESSING_PANEL_H
