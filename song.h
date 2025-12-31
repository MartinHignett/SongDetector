#ifndef SONG_H
#define SONG_H

#include <QJsonDocument>
#include <QObject>

class Song: public QObject {
    Q_OBJECT;

    public:
        static Song* fromJsonDocument(const QJsonDocument& document);

        /* Getters */
        bool        getFound();
        QString     getTitle();
        QString     getArtist();
        QString     getAlbum();
        int         getTrack();

    private:
        /* Constructors */

        /*
         * Default constructor
         *
         * Creates an empty instance with found set to false
         */
        Song();

        /*
         * Creates a new instance
         */
        Song(QString title, QString artist);

        /* Destructor */
        ~Song();

        /* Song data */
        bool        m_found;
        QString     m_title;
        QString     m_artist;
        QString     m_album;
        int         m_track;

        /* JSON parser */
        void        parseSections(const QJsonValue& sectionsRef);
        void        parseSection(const QJsonValue& sectionRef);
        void        parseMetadata(const QJsonValue& metadataRef);
};

#endif // SONG_H
