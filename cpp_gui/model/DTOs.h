/**
 * @file DTOs.h
 * @brief Data Transfer Objects für MachinaMindAIAgent
 * 
 * Definiert alle Datenstrukturen für API-Kommunikation
 */

#ifndef MachinaMindAIAgent_DTOS_H
#define MachinaMindAIAgent_DTOS_H

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector>

namespace MachinaMindAIAgent {

/**
 * @brief Maschinen-Daten
 */
struct Machine {
    int id = 0;
    QString name;
    QString type;
    QString location;
    QDateTime createdAt;

    static Machine fromJson(const QJsonObject& json) {
        Machine m;
        m.id = json["id"].toInt();
        m.name = json["name"].toString();
        m.type = json["type"].toString();
        m.location = json["location"].toString();
        if (json.contains("created_at")) {
            m.createdAt = QDateTime::fromString(json["created_at"].toString(), Qt::ISODate);
        }
        return m;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["type"] = type;
        obj["location"] = location;
        if (createdAt.isValid()) {
            obj["created_at"] = createdAt.toString(Qt::ISODate);
        }
        return obj;
    }
};

/**
 * @brief Sensor-Messwert
 */
struct Measurement {
    int id = 0;
    int machineId = 0;
    QDateTime timestamp;
    QString sensorType;
    double value = 0.0;
    QString unit;

    static Measurement fromJson(const QJsonObject& json) {
        Measurement m;
        m.id = json["id"].toInt();
        m.machineId = json["machine_id"].toInt();
        m.timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);
        m.sensorType = json["sensor_type"].toString();
        m.value = json["value"].toDouble();
        m.unit = json["unit"].toString();
        return m;
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["machine_id"] = machineId;
        obj["timestamp"] = timestamp.toString(Qt::ISODate);
        obj["sensor_type"] = sensorType;
        obj["value"] = value;
        obj["unit"] = unit;
        return obj;
    }
};

/**
 * @brief Event (Warnung, Fehler)
 */
struct Event {
    enum Level { Info, Warning, Error, Critical };

    int id = 0;
    int machineId = 0;
    QDateTime timestamp;
    Level level = Info;
    QString message;

    static Event fromJson(const QJsonObject& json) {
        Event e;
        e.id = json["id"].toInt();
        e.machineId = json["machine_id"].toInt();
        e.timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);

        QString levelStr = json["level"].toString().toUpper();
        if (levelStr == "WARNING")
            e.level = Warning;
        else if (levelStr == "ERROR")
            e.level = Error;
        else if (levelStr == "CRITICAL")
            e.level = Critical;
        else
            e.level = Info;

        e.message = json["message"].toString();
        return e;
    }

    QString levelString() const {
        switch (level) {
            case Warning:
                return "WARNING";
            case Error:
                return "ERROR";
            case Critical:
                return "CRITICAL";
            default:
                return "INFO";
        }
    }
};

/**
 * @brief Chat-Nachricht
 */
struct ChatMessage {
    enum Role { User, Assistant, System };

    Role role = User;
    QString content;
    QDateTime timestamp;
    QStringList sources;

    static ChatMessage fromJson(const QJsonObject& json) {
        ChatMessage msg;
        msg.content = json["answer"].toString();
        msg.role = Assistant;
        msg.timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);

        // Sources
        if (json.contains("sources")) {
            auto sourcesArray = json["sources"].toArray();
            for (const auto& src : sourcesArray) {
                msg.sources.append(src.toString());
            }
        }

        return msg;
    }

    QString roleString() const {
        switch (role) {
            case Assistant:
                return "Assistant";
            case System:
                return "System";
            default:
                return "User";
        }
    }
};

/**
 * @brief Analyse-Ergebnis
 */
struct AnalysisResult {
    int machineId = 0;
    int anomaliesDetected = 0;
    QString summary;
    QVector<QJsonObject> details;
    QDateTime timestamp;

    static AnalysisResult fromJson(const QJsonObject& json) {
        AnalysisResult result;
        result.machineId = json["machine_id"].toInt();
        result.anomaliesDetected = json["anomalies_detected"].toInt();
        result.summary = json["summary"].toString();
        result.timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);

        // Details
        if (json.contains("details")) {
            auto detailsArray = json["details"].toArray();
            for (const auto& detail : detailsArray) {
                result.details.append(detail.toObject());
            }
        }

        return result;
    }
};

/**
 * @brief API Health Status
 */
struct HealthStatus {
    bool isHealthy = false;
    QDateTime timestamp;
    QMap<QString, int> dbStats;

    static HealthStatus fromJson(const QJsonObject& json) {
        HealthStatus status;
        status.isHealthy = (json["status"].toString() == "healthy");
        status.timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);

        if (json.contains("db_stats")) {
            auto stats = json["db_stats"].toObject();
            for (auto it = stats.begin(); it != stats.end(); ++it) {
                status.dbStats[it.key()] = it.value().toInt();
            }
        }

        return status;
    }
};

}  // namespace MachinaMindAIAgent

#endif  // MachinaMindAIAgent_DTOS_H
