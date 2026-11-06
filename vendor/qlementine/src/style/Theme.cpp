#include <oclero/qlementine/style/Theme.hpp>

#include <QFile>
#include <QFontDatabase>

namespace oclero::qlementine {

namespace {

QColor readColor(const QJsonObject& obj, const char* key, const QColor& fallback) {
    if (const auto v = obj.value(QLatin1String(key)); v.isString()) {
        const QColor c(v.toString());
        if (c.isValid()) {
            return c;
        }
    }
    return fallback;
}

} // namespace

Theme::Theme() {
    fontRegular = QFont("Segoe UI Variable", 9);
    fontRegular.setStyleHint(QFont::SansSerif);

    fontBold = fontRegular;
    fontBold.setWeight(QFont::DemiBold);

    fontMonospace = QFont("Cascadia Mono", 9);
    fontMonospace.setStyleHint(QFont::Monospace);
}

Theme Theme::fromJsonPath(const QString& jsonPath) {
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return Theme{};
    }
    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return Theme{};
    }
    return fromJsonObject(doc.object());
}

Theme Theme::fromJsonObject(const QJsonObject& obj) {
    Theme t;

    if (const auto metaObj = obj.value("meta").toObject(); !metaObj.isEmpty()) {
        t.meta.name = metaObj.value("name").toString(t.meta.name);
        t.meta.version = metaObj.value("version").toString(t.meta.version);
        t.meta.author = metaObj.value("author").toString(t.meta.author);
    } else if (obj.contains("name")) {
        t.meta.name = obj.value("name").toString(t.meta.name);
    }

    const QJsonObject colors = obj.contains("colors") ? obj.value("colors").toObject() : obj;

    // Support both Qlementine native keys and Sabit token keys
    t.backgroundColorMain1 = readColor(colors, "backgroundColorMain1",
        readColor(colors, "bg", t.backgroundColorMain1));
    t.backgroundColorMain2 = readColor(colors, "backgroundColorMain2",
        readColor(colors, "surface", t.backgroundColorMain2));
    t.backgroundColorMain3 = readColor(colors, "backgroundColorMain3",
        readColor(colors, "surface2", t.backgroundColorMain3));
    t.backgroundColorMain4 = readColor(colors, "backgroundColorMain4",
        readColor(colors, "hover", t.backgroundColorMain4));
    t.backgroundColorWorkspace = readColor(colors, "backgroundColorWorkspace",
        readColor(colors, "video_bg", t.backgroundColorWorkspace));
    t.backgroundColorTabBar = readColor(colors, "backgroundColorTabBar", t.backgroundColorMain1);

    t.neutralColor = readColor(colors, "neutralColor", t.backgroundColorMain3);
    t.neutralColorHovered = readColor(colors, "neutralColorHovered", t.backgroundColorMain4);
    t.neutralColorPressed = readColor(colors, "neutralColorPressed",
        readColor(colors, "border", t.neutralColorPressed));
    t.neutralColorDisabled = readColor(colors, "neutralColorDisabled", t.backgroundColorMain2);

    t.primaryColor = readColor(colors, "primaryColor",
        readColor(colors, "accent", t.primaryColor));
    t.primaryColorHovered = readColor(colors, "primaryColorHovered", t.primaryColor.lighter(115));
    t.primaryColorPressed = readColor(colors, "primaryColorPressed", t.primaryColor.darker(115));
    t.primaryColorDisabled = readColor(colors, "primaryColorDisabled", t.primaryColor.darker(220));
    t.primaryColorForeground = readColor(colors, "primaryColorForeground",
        readColor(colors, "on_accent", t.primaryColorForeground));

    t.secondaryColor = readColor(colors, "secondaryColor",
        readColor(colors, "accent_secondary", t.secondaryColor));
    t.secondaryColorHovered = readColor(colors, "secondaryColorHovered", t.secondaryColor.lighter(115));
    t.secondaryColorPressed = readColor(colors, "secondaryColorPressed", t.secondaryColor.darker(115));
    t.secondaryColorForeground = readColor(colors, "secondaryColorForeground", t.primaryTextColor);

    t.primaryTextColor = readColor(colors, "primaryTextColor",
        readColor(colors, "text", t.primaryTextColor));
    t.secondaryTextColor = readColor(colors, "secondaryTextColor",
        readColor(colors, "muted", t.secondaryTextColor));
    t.disabledTextColor = readColor(colors, "disabledTextColor", t.secondaryTextColor.darker(150));

    t.borderColor = readColor(colors, "borderColor",
        readColor(colors, "border", t.borderColor));
    t.borderColorHovered = readColor(colors, "borderColorHovered", t.borderColor.lighter(130));
    t.borderColorFocused = readColor(colors, "borderColorFocused",
        readColor(colors, "focus", t.primaryColor));
    t.borderColorDisabled = readColor(colors, "borderColorDisabled",
        readColor(colors, "border_subtle", t.borderColor));
    t.focusColor = readColor(colors, "focusColor", t.borderColorFocused);

    t.statusColorSuccess = readColor(colors, "statusColorSuccess",
        readColor(colors, "success", t.statusColorSuccess));
    t.statusColorWarning = readColor(colors, "statusColorWarning",
        readColor(colors, "warn", t.statusColorWarning));
    t.statusColorError = readColor(colors, "statusColorError",
        readColor(colors, "danger", t.statusColorError));

    if (obj.contains("borderRadius")) {
        t.borderRadius = obj.value("borderRadius").toDouble(t.borderRadius);
    }

    return t;
}

QJsonObject Theme::toJsonObject() const {
    QJsonObject metaObj;
    metaObj["name"] = meta.name;
    metaObj["version"] = meta.version;
    metaObj["author"] = meta.author;

    QJsonObject root;
    root["meta"] = metaObj;
    root["backgroundColorMain1"] = backgroundColorMain1.name();
    root["backgroundColorMain2"] = backgroundColorMain2.name();
    root["backgroundColorMain3"] = backgroundColorMain3.name();
    root["backgroundColorWorkspace"] = backgroundColorWorkspace.name();
    root["primaryColor"] = primaryColor.name();
    root["primaryColorForeground"] = primaryColorForeground.name();
    root["secondaryColor"] = secondaryColor.name();
    root["primaryTextColor"] = primaryTextColor.name();
    root["secondaryTextColor"] = secondaryTextColor.name();
    root["borderColor"] = borderColor.name();
    root["focusColor"] = focusColor.name();
    root["statusColorSuccess"] = statusColorSuccess.name();
    root["statusColorWarning"] = statusColorWarning.name();
    root["statusColorError"] = statusColorError.name();
    root["borderRadius"] = borderRadius;
    return root;
}

} // namespace oclero::qlementine
