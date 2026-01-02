#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>      // <- This is what you're missing
#include <QJsonValue>
#include <QJsonArray>       // If you're also working with arrays
#include <qlist.h>

#include "shazam_response.h"

#define SECTION_TYPE QStringLiteral("type")
#define SECTION_METADATA QStringLiteral("metadata")
#define SONG_SECTION_TYPE QStringLiteral("SONG")
#define METADATA_ALBUM_FIELD QStringLiteral("Album")
#define METADATA_LABEL_FIELD QStringLiteral("Label")
#define METADATA_RELEASE_DATE_FIELD QStringLiteral("Released")
#define METADATA_TILE QStringLiteral("title")
#define METADATA_TEXT QStringLiteral("text")

ShazamResponse::ShazamResponse() :
    m_found(false) {
}

ShazamResponse::ShazamResponse(QString title, QString artist) :
    m_title(title),
    m_artist(artist),
    m_found(true) {
}

/* Destructor */
ShazamResponse::~ShazamResponse() {
}

/* JSON Parser */
ShazamResponse ShazamResponse::fromJsonDocument(const QJsonDocument& json) {
    static QStringList requiredFields = {"title", "subtitle"};
    const QJsonObject rootObject = json.object();
    const QJsonValue trackRef = json["track"];

    if (!trackRef.isObject()) {
        qWarning() << "Shazam couldn't identify song";
        return ShazamResponse();
    }

    const auto track = trackRef.toObject();
    for (const QString &field : requiredFields) {
        if (!track.contains(field)) {
            qWarning() << "Missing field: " << field;
            return ShazamResponse();
        }
    }

    // The Shazam JSON schema seems to use subtitle for the arist name
    // I'm not sure how reliable that is...
    auto shazamResponse = ShazamResponse(track["title"].toString(), track["subtitle"].toString());
    const auto sectionsRef = track["sections"];
    if (sectionsRef.isArray()) {
        shazamResponse.parseSections(sectionsRef);
    }

    return shazamResponse;
}

void ShazamResponse::parseSections(const QJsonValue& sectionsRef) {
    const auto sections = sectionsRef.toArray();
    for (auto &sectionRef : sections) {
        if (sectionRef.isObject()) {
            parseSection(sectionRef);
        }
    }
}

void ShazamResponse::parseSection(const QJsonValue& sectionRef) {
    const auto section = sectionRef.toObject();

    if (section.contains(SECTION_TYPE) &&
        section.contains(SECTION_METADATA) &&
        section[SECTION_TYPE] == SONG_SECTION_TYPE &&
        section[SECTION_METADATA].isArray()) {
        const auto metadataRef = section[SECTION_METADATA];
        parseMetadata(metadataRef);
    }
}

void ShazamResponse::parseMetadata(const QJsonValue& metadataRef) {
    const auto metadata = metadataRef.toArray();

    for (auto &dataRef : metadata) {
        if (!dataRef.isObject()) {
            break;
        }

        const auto data = dataRef.toObject();

        if (data.contains(METADATA_TILE) &&
            data.contains(METADATA_TEXT)) {

            if (data[METADATA_TILE] == METADATA_ALBUM_FIELD) {
                m_album = data[METADATA_TEXT].toString();
            }
        }
    }
}

/*
 * Getters
 */

bool ShazamResponse::getFound() const {
    return m_found;
}

QString ShazamResponse::getTitle() const {
    return m_title;
}

QString ShazamResponse::getAlbum() const {
    return m_album;
}

QString ShazamResponse::getArtist() const {
    return m_artist;
}

int ShazamResponse::getTrack() const {
    return m_track;
}
