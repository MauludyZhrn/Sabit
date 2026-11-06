#pragma once

#include <QColor>
#include <QFont>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

namespace oclero::qlementine {

struct ThemeMeta {
    QString name{"Deep Space Fluent"};
    QString version{"1.0.0"};
    QString author{"Sabit Observatory"};
};

enum class AutoIconColorMode {
    None = 0,
    TextColor = 1,
    Palette = 2,
};

struct Theme {
    ThemeMeta meta{};

    // Background surfaces
    QColor backgroundColorMain1{"#070B14"};
    QColor backgroundColorMain2{"#0F172A"};
    QColor backgroundColorMain3{"#1E293B"};
    QColor backgroundColorMain4{"#27354F"};
    QColor backgroundColorWorkspace{"#04070D"};
    QColor backgroundColorTabBar{"#0B1120"};

    // Neutral control fills
    QColor neutralColor{"#1E293B"};
    QColor neutralColorHovered{"#27354F"};
    QColor neutralColorPressed{"#334155"};
    QColor neutralColorDisabled{"#0F172A"};

    // Primary accent (Lunar Cyan in Dark, Astronomy Red in NightRed)
    QColor primaryColor{"#67E8F9"};
    QColor primaryColorHovered{"#A5F3FC"};
    QColor primaryColorPressed{"#22D3EE"};
    QColor primaryColorDisabled{"#164E63"};
    QColor primaryColorForeground{"#070B14"};
    QColor primaryColorForegroundDisabled{"#475569"};

    // Secondary accent / AI (Aurora Violet in Dark, Deep Red in NightRed)
    QColor secondaryColor{"#8B5CF6"};
    QColor secondaryColorHovered{"#A78BFA"};
    QColor secondaryColorPressed{"#7C3AED"};
    QColor secondaryColorDisabled{"#4C1D95"};
    QColor secondaryColorForeground{"#F8FAFC"};

    // Typography / Foreground colors
    QColor primaryTextColor{"#F8FAFC"};
    QColor secondaryTextColor{"#94A3B8"};
    QColor disabledTextColor{"#475569"};

    // Borders & Focus
    QColor borderColor{"#1E293B"};
    QColor borderColorHovered{"#334155"};
    QColor borderColorFocused{"#67E8F9"};
    QColor borderColorDisabled{"#0F172A"};
    QColor focusColor{"#67E8F9"};

    // Status semantics
    QColor statusColorSuccess{"#10B981"};
    QColor statusColorWarning{"#F59E0B"};
    QColor statusColorError{"#EF4444"};
    QColor statusColorInfo{"#38BDF8"};
    QColor statusColorForeground{"#070B14"};

    // Shadows & Elevation
    QColor shadowColor1{0, 0, 0, 110};
    QColor shadowColor2{0, 0, 0, 160};

    // Geometry metrics (Fluent proportions)
    double borderRadius{6.0};
    double checkBoxBorderRadius{4.0};
    double menuItemBorderRadius{4.0};
    double borderWidth{1.0};
    int controlHeightLarge{34};
    int controlHeightMedium{28};
    int controlHeightSmall{22};
    int spacing{6};
    int iconSize{16};

    // Fonts
    QFont fontRegular{};
    QFont fontBold{};
    QFont fontMonospace{};

    Theme();

    static Theme fromJsonPath(const QString& jsonPath);
    static Theme fromJsonObject(const QJsonObject& obj);
    QJsonObject toJsonObject() const;
};

} // namespace oclero::qlementine
