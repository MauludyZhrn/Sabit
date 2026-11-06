#include <oclero/qlementine/style/QlementineStyle.hpp>

#include <QAbstractButton>
#include <QApplication>
#include <QComboBox>
#include <QPainter>
#include <QSlider>
#include <QSpinBox>
#include <QStyleFactory>
#include <QStyleOption>

namespace oclero::qlementine {

QlementineStyle::QlementineStyle(QObject* parent)
    : QProxyStyle(QStyleFactory::create("Fusion"))
    , animator_(this) {
    setParent(parent);
}

void QlementineStyle::setTheme(const Theme& theme) {
    theme_ = theme;
    if (qApp) {
        QPalette pal = qApp->palette();
        polish(pal);
        qApp->setPalette(pal);
    }
    emit themeChanged();
}

void QlementineStyle::setThemeJsonPath(const QString& jsonPath) {
    setTheme(Theme::fromJsonPath(jsonPath));
}

void QlementineStyle::setAutoIconColor(AutoIconColorMode mode) {
    autoIconColor_ = mode;
}

void QlementineStyle::setAnimationsEnabled(bool enabled) {
    animator_.setEnabled(enabled);
}

bool QlementineStyle::animationsEnabled() const {
    return animator_.isEnabled();
}

void QlementineStyle::polish(QPalette& palette) {
    palette.setColor(QPalette::Window, theme_.backgroundColorMain1);
    palette.setColor(QPalette::WindowText, theme_.primaryTextColor);
    palette.setColor(QPalette::Base, theme_.backgroundColorMain2);
    palette.setColor(QPalette::AlternateBase, theme_.backgroundColorMain3);
    palette.setColor(QPalette::ToolTipBase, theme_.backgroundColorMain2);
    palette.setColor(QPalette::ToolTipText, theme_.primaryTextColor);
    palette.setColor(QPalette::Text, theme_.primaryTextColor);
    palette.setColor(QPalette::Button, theme_.neutralColor);
    palette.setColor(QPalette::ButtonText, theme_.primaryTextColor);
    palette.setColor(QPalette::BrightText, theme_.primaryColor);
    palette.setColor(QPalette::Link, theme_.primaryColor);
    palette.setColor(QPalette::Highlight, theme_.primaryColor);
    palette.setColor(QPalette::HighlightedText, theme_.primaryColorForeground);
    palette.setColor(QPalette::PlaceholderText, theme_.secondaryTextColor);
    palette.setColor(QPalette::Mid, theme_.borderColor);
    palette.setColor(QPalette::Dark, theme_.backgroundColorWorkspace);

    palette.setColor(QPalette::Disabled, QPalette::WindowText, theme_.disabledTextColor);
    palette.setColor(QPalette::Disabled, QPalette::Text, theme_.disabledTextColor);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, theme_.disabledTextColor);
    palette.setColor(QPalette::Disabled, QPalette::Base, theme_.backgroundColorMain1);
    palette.setColor(QPalette::Disabled, QPalette::Button, theme_.neutralColorDisabled);
}

void QlementineStyle::polish(QApplication* app) {
    QProxyStyle::polish(app);
    if (app) {
        QPalette pal = app->palette();
        polish(pal);
        app->setPalette(pal);
    }
}

void QlementineStyle::polish(QWidget* widget) {
    QProxyStyle::polish(widget);
    if (!widget) return;

    if (qobject_cast<QAbstractButton*>(widget) ||
        qobject_cast<QComboBox*>(widget) ||
        qobject_cast<QSlider*>(widget) ||
        qobject_cast<QAbstractSpinBox*>(widget)) {
        widget->setAttribute(Qt::WA_Hover, true);
    }
}

int QlementineStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const {
    switch (metric) {
        case PM_ButtonMargin:
            return 8;
        case PM_LayoutLeftMargin:
        case PM_LayoutTopMargin:
        case PM_LayoutRightMargin:
        case PM_LayoutBottomMargin:
            return 8;
        case PM_LayoutHorizontalSpacing:
        case PM_LayoutVerticalSpacing:
            return theme_.spacing;
        case PM_SmallIconSize:
        case PM_ButtonIconSize:
            return theme_.iconSize;
        case PM_ToolBarIconSize:
            return 18;
        case PM_ScrollBarExtent:
            return 10;
        case PM_SliderLength:
            return 16;
        case PM_SliderThickness:
            return 18;
        default:
            return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

QPixmap QlementineStyle::colorizePixmap(const QPixmap& input, const QColor& targetColor) {
    if (input.isNull() || !targetColor.isValid()) return input;

    QPixmap result(input.size());
    result.setDevicePixelRatio(input.devicePixelRatio());
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawPixmap(0, 0, input);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(result.rect(), targetColor);
    painter.end();
    return result;
}

QPixmap QlementineStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap, const QStyleOption* opt) const {
    if (pixmap.isNull()) return pixmap;
    if (autoIconColor_ == AutoIconColorMode::None) {
        return QProxyStyle::generatedIconPixmap(iconMode, pixmap, opt);
    }

    if (iconMode == QIcon::Disabled) {
        return colorizePixmap(pixmap, theme_.disabledTextColor);
    }
    if (iconMode == QIcon::Selected) {
        return colorizePixmap(pixmap, theme_.primaryColorForeground);
    }
    return pixmap;
}

} // namespace oclero::qlementine
