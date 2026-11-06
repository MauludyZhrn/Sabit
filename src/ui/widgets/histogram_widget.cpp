#include "src/ui/widgets/histogram_widget.h"
#include "src/ui/theme/theme_manager.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>

namespace sabit::ui {

HistogramWidget::HistogramWidget(QWidget* parent) : QWidget(parent) {
    setObjectName("HistogramFrame");
    setMinimumHeight(64);
    setMaximumHeight(76);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void HistogramWidget::update_histogram(const std::vector<uint32_t>& bins_256) {
    bins_ = bins_256;
    bins_r_ = bins_256;
    bins_g_ = bins_256;
    bins_b_ = bins_256;
    update();
}

void HistogramWidget::update_histogram_rgb(const std::vector<uint32_t>& bins_r,
                                           const std::vector<uint32_t>& bins_g,
                                           const std::vector<uint32_t>& bins_b,
                                           const std::vector<uint32_t>& bins_lum) {
    bins_r_ = bins_r;
    bins_g_ = bins_g;
    bins_b_ = bins_b;
    bins_ = bins_lum.empty() ? bins_g : bins_lum;
    update();
}

void HistogramWidget::set_stretch_points(double black_pt, double white_pt) {
    black_pt_ = std::clamp(black_pt, 0.0, 1.0);
    white_pt_ = std::clamp(white_pt, 0.0, 1.0);
    update();
}

void HistogramWidget::clear() {
    bins_.clear();
    bins_r_.clear();
    bins_g_.clear();
    bins_b_.clear();
    update();
}

void HistogramWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const auto& colors = ThemeManager::instance().current_colors();
    const bool is_night_red = (ThemeManager::instance().current_theme() == ThemeType::NightRed);
    const QRectF frameRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);

    // Rounded Fluent canvas background & subtle grid lines
    QPainterPath clipPath;
    clipPath.addRoundedRect(frameRect, 6.0, 6.0);
    p.setClipPath(clipPath);

    p.fillRect(frameRect, QColor(colors.video_bg.isEmpty() ? "#04070D" : colors.video_bg));

    // Subtle quarter grid lines
    QColor gridCol(colors.border);
    gridCol.setAlpha(110);
    p.setPen(QPen(gridCol, 1.0, Qt::DotLine));
    for (int i = 1; i <= 3; ++i) {
        const qreal gx = frameRect.left() + (frameRect.width() * i) / 4.0;
        p.drawLine(QPointF(gx, frameRect.top()), QPointF(gx, frameRect.bottom()));
    }

    const bool has_rgb = !bins_r_.empty() && !bins_g_.empty() && !bins_b_.empty();
    if (has_rgb || !bins_.empty()) {
        const auto& r_vec = has_rgb ? bins_r_ : bins_;
        const auto& g_vec = has_rgb ? bins_g_ : bins_;
        const auto& b_vec = has_rgb ? bins_b_ : bins_;

        uint32_t max_bin = 1;
        auto scan_max = [&](const std::vector<uint32_t>& vec) {
            for (size_t i = 1; i + 1 < vec.size(); ++i) {
                if (vec[i] > max_bin) max_bin = vec[i];
            }
        };
        scan_max(r_vec);
        scan_max(g_vec);
        scan_max(b_vec);
        if (max_bin <= 1) {
            for (size_t i = 0; i < r_vec.size(); ++i) max_bin = std::max(max_bin, r_vec[i]);
            for (size_t i = 0; i < g_vec.size(); ++i) max_bin = std::max(max_bin, g_vec[i]);
            for (size_t i = 0; i < b_vec.size(); ++i) max_bin = std::max(max_bin, b_vec[i]);
        }

        const qreal w = frameRect.width();
        const qreal h = frameRect.height();

        auto draw_channel_curve = [&](const std::vector<uint32_t>& vec, const QColor& line_col) {
            if (vec.size() < 2) return;
            QPainterPath linePath;
            QPainterPath fillPath;
            fillPath.moveTo(frameRect.left(), frameRect.bottom());

            for (size_t i = 0; i < vec.size(); ++i) {
                const qreal x = frameRect.left() + (static_cast<qreal>(i) * w) / (vec.size() - 1);
                const qreal norm = static_cast<qreal>(vec[i]) / static_cast<qreal>(max_bin);
                const qreal bar_h = std::clamp(norm * (h - 12.0), 0.0, h - 12.0);
                const qreal y = frameRect.bottom() - 2.0 - bar_h;
                if (i == 0) {
                    linePath.moveTo(x, y);
                } else {
                    linePath.lineTo(x, y);
                }
                fillPath.lineTo(x, y);
            }
            fillPath.lineTo(frameRect.right(), frameRect.bottom());
            fillPath.closeSubpath();

            QLinearGradient grad(0, frameRect.top(), 0, frameRect.bottom());
            QColor topFill = line_col;
            topFill.setAlpha(46);
            QColor botFill = line_col;
            botFill.setAlpha(5);
            grad.setColorAt(0.0, topFill);
            grad.setColorAt(1.0, botFill);

            p.fillPath(fillPath, grad);
            p.setPen(QPen(line_col, 1.45, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.drawPath(linePath);
        };

        if (is_night_red) {
            draw_channel_curve(r_vec, QColor("#FF2A2A"));
            draw_channel_curve(g_vec, QColor("#CC1818"));
            draw_channel_curve(b_vec, QColor("#991010"));
        } else {
            // Draw Blue, Green, Red channel curves
            draw_channel_curve(b_vec, QColor("#3B82F6")); // Blue
            draw_channel_curve(g_vec, QColor("#22C55E")); // Green
            draw_channel_curve(r_vec, QColor("#EF4444")); // Red
        }

        // Black & White stretch point markers
        const qreal bx = frameRect.left() + black_pt_ * w;
        p.setPen(QPen(QColor(colors.muted), 1.0, Qt::DashLine));
        p.drawLine(QPointF(bx, frameRect.top()), QPointF(bx, frameRect.bottom()));

        const qreal wx = frameRect.left() + white_pt_ * w;
        p.setPen(QPen(QColor(colors.text), 1.0, Qt::DashLine));
        p.drawLine(QPointF(wx, frameRect.top()), QPointF(wx, frameRect.bottom()));
    }

    // Small RGB badge in top-right corner
    if (!is_night_red) {
        QFont badgeFont("Consolas", 7, QFont::Bold);
        p.setFont(badgeFont);
        p.setPen(QColor("#EF4444"));
        p.drawText(QPointF(frameRect.right() - 28.0, frameRect.top() + 10.0), "R");
        p.setPen(QColor("#22C55E"));
        p.drawText(QPointF(frameRect.right() - 19.0, frameRect.top() + 10.0), "G");
        p.setPen(QColor("#3B82F6"));
        p.drawText(QPointF(frameRect.right() - 10.0, frameRect.top() + 10.0), "B");
    }

    p.setClipping(false);
    p.setPen(QPen(QColor(colors.border), 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(frameRect, 6.0, 6.0);
}

} // namespace sabit::ui
