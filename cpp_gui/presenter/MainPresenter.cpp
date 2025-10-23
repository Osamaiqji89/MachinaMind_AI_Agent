/**
 * @file MainPresenter.cpp
 * @brief Implementation des Main Presenters
 */

#include "MainPresenter.h"

#include <QJsonArray>

namespace MachinaMindAIAgent {

MainPresenter::MainPresenter(IMainView* view, DataModel* model, ApiClient* apiClient,
                             QObject* parent)
    : QObject(parent), m_view(view), m_model(model), m_apiClient(apiClient) {
    // Auto-refresh timer
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &MainPresenter::onAutoRefresh);

    // Model signals
    connect(m_model, &DataModel::machinesUpdated, this, &MainPresenter::onMachinesLoaded);
    connect(m_model, &DataModel::eventsUpdated, this, &MainPresenter::onEventsLoaded);
    connect(m_model, &DataModel::healthStatusUpdated, this, &MainPresenter::onHealthCheckCompleted);

    // API error handling
    connect(m_apiClient, &ApiClient::connectionError, this, &MainPresenter::handleApiError);
}

MainPresenter::~MainPresenter() = default;

// ==================== Initialization ====================

void MainPresenter::initialize() {
    if (!m_view) {
        return;  // View not set yet
    }
    loadHealthStatus();
    loadMachines();
}

void MainPresenter::setView(IMainView* view) {
    m_view = view;
}

// ==================== User Actions ====================

void MainPresenter::onMachineSelected(int machineId) {
    m_model->setSelectedMachineId(machineId);
    loadMeasurements(machineId);
    loadEvents(machineId);
}

void MainPresenter::onRefreshClicked() {
    loadMachines();

    int selectedId = m_model->selectedMachineId();
    if (selectedId > 0) {
        loadMeasurements(selectedId);
        loadEvents(selectedId);
    }

    if (m_view) {
        m_view->showInfo("Daten aktualisiert");
    }
}

void MainPresenter::onSendChatMessage(const QString& message) {
    if (!m_view) {
        return;
    }

    if (message.trimmed().isEmpty()) {
        m_view->showError("Bitte geben Sie eine Nachricht ein");
        return;
    }

    // Add user message to chat
    ChatMessage userMsg;
    userMsg.role = ChatMessage::User;
    userMsg.content = message;
    userMsg.timestamp = QDateTime::currentDateTime();
    m_model->addChatMessage(userMsg);
    m_view->appendChatMessage("User", message);

    // Send to backend
    int machineId = m_model->selectedMachineId();

    m_apiClient->sendChatMessage(
        message, machineId,
        [this](const QJsonDocument& doc) {
            // Success
            auto obj = doc.object();
            auto chatMsg = ChatMessage::fromJson(obj);
            m_model->addChatMessage(chatMsg);
            if (m_view) {
                m_view->appendChatMessage("Assistant", chatMsg.content);
            }
        },
        [this](const QString& error) {
            // Error
            if (m_view) {
                m_view->showError("Chat-Fehler: " + error);
            }
        });
}

void MainPresenter::onAnalyzeClicked() {
    if (!m_view) {
        return;
    }

    int machineId = m_model->selectedMachineId();

    if (machineId <= 0) {
        m_view->showError("Bitte wählen Sie eine Maschine aus");
        return;
    }

    m_view->showInfo("Analyse wird durchgeführt...");

    m_apiClient->analyzeMachine(
        machineId, QString(), 60,  // Letzten 60 Minuten
        [this](const QJsonDocument& doc) {
            // Success
            auto result = AnalysisResult::fromJson(doc.object());
            m_model->setAnalysisResult(result);
            if (m_view) {
                m_view->setAnalysisResult(result.summary, result.anomaliesDetected);
            }
        },
        [this](const QString& error) {
            // Error
            if (m_view) {
                m_view->showError("Analyse-Fehler: " + error);
            }
        });
}

