#pragma once

#include <oclero/qlementine/animation/WidgetAnimator.hpp>
#include <oclero/qlementine/style/Theme.hpp>

#include <QProxyStyle>

namespace oclero::qlementine {

class QlementineStyle : public QProxyStyle {
    Q_OBJECT
public:
    explicit QlementineStyle(QObject* parent = nullptr);
    ~QlementineStyle() override = default;

    void setTheme(const Theme& theme);
    void setThemeJsonPath(const QString& jsonPath);
    [[nodiscard]] const Theme& theme() const { return theme_; }

    void setAutoIconColor(AutoIconColorMode mode);
    [[nodiscard]] AutoIconColorMode autoIconColor() const { return autoIconColor_; }

    void setAnimationsEnabled(bool enabled);
    [[nodiscard]] bool animationsEnabled() const;

    [[nodiscard]] WidgetAnimator& animator() { return animator_; }
    [[nodiscard]] const WidgetAnimator& animator() const { return animator_; }

    // QStyle overrides
    void polish(QPalette& palette) override;
    void polish(QApplication* app) override;
    void polish(QWidget* widget) override;
    int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr, const QWidget* widget = nullptr) const override;
    QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* opt) const override;

    static QPixmap colorizePixmap(const QPixmap& input, const QColor& targetColor);

signals:
    void themeChanged();

private:
    Theme theme_{};
    AutoIconColorMode autoIconColor_{AutoIconColorMode::TextColor};
    WidgetAnimator animator_;
};

} // namespace oclero::qlementine
