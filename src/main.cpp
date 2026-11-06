#include <QApplication>
#include <QLocale>
#include <QMessageBox>
#include <QIcon>
#include <QDir>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <exception>
#include <clocale>

#if defined(_WIN32)
#include <windows.h>
static void harden_dll_search() {
    // 09b-Security-Plan.md: Restrict DLL search order to prevent DLL hijacking
    SetDllDirectoryW(L"");
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
}
#else
static void harden_dll_search() {}
#endif

#include "src/app/app_controller.h"
#include "src/ui/main_window.h"
#include "src/ui/theme/theme_manager.h"

int main(int argc, char* argv[]) {
    harden_dll_search();
    std::setlocale(LC_NUMERIC, "C");
    QLocale::setDefault(QLocale::c());

    try {
        QApplication app(argc, argv);
        app.setOrganizationName("Sabit");
        app.setApplicationName("Sabit Observation System");
        app.setApplicationVersion("1.0.0");

        QString app_dir = QCoreApplication::applicationDirPath();

        // 1. Set Application Window Icon (resolved via applicationDirPath)
        QString icon_path = app_dir + "/assets/icons/app_icon.ico";
        if (!QFile::exists(icon_path)) {
            icon_path = app_dir + "/assets/logo.ico";
        }
        if (!QFile::exists(icon_path)) {
            icon_path = app_dir + "/assets/logo.png";
        }
        if (!QFile::exists(icon_path)) {
            icon_path = "assets/logo.ico";
        }
        if (QFile::exists(icon_path)) {
            app.setWindowIcon(QIcon(icon_path));
        }

        // 2. Load Qlementine theme tokens & initialize QlementineStyle with Deep Space Fluent (Night Red: Ctrl+Shift+N)
        sabit::ui::ThemeManager::instance().load_tokens();
        sabit::ui::ThemeManager::instance().apply_theme(&app, sabit::ui::ThemeType::Dark);

        // 3. Initialize application controller & pipeline
        sabit::app::AppController controller;

        // 4. Create & show main window
        sabit::ui::MainWindow window(controller);
        window.show();

        return app.exec();
    } catch (const std::exception& e) {
        // Log critical error to app.log in application directory
        try {
            QString log_path = QCoreApplication::applicationDirPath() + "/app.log";
            std::ofstream log_file(log_path.toStdString(), std::ios::app);
            if (log_file.is_open()) {
                log_file << "[CRITICAL EXCEPTION] " << e.what() << std::endl;
            }
        } catch (...) {}

        std::cerr << "CRITICAL ERROR: " << e.what() << std::endl;
        QMessageBox::critical(nullptr, "Sabit Startup Error",
            QString("Terjadi kesalahan saat memulai aplikasi:\n\n%1").arg(e.what()));
        return 1;
    } catch (...) {
        try {
            QString log_path = QCoreApplication::applicationDirPath() + "/app.log";
            std::ofstream log_file(log_path.toStdString(), std::ios::app);
            if (log_file.is_open()) {
                log_file << "[CRITICAL UNKNOWN EXCEPTION] An unknown error occurred." << std::endl;
            }
        } catch (...) {}

        std::cerr << "CRITICAL ERROR: Unknown exception caught at startup." << std::endl;
        QMessageBox::critical(nullptr, "Sabit Startup Error",
            "Terjadi kesalahan yang tidak diketahui saat memulai aplikasi.");
        return 2;
    }
}
