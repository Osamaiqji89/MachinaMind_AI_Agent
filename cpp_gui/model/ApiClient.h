/**
 * @file ApiClient.h
 * @brief REST API Client für Backend-Kommunikation
 */

#ifndef MachinaMindAIAgent_API_CLIENT_H
#define MachinaMindAIAgent_API_CLIENT_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <functional>

#include "DTOs.h"

namespace MachinaMindAIAgent {

/**
 * @brief Asynchroner REST API Client
 * 
 * Verwendet QNetworkAccessManager für nicht-blockierende HTTP-Requests
 */
class ApiClient : public QObject {
    Q_OBJECT

public:
    explicit ApiClient(const QString& baseUrl = "http://localhost:8000", QObject* parent = nullptr);
    ~ApiClient() override;

    // Callback Types
    using SuccessCallback = std::function<void(const QJsonDocument&)>;
    using ErrorCallback = std::function<void(const QString&)>;

    // API Methods
    void healthCheck(SuccessCallback onSuccess, ErrorCallback onError);
    void getMachines(SuccessCallback onSuccess, ErrorCallback onError);
    void getMachine(int machineId, SuccessCallback onSuccess, ErrorCallback onError);

    void getMeasurements(int machineId, const QString& sensorType, int limit,
                         SuccessCallback onSuccess, ErrorCallback onError);

    void getEvents(int machineId, const QString& level, int limit, SuccessCallback onSuccess,
                   ErrorCallback onError);

    void sendChatMessage(const QString& message, int machineId, SuccessCallback onSuccess,
                         ErrorCallback onError);

    void analyzeMachine(int machineId, const QString& sensorType, int timeRangeMinutes,
                        SuccessCallback onSuccess, ErrorCallback onError);

    void getReports(int machineId, int limit, SuccessCallback onSuccess, ErrorCallback onError);

    // Configuration
    void setBaseUrl(const QString& url);
    QString baseUrl() const { return m_baseUrl; }

signals:
    void connectionError(const QString& error);
    void requestCompleted();

private:
    void get(const QString& endpoint, const QUrlQuery& query, SuccessCallback onSuccess,
             ErrorCallback onError);

    void post(const QString& endpoint, const QJsonObject& body, SuccessCallback onSuccess,
              ErrorCallback onError);

    void handleReply(QNetworkReply* reply, SuccessCallback onSuccess, ErrorCallback onError);

    QNetworkAccessManager* m_networkManager;
    QString m_baseUrl;
};

}  // namespace MachinaMindAIAgent

#endif  // MachinaMindAIAgent_API_CLIENT_H
