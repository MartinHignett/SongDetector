#pragma once

#include <QObject>
#include <qstringview.h>
#include <qtmetamacros.h>

#include "shazam_response.h"

#define SHAZAM_URL QStringLiteral("https://amp.shazam.com/discovery/v5/en/US/android/-/tag/")
#define SHAZAM_QUERY_PARAMS QStringLiteral("?sync=true&webv3=true&sampling=true&connected=&shazamapiversion=v3&sharehub=true&video=v3")

class Shazam : public QObject {
    Q_OBJECT

    public:
        Shazam(QObject* parent);

        void    detectFromUri(const QString& uri, const int bufferLengthInSeconds);

    protected slots:
        void    parseShazamResponse(const QJsonDocument& shazamJsonDocument);
        void    onShazamError();
        void    onShazamResponse();

    signals:
        void    detectionComplete(const ShazamResponse& response);
};
