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
        "    count INTEGER NOT NULL DEFAULT 1 CHECK (count >= 1),"
        "    favourite BOOLEAN NOT NULL DEFAULT 0"
        ")",

        "CREATE INDEX IF NOT EXISTS idx_songs_album_uuid ON songs(album_uuid)",
        "CREATE INDEX IF NOT EXISTS idx_songs_artist_uuid ON songs(artist_uuid)",
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

    // A song is considered a repeat detection (and just bumps `count`)
    // when it matches on artist + name + track number.
    QSqlQuery findSongQuery(m_database);
    findSongQuery.prepare(
        "SELECT song_uuid FROM songs "
        "WHERE artist_uuid = :artist_uuid AND name = :name AND track_number = :track_number"
    );
    findSongQuery.bindValue(":artist_uuid", artistUuid);
    findSongQuery.bindValue(":name", response.getTitle());
    findSongQuery.bindValue(":track_number", response.getTrack());

    if (!findSongQuery.exec()) {
        qWarning() << "Failed to look up existing song:" << findSongQuery.lastError().text();
        m_database.rollback();
        return false;
    }

    bool success = false;

    if (findSongQuery.next()) {
        QSqlQuery updateQuery(m_database);
        updateQuery.prepare("UPDATE songs SET count = count + 1 WHERE song_uuid = :song_uuid");
        updateQuery.bindValue(":song_uuid", findSongQuery.value(0).toString());
        success = updateQuery.exec();

        if (!success) {
            qWarning() << "Failed to increment song count:" << updateQuery.lastError().text();
        }
    } else {
        QSqlQuery insertQuery(m_database);
        insertQuery.prepare(
            "INSERT INTO songs (song_uuid, artist_uuid, album_uuid, name, track_number, count) "
            "VALUES (:song_uuid, :artist_uuid, :album_uuid, :name, :track_number, 1)"
        );
        insertQuery.bindValue(":song_uuid", QUuid::createUuid().toString(QUuid::WithoutBraces));
        insertQuery.bindValue(":artist_uuid", artistUuid);
        insertQuery.bindValue(":album_uuid", albumUuid.isEmpty() ? QVariant() : QVariant(albumUuid));
        insertQuery.bindValue(":name", response.getTitle());
        insertQuery.bindValue(":track_number", response.getTrack());
        success = insertQuery.exec();

        if (!success) {
            qWarning() << "Failed to record detected song:" << insertQuery.lastError().text();
        }
    }

    if (!success) {
        m_database.rollback();
        return false;
    }

    return m_database.commit();
}
