#ifndef SABIT_UI_REPORT_ISSUE_DIALOG_H
#define SABIT_UI_REPORT_ISSUE_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "src/app/app_controller.h"

namespace sabit::ui {

class ReportIssueDialog : public QDialog {
    Q_OBJECT
public:
    explicit ReportIssueDialog(app::AppController& controller, QWidget* parent = nullptr);
    ~ReportIssueDialog() override = default;

private slots:
    void on_send_email();
    void on_copy_report();
    void on_open_github();
    void on_export_diagnostics();

private:
    void setup_ui();
    void populate_environment_info();
    [[nodiscard]] QString build_report_text(bool for_email = false) const;
    [[nodiscard]] QString generate_email_subject() const;

    app::AppController& controller_;
    QComboBox* category_combo_{nullptr};
    QLineEdit* subject_edit_{nullptr};
    QPlainTextEdit* description_edit_{nullptr};
    QPlainTextEdit* env_info_edit_{nullptr};
    QLabel* status_lbl_{nullptr};
    QPushButton* send_email_btn_{nullptr};
    QPushButton* copy_report_btn_{nullptr};
    QPushButton* github_btn_{nullptr};
    QPushButton* export_diag_btn_{nullptr};
    QPushButton* close_btn_{nullptr};
};

} // namespace sabit::ui

#endif // SABIT_UI_REPORT_ISSUE_DIALOG_H
