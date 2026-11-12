#include "src/ui/dialogs/export_dialog.h"
#include "src/ui/fluent/fluent_card.h"
#include "src/ui/fluent/fluent_toggle_switch.h"
#include "src/ui/i18n/translator.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>

namespace sabit::ui {

ExportDialog::ExportDialog(app::AppController& controller, QWidget* parent)
    : QDialog(parent), controller_(controller) {
    bool is_id = (Translator::instance().language() == Language::Indonesian);
    setWindowTitle(is_id ? "Ekspor Paket Diagnostik" : "Export Diagnostics Package");
    setMinimumSize(520, 290);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(12);

    auto* header_title = new QLabel(is_id ? "Ekspor Paket Diagnostik & Audit Observasi" : "Export Diagnostics Package & Observation Audit", this);
    QFont hf = header_title->font();
    hf.setPointSize(11);
    hf.setWeight(QFont::Bold);
    header_title->setFont(hf);

    auto* desc = new QLabel(
        is_id ? "Mengumpulkan berkas session.json, session.log, detections.csv, dan settings.json "
                "untuk verifikasi rukyatul hilal atau pelaporan kendala teknis."
              : "Collects session.json, session.log, detections.csv, and settings.json files "
                "for crescent sighting verification or technical issue reporting.", this);
    desc->setWordWrap(true);
    desc->setObjectName("FieldLabel");

    layout->addWidget(header_title);
    layout->addWidget(desc);

    auto* card = new FluentCard(is_id ? "KONFIGURASI EKSPOR" : "EXPORT CONFIGURATION", this);

    auto* path_layout = new QHBoxLayout();
    dest_path_edit_ = new QLineEdit(card);
    dest_path_edit_->setPlaceholderText(is_id ? "Pilih folder atau berkas tujuan ekspor..." : "Select destination export folder or file...");
    dest_path_edit_->setText("diagnostic_report");

    auto* browse_btn = new QPushButton(tr_str("browse"), card);
    connect(browse_btn, &QPushButton::clicked, this, &ExportDialog::on_browse);

    path_layout->addWidget(dest_path_edit_);
    path_layout->addWidget(browse_btn);
    card->body_layout()->addLayout(path_layout);

    redact_coords_check_ = new FluentToggleSwitch(is_id ? "Samarkan koordinat lokasi (Privasi / Redact Coordinates)" : "Redact location coordinates (Privacy protection)", card);
    redact_coords_check_->setChecked(false);
    card->body_layout()->addWidget(redact_coords_check_);

    status_lbl_ = new QLabel("", card);
    status_lbl_->setObjectName("FieldValue");
    card->body_layout()->addWidget(status_lbl_);

    layout->addWidget(card);
    layout->addStretch();

    auto* btn_layout = new QHBoxLayout();
    btn_layout->setSpacing(8);

    email_btn_ = new QPushButton(is_id ? "Kirim via Email ke Pengembang..." : "Email to Developer...", this);
    email_btn_->setToolTip(is_id ? "Buka email untuk mengirim berkas diagnostik ke mauludyzahron.24@gmail.com"
                                 : "Open email client to send diagnostics to mauludyzahron.24@gmail.com");
    connect(email_btn_, &QPushButton::clicked, this, &ExportDialog::on_send_email);

    auto* close_btn = new QPushButton(tr_str("cancel"), this);
    connect(close_btn, &QPushButton::clicked, this, &QDialog::reject);

    export_btn_ = new QPushButton(is_id ? "Ekspor Sekarang" : "Export Now", this);
    export_btn_->setProperty("class", "primary");
    export_btn_->setDefault(true);
    export_btn_->setCursor(Qt::PointingHandCursor);
    connect(export_btn_, &QPushButton::clicked, this, &ExportDialog::on_export);

    btn_layout->addWidget(email_btn_);
    btn_layout->addStretch();
    btn_layout->addWidget(close_btn);
    btn_layout->addWidget(export_btn_);
    layout->addLayout(btn_layout);
}

void ExportDialog::on_send_email() {
    bool is_id = (Translator::instance().language() == Language::Indonesian);
    QString dest = dest_path_edit_->text().trimmed();
    if (dest.isEmpty()) {
        dest = "diagnostic_report";
    }

    bool mask = redact_coords_check_->isChecked();
    controller_.export_diagnostic(dest.toStdString(), mask);

    QUrl mail_url("mailto:mauludyzahron.24@gmail.com");
    QUrlQuery query;
    query.addQueryItem("subject", "[Sabit Diagnostics Package] Observation Audit Report");
    QString body = QString(
        "=== SABIT DIAGNOSTICS PACKAGE SUBMISSION ===\n"
        "Developer Recipient: mauludyzahron.24@gmail.com\n\n"
        "Export Path: %1\n"
        "Coordinates Redacted: %2\n\n"
        "Please attach the generated diagnostic files (session.json, session.log, detections.csv, settings.json)\n"
        "or directory from the above export path to this email.\n"
        "============================================")
        .arg(dest, mask ? "Yes" : "No");
    query.addQueryItem("body", body);
    mail_url.setQuery(query);

    QDesktopServices::openUrl(mail_url);

    QMessageBox::information(this,
        is_id ? "Kirim Paket Diagnostik via Email" : "Email Diagnostics Package",
        is_id ? "Klien email Anda sedang dibuka untuk mengirim laporan ke mauludyzahron.24@gmail.com.\n\n"
                "Silakan lampirkan berkas paket diagnostik dari lokasi:\n" + dest + "\nke dalam email tersebut."
              : "Opening email client to send report to mauludyzahron.24@gmail.com.\n\n"
                "Please attach the diagnostic files from:\n" + dest + "\nto the email draft.");
}

void ExportDialog::on_browse() {
    bool is_id = (Translator::instance().language() == Language::Indonesian);
    QString dir = QFileDialog::getExistingDirectory(this, is_id ? "Pilih Folder Tujuan Ekspor" : "Select Export Destination Directory");
    if (!dir.isEmpty()) {
        dest_path_edit_->setText(dir + "/diagnostic_export");
    }
}

void ExportDialog::on_export() {
    bool is_id = (Translator::instance().language() == Language::Indonesian);
    QString dest = dest_path_edit_->text().trimmed();
    if (dest.isEmpty()) {
        QMessageBox::warning(this, is_id ? "Peringatan" : "Warning", is_id ? "Harap tentukan jalur tujuan ekspor." : "Please specify an export destination path.");
        return;
    }

    bool mask = redact_coords_check_->isChecked();
    bool ok = controller_.export_diagnostic(dest.toStdString(), mask);
    if (ok) {
        status_lbl_->setText(is_id ? "Paket diagnostik berhasil diekspor." : "Diagnostics package exported successfully.");
        QMessageBox::information(this, is_id ? "Ekspor Berhasil" : "Export Succeeded",
            QString(is_id ? "Paket diagnostik berhasil disimpan ke:\n%1\n\nKoordinat lokasi: %2"
                          : "Diagnostics package successfully saved to:\n%1\n\nCoordinates: %2")
                .arg(dest)
                .arg(mask ? (is_id ? "Disamarkan ([REDACTED])" : "Redacted ([REDACTED])") : (is_id ? "Asli" : "Original")));
        accept();
    } else {
        status_lbl_->setText(is_id ? "Gagal mengekspor: Sesi aktif belum pernah dimulai." : "Export failed: No active session has been started.");
        QMessageBox::warning(this, is_id ? "Ekspor Gagal" : "Export Failed",
            is_id ? "Gagal membuat paket diagnostik. Pastikan sesi telah dimulai."
                  : "Failed to create diagnostics package. Make sure an observation session has started.");
    }
}

} // namespace sabit::ui
