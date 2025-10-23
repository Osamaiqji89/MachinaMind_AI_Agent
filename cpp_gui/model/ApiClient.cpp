/**
 * @file ApiClient.cpp
 * @brief Implementation des REST API Clients
 */

#include "ApiClient.h"

#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrlQuery>

namespace MachinaMindAIAgent {

ApiClient::ApiClient(const QString& baseUrl, QObject* parent)
    : QObject(parent), m_baseUrl(baseUrl) {
    m_networkManager = new QNetworkAccessManager(this);
}

ApiClient::~ApiClient() = default;

void ApiClient::setBaseUrl(const QString& url) {
    m_baseUrl = url;
}

// ==================== Health ====================

void ApiClient::healthCheck(SuccessCallback onSuccess, ErrorCallback onError) {
    get("/health", QUrlQuery(), onSuccess, onError);
}

// ==================== Machines ====================

void ApiClient::getMachines(SuccessCallback onSuccess, ErrorCallback onError) {
    get("/machines", QUrlQuery(), onSuccess, onError);
}

void ApiClient::getMachine(int machineId, SuccessCallback onSuccess, ErrorCallback onError) {
    get(QString("/machines/%1").arg(machineId), QUrlQuery(), onSuccess, onError);
}

// ==================== Measurements ====================

void ApiClient::getMeasurements(int machineId, const QString& sensorType, int limit,
                                SuccessCallback onSuccess, ErrorCallback onError) {
    QUrlQuery query;
    if (!sensorType.isEmpty()) {
        query.addQueryItem("sensor_type", sensorType);
    }
    query.addQueryItem("limit", QString::number(limit));

    get(QString("/measurements/%1").arg(machineId), query, onSuccess, onError);
}

// ==================== Events ====================

void ApiClient::getEvents(int machineId, const QString& level, int limit,
                          SuccessCallback onSuccess, ErrorCallback onError) {
    QUrlQuery query;
    if (machineId > 0) {
        query.addQueryItem("machine_id", QString::number(machineId));
    }
    if (!level.isEmpty()) {
        query.addQueryItem("level", level);
    }
    query.addQueryItem("limit", QString::number(limit));

    get("/events", query, onSuccess, onError);
}

// ==================== Chat ====================

void ApiClient::sendChatMessage(const QString& message, int machineId, SuccessCallback onSuccess,
                                ErrorCallback onError) {
    QJsonObject body;
    body["message"] = message;

    if (machineId > 0) {
        body["machine_id"] = machineId;
    }
    body["context_limit"] = 10;

    // DEBUG: Log what we're sending
    qDebug() << "=== SENDING CHAT MESSAGE ===";
    qDebug() << "URL:" << m_baseUrl + "/chat";
    qDebug() << "Body:" << QJsonDocument(body).toJson(QJsonDocument::Compact);
    qDebug() << "===========================";

    post("/chat", body, onSuccess, onError);
}

// ==================== Analysis ====================

void ApiClient::analyzeMachine(int machineId, const QString& sensorType, int timeRangeMinutes,
                               SuccessCallback onSuccess, ErrorCallback onError) {
    QJsonObject body;
    body["machine_id"] = machineId;

    if (!sensorType.isEmpty()) {
        body["sensor_type"] = sensorType;
    }
    body["time_range_minutes"] = timeRangeMinutes;

    post("/analyze", body, onSuccess, onError);
}

// ==================== Reports ====================

void ApiClient::getReports(int machineId, int limit, SuccessCallback onSuccess,
                           ErrorCallback onError) {
    QUrlQuery query;
    if (machineId > 0) {
        query.addQueryItem("machine_id", QString::number(machineId));
    }
    query.addQueryItem("limit", QString::number(limit));

    get("/reports", query, onSuccess, onError);
}

// ==================== HTTP Methods ====================

void ApiClient::get(const QString& endpoint, const QUrlQuery& query, SuccessCallback onSuccess,
                    ErrorCallback onError) {
    QUrl url(m_baseUrl + endpoint);
    if (!query.isEmpty()) {
        url.setQuery(query);
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager->get(request);
    handleReply(reply, onSuccess, onError);
}

void ApiClient::post(const QString& endpoint, const QJsonObject& body, SuccessCallback onSuccess,
                     ErrorCallback onError) {
    QUrl url(m_baseUrl + endpoint);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonDocument doc(body);
    QByteArray data = doc.toJson();

    qDebug() << "POST Request:" << url.toString();
    qDebug() << "POST Body:" << data;

    QNetworkReply* reply = m_networkManager->post(request, data);
    
    qDebug() << "Reply object created:" << reply;
    qDebug() << "Calling handleReply...";
    
    handleReply(reply, onSuccess, onError);
}

void ApiClient::handleReply(QNetworkReply* reply, SuccessCallback onSuccess,
                            ErrorCallback onError) {
    qDebug() << "handleReply called, setting up connection...";
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess, onError]() {
        reply->deleteLater();

        qDebug() << "=== REPLY RECEIVED ===";
        qDebug() << "Status:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "Error:" << reply->error();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            qDebug() << "Response:" << data;
            QJsonDocument doc = QJsonDocument::fromJson(data);

            if (!doc.isNull()) {
                onSuccess(doc);
                emit requestCompleted();
            } else {
                QString error = "Invalid JSON response";
                qDebug() << "ERROR:" << error;
                onError(error);
                emit connectionError(error);
            }
        } else {
            QString error = reply->errorString();
            QByteArray data = reply->readAll();
            qDebug() << "ERROR:" << error;
            qDebug() << "Response body:" << data;
            onError(error);
            emit connectionError(error);
        }
        qDebug() << "=====================";
    });
}

}  // namespace MachinaMindAIAgent
