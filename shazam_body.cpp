#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>
#include <QDateTime>
#include <QDebug>

#include <random>

#include "shazam_body.h"

ShazamGelocation::ShazamGelocation() {
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> dis_float(0.0f, 1.0f);

    float fuzz = dis_float(gen) * 15.3f - 7.65f;

    altitude = dis_float(gen) * 400.0f + 100.0f + fuzz;
    latitude = dis_float(gen) * 180.0f - 90.0f + fuzz;
    longitude = dis_float(gen) * 360.0f - 180.0f + fuzz;
}

QJsonObject ShazamGelocation::toJsonObject() const {
    QJsonObject obj;
    obj["altitude"] = altitude;
    obj["latitude"] = latitude;
    obj["longitude"] = longitude;
    return obj;
}

ShazamSignature::ShazamSignature(const QString& uri, int sample_ms, int timestamp) :
    uri(uri),
    samplems(sample_ms),
    timestamp(timestamp) {
}

QJsonObject ShazamSignature::toJsonObject() const {
    QJsonObject obj;
    obj["uri"] = uri;
    obj["samplems"] = samplems;
    obj["timestamp"] = timestamp;
    return obj;
}

ShazamBody::ShazamBody(const QString& uri, int sample_ms) :
    timestamp(QDateTime::currentSecsSinceEpoch()),
    geolocation(),
    signature(uri, sample_ms, timestamp),
    // The id() method return a QByteArray for some reason.
    // We trust that its a sequence of UTF8 characters
    // Should give us something like "Europe/Berlin"
    timezone(QString::fromUtf8(QTimeZone::systemTimeZone().id())) {
}

QJsonDocument ShazamBody::toJsonDocument() const {
    QJsonObject obj;
    obj["timestamp"] = timestamp;
    obj["timezone"] = timezone;
    obj["geolocation"] = geolocation.toJsonObject();
    obj["signature"] = signature.toJsonObject();

    return QJsonDocument(obj);
}
