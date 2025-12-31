#include <QJsonDocument>
#include <QJsonObject>      // <- This is what you're missing
#include <QJsonValue>
#include <QJsonArray>       // If you're also working with arrays
#include <qlist.h>

#include "song.h"

#define SECTION_TYPE QStringLiteral("type")
#define SECTION_METADATA QStringLiteral("metadata")
#define SONG_SECTION_TYPE QStringLiteral("SONG")
#define METADATA_ALBUM_FIELD QStringLiteral("Album")
#define METADATA_LABEL_FIELD QStringLiteral("Label")
#define METADATA_RELEASE_DATE_FIELD QStringLiteral("Released")
#define METADATA_TILE QStringLiteral("title")
#define METADATA_TEXT QStringLiteral("text")

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

    if (section.contains(SECTION_TYPE) &&
        section.contains(SECTION_METADATA) &&
        section[SECTION_TYPE] == SONG_SECTION_TYPE &&
        section[SECTION_METADATA].isArray()) {
        const auto metadataRef = section[SECTION_METADATA];
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

        if (data.contains(METADATA_TILE) &&
            data.contains(METADATA_TEXT)) {

            if (data[METADATA_TILE] == METADATA_ALBUM_FIELD) {
                m_album = data[METADATA_TEXT].toString();
            }
        }
    }
}

/* Getters */
bool Song::getFound() const {
    return m_found;
}

QString Song::getTitle() const {
    return m_title;
}

QString Song::getAlbum() const {
    return m_album;
}

QString Song::getArtist() const {
    return m_artist;
}

int Song::getTrack() const {
    return m_track;
}
