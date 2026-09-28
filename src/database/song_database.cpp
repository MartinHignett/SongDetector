#include "song_database.h"
#include "shazam/shazam_response.h"

#include <QDebug>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

SongDatabase::SongDatabase(QObject* parent)
    : QObject(parent)
    // Each SongDatabase instance gets its own connection name, since Qt's
    // SQL connections are identified globally by name rather than by object.
    , m_connectionName(QUuid::createUuid().toString()) {
}

SongDatabase::~SongDatabase() {
    if (m_database.isOpen()) {
        m_database.close();
    }

    m_database = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
}

QString SongDatabase::databaseFilePath() const {
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    return QDir(dataDir).filePath("song_history.sqlite");
}

bool SongDatabase::open() {
    m_database = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_database.setDatabaseName(databaseFilePath());

    if (!m_database.open()) {
        qWarning() << "Failed to open song history database:" << m_database.lastError().text();
        return false;
    }

    // Foreign key enforcement is off by default per-connection in SQLite.
    QSqlQuery(m_database).exec("PRAGMA foreign_keys = ON");

    return createSchema();
}

bool SongDatabase::createSchema() {
    // CREATE TABLE IF NOT EXISTS makes this safe to run on every startup,
    // whether the database file is brand new or already populated.
    // Tables are created in dependency order (labels/artists before albums,
    // albums before songs) so the foreign key references are always valid.
    static const QStringList statements = {
        "CREATE TABLE IF NOT EXISTS labels ("
        "    label_uuid TEXT PRIMARY KEY,"
        "    name VARCHAR(100) NOT NULL"
        ")",

        "CREATE TABLE IF NOT EXISTS artists ("
        "    artist_uuid TEXT PRIMARY KEY,"
        "    name VARCHAR(100) NOT NULL,"
        "    website VARCHAR(100),"
        "    bandcamp VARCHAR(100),"
        "    discogs VARCHAR(100),"
        "    favourite BOOLEAN NOT NULL DEFAULT 0"
        ")",

        "CREATE INDEX IF NOT EXISTS idx_artists_name ON artists(name)",

        // release_date and label_uuid are nullable rather than NOT NULL:
        // Shazam detections only give us an album name, so this data isn't
        // available until a future metadata-enrichment step fills it in.
        "CREATE TABLE IF NOT EXISTS albums ("
        "    album_uuid TEXT PRIMARY KEY,"
        "    name VARCHAR(100) NOT NULL,"
        "    release_date DATETIME,"
        "    label_uuid TEXT REFERENCES labels(label_uuid),"
        "    website VARCHAR(100),"
        "    bandcamp VARCHAR(100),"
        "    discogs VARCHAR(100),"
        "    favourite BOOLEAN NOT NULL DEFAULT 0"
        ")",

        "CREATE INDEX IF NOT EXISTS idx_albums_name ON albums(name)",
        "CREATE INDEX IF NOT EXISTS idx_albums_release_date ON albums(release_date)",
        "CREATE INDEX IF NOT EXISTS idx_albums_label_uuid ON albums(label_uuid)",

        "CREATE TABLE IF NOT EXISTS songs ("
        "    song_uuid TEXT PRIMARY KEY,"
        "    artist_uuid TEXT NOT NULL REFERENCES artists(artist_uuid),"
        "    album_uuid TEXT REFERENCES albums(album_uuid),"
        "    name VARCHAR(100) NOT NULL,"
        "    track_number INTEGER NOT NULL,"
        "    favourite BOOLEAN NOT NULL DEFAULT 0,"
        "    shazam_id VARCHAR(100),"
        "    isrc VARCHAR(100)"
        ")",

        "CREATE INDEX IF NOT EXISTS idx_songs_album_uuid ON songs(album_uuid)",
        "CREATE INDEX IF NOT EXISTS idx_songs_artist_uuid ON songs(artist_uuid)",
        "CREATE INDEX IF NOT EXISTS idx_songs_shazam_id ON songs(shazam_id)",

        "CREATE TABLE IF NOT EXISTS history ("
        "    identified_on DATETIME NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ', 'now')),"
        "    song_uuid TEXT NOT NULL REFERENCES songs(song_uuid),"
        "    PRIMARY KEY (identified_on, song_uuid)"
        ")",

        "CREATE INDEX IF NOT EXISTS idx_history_song_uuid ON history(song_uuid)",
        "CREATE INDEX IF NOT EXISTS idx_history_identified_on ON history(identified_on)",
    };

    QSqlQuery query(m_database);

    for (const QString& statement : statements) {
        if (!query.exec(statement)) {
            qWarning() << "Failed to apply schema statement:" << statement << "-" << query.lastError().text();
            return false;
        }
    }

    return true;
}

QSqlDatabase SongDatabase::database() const {
    return m_database;
}

