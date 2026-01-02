#ifndef SHAZAM_RESPONSE_H
#define SHAZAM_RESPONSE_H

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

        /* JSON parser */
        void        parseSections(const QJsonValue& sectionsRef);
        void        parseSection(const QJsonValue& sectionRef);
        void        parseMetadata(const QJsonValue& metadataRef);

};

#endif // SHAZAM_RESPONSE_H
