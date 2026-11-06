#pragma once

#include <QColor>
#include <QEasingCurve>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QVariantAnimation>
#include <QWidget>
#include <functional>

namespace oclero::qlementine {

class WidgetAnimator : public QObject {
    Q_OBJECT
public:
    explicit WidgetAnimator(QObject* parent = nullptr);
    ~WidgetAnimator() override = default;

    void setEnabled(bool enabled) { enabled_ = enabled; }
    [[nodiscard]] bool isEnabled() const { return enabled_; }

    void animateProgress(
        QWidget* widget,
        const QString& channel,
        qreal startValue,
        qreal endValue,
        int durationMs = 160,
        QEasingCurve::Type curve = QEasingCurve::OutCubic,
        std::function<void(qreal)> onUpdate = nullptr);

    [[nodiscard]] qreal progress(const QWidget* widget, const QString& channel, qreal fallback = 0.0) const;

    static QColor interpolateColor(const QColor& from, const QColor& to, qreal t);

private:
    bool enabled_{true};
    struct ChannelKey {
        const QWidget* widget{nullptr};
        QString channel;
        bool operator==(const ChannelKey& other) const {
            return widget == other.widget && channel == other.channel;
        }
    };
    friend size_t qHash(const ChannelKey& key, size_t seed) noexcept {
        return qHash(reinterpret_cast<quintptr>(key.widget), seed) ^ qHash(key.channel, seed);
    }

    QHash<ChannelKey, QPointer<QVariantAnimation>> animations_;
    QHash<ChannelKey, qreal> values_;
};

} // namespace oclero::qlementine
