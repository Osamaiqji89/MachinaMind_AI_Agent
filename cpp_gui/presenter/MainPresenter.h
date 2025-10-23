/**
 * @file MainPresenter.h
 * @brief Presenter im MVP-Pattern - Business Logic & Orchestrierung
 */

#ifndef MachinaMindAIAgent_PRESENTER_H
#define MachinaMindAIAgent_PRESENTER_H

#include <QObject>
#include <QTimer>

#include "../model/ApiClient.h"
#include "../model/DataModel.h"

namespace MachinaMindAIAgent {

// Forward declarations
class IMainView;

/**
 * @brief Main Presenter - Vermittelt zwischen View und Model
 * 
 * Verantwortlichkeiten:
 * - UI-Event-Handling
 * - API-Orchestrierung
 * - View-Updates
 * - Input-Validation
 */
class MainPresenter : public QObject {
    Q_OBJECT

public:
    explicit MainPresenter(IMainView* view, DataModel* model, ApiClient* apiClient,
                           QObject* parent = nullptr);
    ~MainPresenter() override;

    // Initialization
    void initialize();
    void setView(IMainView* view);

    // User Actions
    void onMachineSelected(int machineId);
    void onRefreshClicked();
    void onSendChatMessage(const QString& message);
    void onAnalyzeClicked();
    void onConnectClicked(const QString& serverUrl);

    // Auto-refresh
    void setAutoRefreshEnabled(bool enabled);
    void setRefreshInterval(int seconds);

private slots:
    void onAutoRefresh();
    void onHealthCheckCompleted();
    void onMachinesLoaded();
    void onMeasurementsLoaded();
    void onEventsLoaded();

private:
    // API Calls
    void loadHealthStatus();
    void loadMachines();
    void loadMeasurements(int machineId);
    void loadEvents(int machineId);

    // Error Handling
    void handleApiError(const QString& error);

    // View Interface
    IMainView* m_view;

    // Model & API
    DataModel* m_model;
    ApiClient* m_apiClient;

    // Auto-refresh
    QTimer* m_refreshTimer;
    bool m_autoRefreshEnabled = false;
};

/**
 * @brief View Interface (für Dependency Inversion)
 * 
 * Definiert Methoden, die die View implementieren muss
 */
class IMainView {
public:
    virtual ~IMainView() = default;

    virtual void showError(const QString& message) = 0;
    virtual void showInfo(const QString& message) = 0;
    virtual void setConnectionStatus(bool connected) = 0;
    virtual void appendChatMessage(const QString& role, const QString& message) = 0;
    virtual void updateMachineList(const QVector<Machine>& machines) = 0;
    virtual void updateChart(const QVector<Measurement>& measurements) = 0;
    virtual void updateEventsTable(const QVector<Event>& events) = 0;
    virtual void setAnalysisResult(const QString& summary, int anomalyCount) = 0;
};

}  // namespace MachinaMindAIAgent

#endif  // MachinaMindAIAgent_PRESENTER_H
