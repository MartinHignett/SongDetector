#include <QJsonDocument>
#include <QJsonObject>      // <- This is what you're missing
#include <QJsonValue>
#include <QJsonArray>       // If you're also working with arrays
#include <qlist.h>

#include "song.h"

#define SONG_SECTION_TYPE QStringLiteral("SONG")
#define METADATA_ALBUM_FIELD QStringLiteral("Album")
#define METADATA_LABEL_FIELD QStringLiteral("Label")
#define METADATA_RELEASE_DATE_FIELD QStringLiteral("Released")

Song::Song() :
    m_found(false) {
}

Song::Song(QString title, QString artist) :
    m_title(title),
    m_artist(artist),
    m_found(true) {
}

/* Destructor */
Song::~Song() {
}

/* JSON Parser */
Song* Song::fromJsonDocument(const QJsonDocument& json) {
    static QStringList requiredFields = {"title", "subtitle"};

    const QJsonObject rootObject = json.object();
    const QJsonValue trackRef = json["track"];

    if (!trackRef.isObject()) {
        qDebug() << "Shazam couldn't identify song";
        return new Song();
    }

    const auto track = trackRef.toObject();
    for (const QString &field : requiredFields) {
        if (!track.contains(field)) {
            qDebug() << "Missing field: " << field;
            return new Song();
        }
    }

    // The Shazam JSON schema seems to use subtitle for the arist name
    // I'm not sure how reliable that is...
    const auto song = new Song(track["title"].toString(), track["subtitle"].toString());

    const auto sectionsRef = track["sections"];
    if (sectionsRef.isArray()) {
        song->parseSections(sectionsRef);
    }


    return song;
}

void Song::parseSections(const QJsonValue& sectionsRef) {
    const auto sections = sectionsRef.toArray();
    for (auto &sectionRef : sections) {
        if (sectionRef.isObject()) {
            parseSection(sectionRef);
        }
    }
}

void Song::parseSection(const QJsonValue& sectionRef) {
    const auto section = sectionRef.toObject();

    if (section.contains("type") &&
        section.contains("metadata") &&
        section["type"] == SONG_SECTION_TYPE &&
        section["metadata"].isArray()) {
        const auto metadataRef = section["metadata"];
        parseMetadata(metadataRef);
    }
}

void Song::parseMetadata(const QJsonValue& metadataRef) {
    const auto metadata = metadataRef.toArray();

    for (auto &dataRef : metadata) {
        if (!dataRef.isObject()) {
            break;
        }

        const auto data = dataRef.toObject();

        if (data.contains("title") &&
            data.contains("text")) {

            if (data["title"] == METADATA_ALBUM_FIELD) {
                m_album = data["text"].toString();
            }
        }
    }
}

/* Getters */
bool Song::getFound() {
    return m_found;
}

QString Song::getTitle() {
    return m_title;
}

QString Song::getAlbum() {
    return m_album;
}

QString Song::getArtist() {
    return m_artist;
}

int Song::getTrack() {
    return m_track;
}
