#pragma once

#include <QObject>
#include <QString>
#include <qtmetamacros.h>

class ShazamResponse {
    public:
        /*
        * Default constructor
        *
        * Creates an empty instance with found set to false
        */
        ShazamResponse();

        /* Destructor */
        ~ShazamResponse();

        static ShazamResponse fromJsonDocument(const QJsonDocument& document);

        /* Getters */
        bool        getFound() const;
        QString     getTitle() const;
        QString     getArtist() const;
        QString     getAlbum() const;
        int         getTrack() const;
        QString     getShazamId() const;
        QString     getIsrc() const;

    private:
        /* Constructors */

        /*
        * Creates a new instance
        */
        ShazamResponse(QString title, QString artist);

        /* true if the song was found, otherwise false */
        bool        m_found;

        /* Song data */
        QString     m_title;
        QString     m_artist;
        QString     m_album;
        int         m_track;

        /* Shazam's own track identifier, and the track's ISRC if known */
        QString     m_shazamId;
        QString     m_isrc;

        /* JSON parser */
        void        parseSections(const QJsonValue& sectionsRef);
        void        parseSection(const QJsonValue& sectionRef);
        void        parseMetadata(const QJsonValue& metadataRef);

};
