#include <QNetworkRequest>
#include <QRestAccessManager>
#include <QRestReply>
#include <QUuid>
#include <qobject.h>
#include <qobjectdefs.h>

#include "shazam.h"
#include "shazam_body.h"
#include "shazam_response.h"

Shazam::Shazam(QObject* parent) : QObject(parent) {
}

void Shazam::detectFromUri(const QString& uri, const int bufferLengthInSeconds) {
    const auto networkAccessManager = new QNetworkAccessManager(this);
    const auto restAccessManager = new QRestAccessManager(networkAccessManager, this);

    ShazamBody shazamBody(uri, bufferLengthInSeconds);
    const auto jsonBody = shazamBody.toJsonDocument();

    const QString url =
        SHAZAM_URL +
        QUuid::createUuid().toString(QUuid::WithoutBraces) +
        "/" +
        QUuid::createUuid().toString(QUuid::WithoutBraces) +
        SHAZAM_QUERY_PARAMS;

    auto request = QNetworkRequest(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::UserAgentHeader, "Dalvik/2.1.0 (Linux; U; Android 6.0.1; SM-G920F Build/MMB29K)");
    request.setRawHeader("Accept", "*/*");
    request.setRawHeader("Connection", "keep-alive");
    request.setRawHeader("Content-Language", "en_US");

    const auto response = restAccessManager->post(request, jsonBody);
    QObject::connect(response, &QNetworkReply::finished, this, [this, response, restAccessManager, networkAccessManager]() {
        qDebug() << "Shazam finished!";

        if (response) {
            QRestReply restResponse(response);
            if (!restResponse.isSuccess()) {
                qWarning() << "Error returned by Shazam";
                QMetaObject::invokeMethod(
                    this,
                    "onShazamError",
                    Qt::QueuedConnection);
            } else {
                const auto jsonResponse = restResponse.readJson();
                QMetaObject::invokeMethod(
                    this,
                    "parseShazamResponse",
                    Qt::QueuedConnection,
                    Q_ARG(const QJsonDocument, jsonResponse.value()));
            }
            response->deleteLater();
        }

        restAccessManager->deleteLater();
        networkAccessManager->deleteLater();
    });
}

void Shazam::parseShazamResponse(const QJsonDocument& shazamJsonDocument) {
    const auto shazamResponse = ShazamResponse::fromJsonDocument(shazamJsonDocument);

    qDebug() << "Found song: " << shazamResponse.getFound();

    if (shazamResponse.getFound()) {
        qDebug() << "Song name: " << shazamResponse.getTitle() << " Arist: " << shazamResponse.getArtist();
        detectionComplete(shazamResponse);
    }
}

void Shazam::onShazamError() {
    ShazamResponse shazamResponse;
    detectionComplete(shazamResponse);
}
