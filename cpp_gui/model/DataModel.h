/**
 * @file DataModel.h
 * @brief Model Layer - Daten-Cache und Business Logic
 */

#ifndef MachinaMindAIAgent_DATA_MODEL_H
#define MachinaMindAIAgent_DATA_MODEL_H

#include <QMap>
#include <QObject>
#include <QVector>

#include "DTOs.h"

namespace MachinaMindAIAgent {

/**
 * @brief Zentrale Daten-Verwaltung (Model im MVP-Pattern)
 * 
 * Verantwortlich für:
 * - Caching von API-Daten
 * - Daten-Validierung
 * - Business Logic (kein UI-Code!)
 */
class DataModel : public QObject {
    Q_OBJECT

public:
    explicit DataModel(QObject* parent = nullptr);
    ~DataModel() override;

    // Machines
    void setMachines(const QVector<Machine>& machines);
    QVector<Machine> machines() const { return m_machines; }
    Machine getMachine(int id) const;

    // Measurements
    void addMeasurements(int machineId, const QVector<Measurement>& measurements);
    QVector<Measurement> getMeasurements(int machineId, const QString& sensorType = QString()) const;
    Measurement getLatestMeasurement(int machineId, const QString& sensorType) const;

    // Events
    void addEvents(const QVector<Event>& events);
    QVector<Event> getEvents(int machineId = -1, Event::Level minLevel = Event::Info) const;
    int getCriticalEventCount(int machineId = -1) const;

    // Chat History
    void addChatMessage(const ChatMessage& message);
    QVector<ChatMessage> getChatHistory() const { return m_chatHistory; }
    void clearChatHistory();

    // Analysis Results
    void setAnalysisResult(const AnalysisResult& result);
    AnalysisResult getLatestAnalysisResult() const { return m_latestAnalysis; }

    // Health
    void setHealthStatus(const HealthStatus& status);
    HealthStatus healthStatus() const { return m_healthStatus; }

    // Configuration
    void setSelectedMachineId(int id);
    int selectedMachineId() const { return m_selectedMachineId; }

signals:
    void machinesUpdated();
    void measurementsUpdated(int machineId);
    void eventsUpdated();
    void chatMessageAdded(const ChatMessage& message);
    void analysisResultUpdated();
    void healthStatusUpdated();
    void selectedMachineChanged(int machineId);

private:
    // Data Cache
    QVector<Machine> m_machines;
    QMap<int, QVector<Measurement>> m_measurementsByMachine;
    QVector<Event> m_events;
    QVector<ChatMessage> m_chatHistory;
    AnalysisResult m_latestAnalysis;
    HealthStatus m_healthStatus;

    // State
    int m_selectedMachineId = -1;
};

}  // namespace MachinaMindAIAgent

#endif  // MachinaMindAIAgent_DATA_MODEL_H
