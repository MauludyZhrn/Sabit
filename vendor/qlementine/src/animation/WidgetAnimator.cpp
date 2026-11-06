#include <oclero/qlementine/animation/WidgetAnimator.hpp>

#include <algorithm>

namespace oclero::qlementine {

WidgetAnimator::WidgetAnimator(QObject* parent)
    : QObject(parent) {}

void WidgetAnimator::animateProgress(
    QWidget* widget,
    const QString& channel,
    qreal startValue,
    qreal endValue,
    int durationMs,
    QEasingCurve::Type curve,
    std::function<void(qreal)> onUpdate) {
    if (!widget) return;

    const ChannelKey key{widget, channel};

    if (!enabled_ || durationMs <= 0) {
        values_[key] = endValue;
        if (onUpdate) onUpdate(endValue);
        widget->update();
        return;
    }

    if (auto existing = animations_.value(key)) {
        existing->stop();
        existing->deleteLater();
    }

    const qreal actualStart = values_.contains(key) ? values_.value(key) : startValue;
    auto* anim = new QVariantAnimation(widget);
    anim->setDuration(durationMs);
    anim->setStartValue(actualStart);
    anim->setEndValue(endValue);
    anim->setEasingCurve(curve);

    QPointer<QWidget> safeWidget(widget);
    connect(anim, &QVariantAnimation::valueChanged, this, [this, key, safeWidget, onUpdate](const QVariant& val) {
        const qreal v = val.toReal();
        values_[key] = v;
        if (onUpdate) onUpdate(v);
        if (safeWidget) safeWidget->update();
    });

    connect(widget, &QObject::destroyed, this, [this, key]() {
        animations_.remove(key);
        values_.remove(key);
    });

    animations_[key] = anim;
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

qreal WidgetAnimator::progress(const QWidget* widget, const QString& channel, qreal fallback) const {
    if (!widget) return fallback;
    const ChannelKey key{widget, channel};
    return values_.value(key, fallback);
}

QColor WidgetAnimator::interpolateColor(const QColor& from, const QColor& to, qreal t) {
    const qreal clamped = std::clamp(t, 0.0, 1.0);
    const int r = static_cast<int>(from.red() + (to.red() - from.red()) * clamped);
    const int g = static_cast<int>(from.green() + (to.green() - from.green()) * clamped);
    const int b = static_cast<int>(from.blue() + (to.blue() - from.blue()) * clamped);
    const int a = static_cast<int>(from.alpha() + (to.alpha() - from.alpha()) * clamped);
    return QColor(r, g, b, a);
}

} // namespace oclero::qlementine
