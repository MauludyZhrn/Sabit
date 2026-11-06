#ifndef SABIT_UI_HISTOGRAM_WIDGET_H
#define SABIT_UI_HISTOGRAM_WIDGET_H

#include <QWidget>
#include <vector>
#include <cstdint>

namespace sabit::ui {

class HistogramWidget : public QWidget {
    Q_OBJECT
public:
    explicit HistogramWidget(QWidget* parent = nullptr);
    ~HistogramWidget() override = default;

    void update_histogram(const std::vector<uint32_t>& bins_256);
    void update_histogram_rgb(const std::vector<uint32_t>& bins_r,
                              const std::vector<uint32_t>& bins_g,
                              const std::vector<uint32_t>& bins_b,
                              const std::vector<uint32_t>& bins_lum = {});
    void set_stretch_points(double black_pt, double white_pt);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::vector<uint32_t> bins_;
    std::vector<uint32_t> bins_r_;
    std::vector<uint32_t> bins_g_;
    std::vector<uint32_t> bins_b_;
    double black_pt_{0.0};
    double white_pt_{1.0};
};

} // namespace sabit::ui

#endif // SABIT_UI_HISTOGRAM_WIDGET_H
