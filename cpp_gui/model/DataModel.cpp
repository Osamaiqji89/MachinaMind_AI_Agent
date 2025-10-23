/**
 * @file DataModel.cpp
 * @brief Implementation des Data Models
 */

#include "DataModel.h"

namespace MachinaMindAIAgent {

DataModel::DataModel(QObject* parent) : QObject(parent) {
}

DataModel::~DataModel() = default;

// ==================== Machines ====================

void DataModel::setMachines(const QVector<Machine>& machines) {
    m_machines = machines;
    emit machinesUpdated();
}

Machine DataModel::getMachine(int id) const {
    for (const auto& machine : m_machines) {
        if (machine.id == id) {
            return machine;
        }
    }
    return Machine();  // Empty machine if not found
}

// ==================== Measurements ====================

void DataModel::addMeasurements(int machineId, const QVector<Measurement>& measurements) {
    // Merge new measurements (keep latest)
    auto& existing = m_measurementsByMachine[machineId];

    for (const auto& newMeasurement : measurements) {
        // Check if already exists (avoid duplicates)
        bool found = false;
        for (auto& existing_m : existing) {
            if (existing_m.id == newMeasurement.id) {
                existing_m = newMeasurement;  // Update
                found = true;
                break;
            }
        }
        if (!found) {
            existing.append(newMeasurement);
        }
    }

    // Sort by timestamp (newest first)
    std::sort(existing.begin(), existing.end(),
              [](const Measurement& a, const Measurement& b) { return a.timestamp > b.timestamp; });

    // Limit cache size (keep last 1000)
    if (existing.size() > 1000) {
        existing.resize(1000);
    }

    emit measurementsUpdated(machineId);
}

QVector<Measurement> DataModel::getMeasurements(int machineId, const QString& sensorType) const {
    const auto& allMeasurements = m_measurementsByMachine.value(machineId);

    if (sensorType.isEmpty()) {
        return allMeasurements;
    }

    // Filter by sensor type
    QVector<Measurement> filtered;
    for (const auto& m : allMeasurements) {
        if (m.sensorType == sensorType) {
            filtered.append(m);
        }
    }
    return filtered;
}

Measurement DataModel::getLatestMeasurement(int machineId, const QString& sensorType) const {
    const auto measurements = getMeasurements(machineId, sensorType);
    return measurements.isEmpty() ? Measurement() : measurements.first();
}

// ==================== Events ====================

void DataModel::addEvents(const QVector<Event>& events) {
    for (const auto& newEvent : events) {
        // Check for duplicates
        bool found = false;
        for (const auto& existing : m_events) {
            if (existing.id == newEvent.id) {
                found = true;
                break;
            }
        }
        if (!found) {
            m_events.append(newEvent);
        }
    }

    // Sort by timestamp (newest first)
    std::sort(m_events.begin(), m_events.end(),
              [](const Event& a, const Event& b) { return a.timestamp > b.timestamp; });

    // Limit cache size
    if (m_events.size() > 500) {
        m_events.resize(500);
    }

    emit eventsUpdated();
}

QVector<Event> DataModel::getEvents(int machineId, Event::Level minLevel) const {
    QVector<Event> filtered;

    for (const auto& event : m_events) {
        if ((machineId < 0 || event.machineId == machineId) && event.level >= minLevel) {
            filtered.append(event);
        }
    }

    return filtered;
}

int DataModel::getCriticalEventCount(int machineId) const {
    int count = 0;
    for (const auto& event : m_events) {
        if ((machineId < 0 || event.machineId == machineId) &&
            (event.level == Event::Critical || event.level == Event::Error)) {
            count++;
        }
    }
    return count;
}

// ==================== Chat ====================

void DataModel::addChatMessage(const ChatMessage& message) {
    m_chatHistory.append(message);

    // Limit history
    if (m_chatHistory.size() > 100) {
        m_chatHistory.removeFirst();
    }

    emit chatMessageAdded(message);
}

void DataModel::clearChatHistory() {
    m_chatHistory.clear();
    emit chatMessageAdded(ChatMessage());  // Notify UI
}

// ==================== Analysis ====================

void DataModel::setAnalysisResult(const AnalysisResult& result) {
    m_latestAnalysis = result;
    emit analysisResultUpdated();
}

// ==================== Health ====================

void DataModel::setHealthStatus(const HealthStatus& status) {
    m_healthStatus = status;
    emit healthStatusUpdated();
}

// ==================== Configuration ====================

void DataModel::setSelectedMachineId(int id) {
    if (m_selectedMachineId != id) {
        m_selectedMachineId = id;
        emit selectedMachineChanged(id);
    }
}

}  // namespace MachinaMindAIAgent
