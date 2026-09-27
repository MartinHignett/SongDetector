#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>

class ShazamResponse;

/*
 * Persists identified songs to a local SQLite database, creating the
 * database file and schema on first use if they don't already exist.
 */
class SongDatabase : public QObject {
    Q_OBJECT

public:
    explicit SongDatabase(QObject* parent = nullptr);
    ~SongDatabase();

    /*
    * Opens the database connection, creating the database file and
    * schema if required. Returns true on success.
    */
    bool open();

    bool recordDetection(const ShazamResponse& response);

private:
    QSqlDatabase    m_database;
    QString         m_connectionName;

    QString         databaseFilePath() const;
    bool            createSchema();
    QString         findOrCreateArtist(const QString& name);
    QString         findOrCreateAlbum(const QString& name);
};
