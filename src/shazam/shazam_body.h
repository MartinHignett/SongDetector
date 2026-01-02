#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

class ShazamGelocation {
    public:
        ShazamGelocation();

        // Convert to JSON object
        QJsonObject toJsonObject() const;

    private:
        float altitude;
        float latitude;
        float longitude;
};

class ShazamSignature {
    public:
        ShazamSignature(const QString& uri, const int sample_ms, const int timestamp);

        // Convert to JSON object
        QJsonObject toJsonObject() const;

    private:
        const int         samplems;
        const int         timestamp;
        const QString     uri;
};

class ShazamBody {
    public:
        ShazamBody(const QString& uri, const int sample_ms);

        // Convert to JSON document for REST
        QJsonDocument       toJsonDocument() const;

    private:
        const int                 timestamp;
        const ShazamGelocation    geolocation;
        const ShazamSignature     signature;
        const QString             timezone;
};