QString SongDatabase::findOrCreateArtist(const QString& name) {
    QSqlQuery findQuery(m_database);
    findQuery.prepare("SELECT artist_uuid FROM artists WHERE name = :name");
    findQuery.bindValue(":name", name);

    if (findQuery.exec() && findQuery.next()) {
        return findQuery.value(0).toString();
    }

    const QString artistUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QSqlQuery insertQuery(m_database);
    insertQuery.prepare("INSERT INTO artists (artist_uuid, name) VALUES (:artist_uuid, :name)");
    insertQuery.bindValue(":artist_uuid", artistUuid);
    insertQuery.bindValue(":name", name);

    if (!insertQuery.exec()) {
        qWarning() << "Failed to create artist:" << insertQuery.lastError().text();
        return QString();
    }

    return artistUuid;
}

QString SongDatabase::findOrCreateAlbum(const QString& name) {
    if (name.isEmpty()) {
        return QString();
    }

    QSqlQuery findQuery(m_database);
    findQuery.prepare("SELECT album_uuid FROM albums WHERE name = :name");
    findQuery.bindValue(":name", name);

    if (findQuery.exec() && findQuery.next()) {
        return findQuery.value(0).toString();
    }

    const QString albumUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QSqlQuery insertQuery(m_database);
    insertQuery.prepare("INSERT INTO albums (album_uuid, name) VALUES (:album_uuid, :name)");
    insertQuery.bindValue(":album_uuid", albumUuid);
    insertQuery.bindValue(":name", name);

    if (!insertQuery.exec()) {
        qWarning() << "Failed to create album:" << insertQuery.lastError().text();
        return QString();
    }

    return albumUuid;
}

QString SongDatabase::findOrCreateSong(const QString& artistUuid, const QString& albumUuid, const ShazamResponse& response) {
    QSqlQuery findQuery(m_database);
    findQuery.prepare(
        "SELECT song_uuid FROM songs "
        "WHERE artist_uuid = :artist_uuid AND name = :name AND track_number = :track_number"
    );
    findQuery.bindValue(":artist_uuid", artistUuid);
    findQuery.bindValue(":name", response.getTitle());
    findQuery.bindValue(":track_number", response.getTrack());

    if (findQuery.exec() && findQuery.next()) {
        return findQuery.value(0).toString();
    }

    const QString songUuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString shazamId = response.getShazamId();
    const QString isrc = response.getIsrc();

    QSqlQuery insertQuery(m_database);
    insertQuery.prepare(
        "INSERT INTO songs (song_uuid, artist_uuid, album_uuid, name, track_number, shazam_id, isrc) "
        "VALUES (:song_uuid, :artist_uuid, :album_uuid, :name, :track_number, :shazam_id, :isrc)"
    );
    insertQuery.bindValue(":song_uuid", songUuid);
    insertQuery.bindValue(":artist_uuid", artistUuid);
    insertQuery.bindValue(":album_uuid", albumUuid.isEmpty() ? QVariant() : QVariant(albumUuid));
    insertQuery.bindValue(":name", response.getTitle());
    insertQuery.bindValue(":track_number", response.getTrack());
    insertQuery.bindValue(":shazam_id", shazamId.isEmpty() ? QVariant() : QVariant(shazamId));
    insertQuery.bindValue(":isrc", isrc.isEmpty() ? QVariant() : QVariant(isrc));

    if (!insertQuery.exec()) {
        qWarning() << "Failed to create song:" << insertQuery.lastError().text();
        return QString();
    }

    return songUuid;
}

bool SongDatabase::recordDetection(const ShazamResponse& response) {
    if (!m_database.transaction()) {
        qWarning() << "Failed to start transaction for song detection:" << m_database.lastError().text();
        return false;
    }

    const QString artistUuid = findOrCreateArtist(response.getArtist());

    if (artistUuid.isEmpty()) {
        m_database.rollback();
        return false;
    }

    // A blank album name means findOrCreateAlbum() didn't create/find a
    // row, so albumUuid stays empty and is bound as NULL below.
    const QString albumUuid = findOrCreateAlbum(response.getAlbum());

    // The song identity (artist + name + track number) is reused across
    // repeat detections; each detection just adds a new history row.
    const QString songUuid = findOrCreateSong(artistUuid, albumUuid, response);

    if (songUuid.isEmpty()) {
        m_database.rollback();
        return false;
    }

    QSqlQuery insertHistoryQuery(m_database);
    insertHistoryQuery.prepare("INSERT INTO history (song_uuid) VALUES (:song_uuid)");
    insertHistoryQuery.bindValue(":song_uuid", songUuid);

    if (!insertHistoryQuery.exec()) {
        qWarning() << "Failed to record song history entry:" << insertHistoryQuery.lastError().text();
        m_database.rollback();
        return false;
    }

    return m_database.commit();
}
