/**
 * @file main.cpp
 * @brief MachinaMindAIAgent Application Entry Point
 */

#include <QApplication>
#include <QFile>
#include <QDebug>

#include "model/ApiClient.h"
#include "model/DataModel.h"
#include "presenter/MainPresenter.h"
#include "view/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Application metadata
    QApplication::setApplicationName("MachinaMindAIAgent");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("MachinaMindAIAgent");

    // Load stylesheet
    QFile styleFile(":/styles/dark_theme.qss");
    if (!styleFile.open(QFile::ReadOnly)) {
        qWarning() << "Failed to load stylesheet from resources, trying local file...";
        // Fallback: Try to load from local filesystem
        styleFile.setFileName("styles/dark_theme.qss");
        if (!styleFile.open(QFile::ReadOnly)) {
            qWarning() << "Failed to load stylesheet from local filesystem";
        }
    }
    
    if (styleFile.isOpen()) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
        styleFile.close();
        qDebug() << "✓ Stylesheet loaded successfully";
    }

    // Create MVP components (heap allocation for proper lifetime)
    auto* model = new MachinaMindAIAgent::DataModel();
    auto* apiClient = new MachinaMindAIAgent::ApiClient("http://localhost:8000");

    // Create presenter without view initially
    auto* presenter = new MachinaMindAIAgent::MainPresenter(nullptr, model, apiClient);

    // Create window with presenter reference
    auto* window = new MachinaMindAIAgent::MainWindow(presenter);

    // Now connect view to presenter
    presenter->setView(window);

    // Show window
    window->show();

    return app.exec();
}
