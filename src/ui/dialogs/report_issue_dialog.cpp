#include "src/ui/dialogs/report_issue_dialog.h"
#include "src/ui/dialogs/export_dialog.h"
#include "src/ui/fluent/fluent_card.h"
#include "src/ui/i18n/translator.h"
#include "src/core/frame.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDesktopServices>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSysInfo>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>

namespace sabit::ui {

ReportIssueDialog::ReportIssueDialog(app::AppController& controller, QWidget* parent)
    : QDialog(parent), controller_(controller) {
    setup_ui();
    populate_environment_info();
}

void ReportIssueDialog::setup_ui() {
    const bool is_id = (Translator::instance().language() == Language::Indonesian);
    setWindowTitle(is_id ? "Laporkan Masalah & Masukan Pengembang" : "Report Issue & Developer Feedback");
    setMinimumSize(640, 620);
    setModal(true);

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(16, 16, 16, 16);
    main_layout->setSpacing(10);

    // 1. Header & Guidance
    auto* header_title = new QLabel(
        is_id ? "Laporkan Masalah, Driver Kamera, atau Kendala Fitur"
              : "Report Issue, Camera Driver Failure, or Feature Bug",
        this);
    QFont hf = header_title->font();
    hf.setPointSize(11);
    hf.setWeight(QFont::Bold);
    header_title->setFont(hf);

    auto* desc = new QLabel(
        is_id ? "Kirimkan laporan kendala teknis langsung ke pengembang Sabit melalui email resmi: "
                "mauludyzahron.24@gmail.com atau melalui pelacakan GitHub Issues."
              : "Submit technical issue reports directly to the Sabit developer via official email: "
                "mauludyzahron.24@gmail.com or via GitHub Issues tracking.",
        this);
    desc->setWordWrap(true);
    desc->setObjectName("FieldLabel");

    main_layout->addWidget(header_title);
    main_layout->addWidget(desc);

    // Email badge banner
    auto* email_banner = new QLabel(
        is_id ? "Tujuan Laporan: mauludyzahron.24@gmail.com (Pengembang Resmi Sabit)"
              : "Report Target: mauludyzahron.24@gmail.com (Official Sabit Developer)",
        this);
    email_banner->setStyleSheet(
        "background-color: #2D2244; color: #D8B4FE; border: 1px solid #635985; "
        "border-radius: 4px; padding: 6px 10px; font-weight: bold;");
    main_layout->addWidget(email_banner);

    // 2. Issue Form Card
    auto* form_card = new FluentCard(is_id ? "RINCIAN KENDALA" : "ISSUE DETAILS", this);
    auto* form_layout = new QFormLayout();
    form_layout->setHorizontalSpacing(14);
    form_layout->setVerticalSpacing(8);

    category_combo_ = new QComboBox(form_card);
    category_combo_->setMinimumHeight(28);
    if (is_id) {
        category_combo_->addItem("[Driver] Driver Kamera & Koneksi (ASCOM/QHY/ZWO/UVC/DirectShow)");
        category_combo_->addItem("[Bug] Kesalahan Program / Crash / Antarmuka UI");
        category_combo_->addItem("[Processing] Pengolahan Gambar (Debayer/Stacking/Stretch/Kalibrasi)");
        category_combo_->addItem("[AI Sensor] Deteksi Sensor AI Hilal (False Positive/Negative/Akurasi)");
        category_combo_->addItem("[Falak] Astronomi, Ephemeris & Kriteria Visibilitas");
        category_combo_->addItem("[Capture] Perekaman Berkas & Format Ekspor (FITS/TIFF/SER/AVI/MP4/PDF)");
        category_combo_->addItem("[Fitur] Saran & Permintaan Fitur Baru (Feature Request)");
        category_combo_->addItem("[Lainnya] Kendala Teknis Lainnya");
    } else {
        category_combo_->addItem("[Driver] Camera Driver & Connection (ASCOM/QHY/ZWO/UVC/DirectShow)");
        category_combo_->addItem("[Bug] Application Error / Crash / UI Glitch");
        category_combo_->addItem("[Processing] Image Processing (Debayer/Stacking/Stretch/Calibration)");
        category_combo_->addItem("[AI Sensor] AI Hilal Sensor Detection (False Positive/Negative/Accuracy)");
        category_combo_->addItem("[Ephemeris] Astronomy, Ephemeris & Visibility Criteria");
        category_combo_->addItem("[Capture] File Recording & Export Formats (FITS/TIFF/SER/AVI/MP4/PDF)");
        category_combo_->addItem("[Feature] Feature Request & Suggestions");
        category_combo_->addItem("[Other] Other Technical Issues");
    }

    subject_edit_ = new QLineEdit(form_card);
    subject_edit_->setMinimumHeight(28);
    subject_edit_->setPlaceholderText(
        is_id ? "Ringkasan masalah (contoh: Gambar kamera RGB tampil monokrom, atau Driver crash saat disconnect)..."
              : "Issue summary (e.g. RGB camera displays monochrome stream, or driver crash on disconnect)...");

    description_edit_ = new QPlainTextEdit(form_card);
    description_edit_->setMinimumHeight(90);
    description_edit_->setPlaceholderText(
        is_id ? "Jelaskan langkah-langkah terjadinya masalah:\n"
                "1. Apa yang sedang dilakukan sebelum masalah terjadi?\n"
                "2. Perilaku apa yang diharapkan dan apa yang sebenarnya terjadi?\n"
                "3. Pesan error atau kode status yang terlihat..."
              : "Describe the steps to reproduce the issue:\n"
                "1. What action was performed before the issue occurred?\n"
                "2. What was expected versus what actually happened?\n"
                "3. Any error messages or observed status codes...");

    form_layout->addRow(is_id ? "Kategori Masalah:" : "Issue Category:", category_combo_);
    form_layout->addRow(is_id ? "Judul Masalah:" : "Issue Subject:", subject_edit_);
    form_layout->addRow(is_id ? "Deskripsi Detail:" : "Detailed Description:", description_edit_);
    form_card->body_layout()->addLayout(form_layout);
    main_layout->addWidget(form_card);

    // 3. System & Runtime Diagnostics Card
    auto* env_card = new FluentCard(
        is_id ? "DIAGNOSTIK SISTEM & PERANGKAT (OTOMATIS)" : "SYSTEM & HARDWARE DIAGNOSTICS (AUTOMATIC)",
        this);

    env_info_edit_ = new QPlainTextEdit(env_card);
    env_info_edit_->setReadOnly(true);
    env_info_edit_->setMaximumHeight(105);
    QFont mono_font("Courier New", 9);
    mono_font.setStyleHint(QFont::Monospace);
    env_info_edit_->setFont(mono_font);
    env_info_edit_->setStyleSheet("background-color: #120D1D; color: #C4B5FD; border: 1px solid #443C68;");

    auto* diag_hint = new QLabel(
        is_id ? "Data diagnostik di atas otomatis disertakan dalam laporan untuk mempercepat investigasi pengembang."
              : "The diagnostics data above is automatically included in the report to accelerate investigation.",
        env_card);
    diag_hint->setObjectName("FieldLabel");

    env_card->body_layout()->addWidget(env_info_edit_);
    env_card->body_layout()->addWidget(diag_hint);
    main_layout->addWidget(env_card);

    // Status label for copy feedback
    status_lbl_ = new QLabel("", this);
    status_lbl_->setObjectName("FieldValue");
    status_lbl_->setStyleSheet("color: #34D399; font-weight: bold;");
    main_layout->addWidget(status_lbl_);

    // 4. Action Buttons
    auto* btn_layout = new QHBoxLayout();
    btn_layout->setSpacing(8);

    export_diag_btn_ = new QPushButton(is_id ? "Ekspor Paket Diagnostik (.zip)..." : "Export Diagnostics Package (.zip)...", this);
    connect(export_diag_btn_, &QPushButton::clicked, this, &ReportIssueDialog::on_export_diagnostics);

    github_btn_ = new QPushButton(is_id ? "Buka GitHub Issues" : "Open GitHub Issues", this);
    connect(github_btn_, &QPushButton::clicked, this, &ReportIssueDialog::on_open_github);

    copy_report_btn_ = new QPushButton(is_id ? "Salin Teks Laporan" : "Copy Report Text", this);
    connect(copy_report_btn_, &QPushButton::clicked, this, &ReportIssueDialog::on_copy_report);

    close_btn_ = new QPushButton(tr_str("cancel"), this);
    connect(close_btn_, &QPushButton::clicked, this, &QDialog::reject);

    send_email_btn_ = new QPushButton(is_id ? "Kirim via Email" : "Send via Email", this);
    send_email_btn_->setProperty("class", "primary");
    send_email_btn_->setDefault(true);
    send_email_btn_->setCursor(Qt::PointingHandCursor);
    connect(send_email_btn_, &QPushButton::clicked, this, &ReportIssueDialog::on_send_email);

    btn_layout->addWidget(export_diag_btn_);
    btn_layout->addWidget(github_btn_);
    btn_layout->addStretch();
    btn_layout->addWidget(copy_report_btn_);
    btn_layout->addWidget(close_btn_);
    btn_layout->addWidget(send_email_btn_);

    main_layout->addLayout(btn_layout);
}

void ReportIssueDialog::populate_environment_info() {
    QStringList lines;
    lines << "Application: Sabit v1.0.0 (Native C++20 / Qt 6.8.0 x64)";
    lines << QString("Operating System: %1 (%2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());
    lines << QString("Report Generated: %1 UTC").arg(QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss"));

    // Camera diagnostics
    auto sys_status = controller_.get_system_status();
    auto caps = controller_.camera_caps();
    auto cam_status = controller_.camera_status();
    if (sys_status.camera_connected) {
        lines << QString("Connected Camera: %1 [Driver: %2]").arg(QString::fromStdString(caps.model), QString::fromStdString(caps.driver));
        lines << QString("Sensor Resolution: %1 x %2 | Color: %3 [Bayer: %4]")
                     .arg(caps.max_width)
                     .arg(caps.max_height)
                     .arg(caps.is_color ? "Color" : "Monochrome")
                     .arg(core::bayer_pattern_to_string(caps.bayer));
        lines << QString("FPS: %1 | Captured: %2 | Dropped: %3")
                     .arg(QString::number(cam_status.actual_fps, 'f', 1))
                     .arg(cam_status.frames_captured)
                     .arg(cam_status.frames_dropped_hw);
    } else {
        lines << "Connected Camera: None (No camera connected / Idle)";
    }

    // Active session
    if (controller_.is_session_active()) {
        auto sess = controller_.session_info();
        lines << QString("Active Session: ID %1 | Target: %2 | Observer: %3")
                     .arg(QString::fromStdString(sess.session_id))
                     .arg(QString::fromStdString(sess.target))
                     .arg(QString::fromStdString(sess.observer));
        lines << QString("Session Folder: %1").arg(QString::fromStdString(sess.folder_path));
    } else {
        lines << "Active Session: Inactive (Idle)";
    }

    // AI & Storage
    lines << QString("AI Detection Sensor: %1 (Confidence Threshold: 0.40)")
                 .arg(controller_.is_ai_enabled() ? "Active" : "Disabled");
    lines << QString("Captures Directory: %1").arg(QString::fromStdString(controller_.captures_dir()));

    // Observer Site
    auto site = controller_.observer_site();
    lines << QString("Observer Site: %1 (Lat: %2 deg, Lon: %3 deg, Alt: %4 m)")
                 .arg(QString::fromStdString(site.name))
                 .arg(QString::number(site.latitude, 'f', 4))
                 .arg(QString::number(site.longitude, 'f', 4))
                 .arg(QString::number(site.elevation_m, 'f', 0));

    env_info_edit_->setPlainText(lines.join("\n"));
}

QString ReportIssueDialog::generate_email_subject() const {
    QString cat = category_combo_->currentText();
    // Extract short bracketed tag
    int open_bracket = cat.indexOf('[');
    int close_bracket = cat.indexOf(']');
    QString tag = "Issue";
    if (open_bracket != -1 && close_bracket != -1 && close_bracket > open_bracket) {
        tag = cat.mid(open_bracket + 1, close_bracket - open_bracket - 1);
    }

    QString subj = subject_edit_->text().trimmed();
    if (subj.isEmpty()) {
        subj = "Technical Report";
    }

    return QString("[Sabit v1.0.0] [%1] %2").arg(tag, subj);
}

QString ReportIssueDialog::build_report_text(bool for_email) const {
    Q_UNUSED(for_email);
    QStringList report;
    report << "=======================================================";
    report << "SABIT ASTRONOMICAL SOFTWARE - ISSUE & BUG REPORT";
    report << "Developer Recipient: mauludyzahron.24@gmail.com";
    report << "=======================================================";
    report << "";
    report << QString("CATEGORY : %1").arg(category_combo_->currentText());
    report << QString("SUBJECT  : %1").arg(subject_edit_->text().trimmed().isEmpty() ? "(No Subject Specified)" : subject_edit_->text().trimmed());
    report << "";
    report << "--- PROBLEM DESCRIPTION & STEPS TO REPRODUCE ---";
    QString desc = description_edit_->toPlainText().trimmed();
    if (desc.isEmpty()) {
        report << "(No additional description provided by user)";
    } else {
        report << desc;
    }
    report << "";
    report << "--- SYSTEM & RUNTIME ENVIRONMENT ---";
    report << env_info_edit_->toPlainText();
    report << "";
    report << "--- ATTACHMENT NOTE ---";
    report << "If applicable, please attach any relevant screenshots or the";
    report << "exported diagnostic package (.zip) generated via 'Export Diagnostics Package'.";
    report << "=======================================================";

    return report.join("\n");
}

void ReportIssueDialog::on_send_email() {
    const bool is_id = (Translator::instance().language() == Language::Indonesian);
    QString full_report = build_report_text(true);
    QString subject = generate_email_subject();

    // Copy to clipboard first so user can paste it anywhere
    QGuiApplication::clipboard()->setText(full_report);

    // Build mailto URL cleanly with QUrl and QUrlQuery
    QUrl mail_url("mailto:mauludyzahron.24@gmail.com");
    QUrlQuery query;
    query.addQueryItem("subject", subject);
    query.addQueryItem("body", full_report);
    mail_url.setQuery(query);

    bool opened = QDesktopServices::openUrl(mail_url);

    status_lbl_->setText(
        is_id ? "Teks laporan berhasil disalin ke clipboard! Membuka klien email..."
              : "Report text copied to clipboard! Opening email client...");

    QMessageBox::information(
        this,
        is_id ? "Pengiriman Laporan Masalah" : "Issue Report Submission",
        QString(
            is_id ? "Laporan telah disiapkan untuk dikirim ke:\nmauludyzahron.24@gmail.com\n\n"
                    "%1\n\n"
                    "Catatan: Teks laporan juga telah otomatis disalin ke papan klip (clipboard) "
                    "sehingga Anda dapat langsung menempelkannya (Ctrl+V) jika menggunakan browser webmail (Gmail/Yahoo/dll).\n\n"
                    "Bila Anda memiliki paket diagnostik (.zip) atau screenshot, silakan lampirkan pada email tersebut."
                  : "The report has been prepared for:\nmauludyzahron.24@gmail.com\n\n"
                    "%1\n\n"
                    "Note: The report text has also been automatically copied to your clipboard "
                    "so you can paste it (Ctrl+V) into your browser webmail (Gmail/Yahoo/etc) if needed.\n\n"
                    "If you have an exported diagnostic package (.zip) or screenshots, please attach them to the email.")
            .arg(opened
                     ? (is_id ? "Klien email bawaan sistem sedang dibuka." : "System email client is opening.")
                     : (is_id ? "Klien email bawaan tidak terdeteksi. Silakan buka email browser Anda dan kirim ke mauludyzahron.24@gmail.com."
                              : "No default desktop mail client detected. Please open your webmail and send to mauludyzahron.24@gmail.com.")));
}

void ReportIssueDialog::on_copy_report() {
    const bool is_id = (Translator::instance().language() == Language::Indonesian);
    QString full_report = build_report_text(false);
    QGuiApplication::clipboard()->setText(full_report);

    status_lbl_->setText(
        is_id ? "Teks laporan berhasil disalin ke papan klip (clipboard)!"
              : "Report text successfully copied to clipboard!");
}

void ReportIssueDialog::on_open_github() {
    QString subject = generate_email_subject();
    QString body = build_report_text(false);

    QUrl gh_url("https://github.com/MauludyZhrn/Sabit/issues/new");
    QUrlQuery query;
    query.addQueryItem("title", subject);
    query.addQueryItem("body", body);
    gh_url.setQuery(query);

    QDesktopServices::openUrl(gh_url);
}

void ReportIssueDialog::on_export_diagnostics() {
    ExportDialog dlg(controller_, this);
    dlg.exec();
}

} // namespace sabit::ui