void MainPresenter::onConnectClicked(const QString& serverUrl) {
    if (!m_view) {
        return;
    }

    if (serverUrl.trimmed().isEmpty()) {
        m_view->showError("Server-URL darf nicht leer sein");
        return;
    }

    m_apiClient->setBaseUrl(serverUrl);
    loadHealthStatus();
}

// ==================== Auto-Refresh ====================

void MainPresenter::setAutoRefreshEnabled(bool enabled) {
    m_autoRefreshEnabled = enabled;

    if (enabled) {
        m_refreshTimer->start();
    } else {
        m_refreshTimer->stop();
    }
}

void MainPresenter::setRefreshInterval(int seconds) {
    m_refreshTimer->setInterval(seconds * 1000);
}

void MainPresenter::onAutoRefresh() {
    if (m_autoRefreshEnabled) {
        onRefreshClicked();
    }
}

// ==================== API Calls ====================

void MainPresenter::loadHealthStatus() {
    m_apiClient->healthCheck(
        [this](const QJsonDocument& doc) {
            // Success
            auto status = HealthStatus::fromJson(doc.object());
            m_model->setHealthStatus(status);
            if (m_view) {
                m_view->setConnectionStatus(status.isHealthy);
            }
        },
        [this](const QString& error) {
            // Error
            if (m_view) {
                m_view->setConnectionStatus(false);
            }
            handleApiError("Health Check fehlgeschlagen: " + error);
        });
}

void MainPresenter::loadMachines() {
    m_apiClient->getMachines(
        [this](const QJsonDocument& doc) {
            // Success
            QVector<Machine> machines;
            auto array = doc.array();

            for (const auto& item : array) {
                machines.append(Machine::fromJson(item.toObject()));
            }

            m_model->setMachines(machines);
        },
        [this](const QString& error) { handleApiError("Maschinen laden fehlgeschlagen: " + error); });
}

void MainPresenter::loadMeasurements(int machineId) {
    m_apiClient->getMeasurements(
        machineId, QString(), 100,
        [this, machineId](const QJsonDocument& doc) {
            // Success
            QVector<Measurement> measurements;
            auto array = doc.array();

            for (const auto& item : array) {
                measurements.append(Measurement::fromJson(item.toObject()));
            }

            m_model->addMeasurements(machineId, measurements);
            if (m_view) {
                m_view->updateChart(measurements);
            }
        },
        [this](const QString& error) { handleApiError("Messungen laden fehlgeschlagen: " + error); });
}

void MainPresenter::loadEvents(int machineId) {
    m_apiClient->getEvents(
        machineId, QString(), 50,
        [this](const QJsonDocument& doc) {
            // Success
            QVector<Event> events;
            auto array = doc.array();

            for (const auto& item : array) {
                events.append(Event::fromJson(item.toObject()));
            }

            m_model->addEvents(events);
        },
        [this](const QString& error) { handleApiError("Events laden fehlgeschlagen: " + error); });
}

// ==================== Slots ====================

void MainPresenter::onHealthCheckCompleted() {
    if (!m_view) {
        return;
    }
    auto status = m_model->healthStatus();
    m_view->setConnectionStatus(status.isHealthy);
}

void MainPresenter::onMachinesLoaded() {
    if (!m_view) {
        return;
    }
    auto machines = m_model->machines();
    m_view->updateMachineList(machines);
}

void MainPresenter::onMeasurementsLoaded() {
    if (!m_view) {
        return;
    }
    // Update chart with latest data
    int machineId = m_model->selectedMachineId();
    if (machineId > 0) {
        auto measurements = m_model->getMeasurements(machineId);
        m_view->updateChart(measurements);
    }
}

void MainPresenter::onEventsLoaded() {
    if (!m_view) {
        return;
    }
    int machineId = m_model->selectedMachineId();
    auto events = m_model->getEvents(machineId);
    m_view->updateEventsTable(events);
}

// ==================== Error Handling ====================

void MainPresenter::handleApiError(const QString& error) {
    if (!m_view) {
        return;
    }
    m_view->showError(error);
    m_view->setConnectionStatus(false);
}

}  // namespace MachinaMindAIAgent
