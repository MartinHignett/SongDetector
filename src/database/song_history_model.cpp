#include "song_history_model.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

#include <algorithm>

namespace {

// history.identified_on is stored as UTC ISO-8601 with millisecond
// precision; displayed in the user's local time, rounded to the nearest
// second since sub-second precision isn't meaningful to a person.
QString formatIdentifiedOn(const QString& raw) {
    QDateTime dateTime = QDateTime::fromString(raw, Qt::ISODateWithMs);

    if (!dateTime.isValid()) {
        return raw;
    }

    dateTime = dateTime.toLocalTime();

    const int msec = dateTime.time().msec();
    dateTime = dateTime.addMSecs(-msec);
    if (msec >= 500) {
        dateTime = dateTime.addSecs(1);
    }

    return dateTime.toString("yyyy-MM-dd HH:mm:ss");
}

}

SongHistoryModel::SongHistoryModel(QObject* parent)
    : QAbstractTableModel(parent) {
}

void SongHistoryModel::load(const QSqlDatabase& database) {
    beginResetModel();

    m_database = database;
    m_rows.clear();

    QSqlQuery query(m_database);
    query.exec(
        "SELECT history.song_uuid,"
        "       max(history.identified_on),"
        "       count(*) as count,"
        "       artists.name,"
        "       songs.name,"
        "       albums.name,"
        "       songs.track_number,"
        "       songs.favourite "
        "FROM history "
        "JOIN songs ON songs.song_uuid = history.song_uuid "
        "JOIN artists ON artists.artist_uuid = songs.artist_uuid "
        "LEFT JOIN albums ON albums.album_uuid = songs.album_uuid "
        "GROUP BY history.song_uuid "
        "ORDER BY history.identified_on DESC"
    );

    if (query.lastError().isValid()) {
        qWarning() << "Failed to load song history:" << query.lastError().text();
    }

    while (query.next()) {
        Row row;
        row.songUuid        = query.value(0).toString();
        row.identifiedOn    = formatIdentifiedOn(query.value(1).toString());
        row.count           = query.value(2).toInt();
        row.artist          = query.value(3).toString();
        row.title           = query.value(4).toString();
        row.album           = query.value(5).toString();
        row.track           = query.value(6).toInt();
        row.favourite       = query.value(7).toBool();
        m_rows.append(row);
    }

    sortRows();

    endResetModel();
}

bool SongHistoryModel::deleteSong(int row) {
    if (row < 0 || row >= m_rows.size()) {
        return false;
    }

    const QString songUuid = m_rows.at(row).songUuid;

    if (!m_database.transaction()) {
        qWarning() << "Failed to start transaction:" << m_database.lastError().text();
        return false;
    }

    // History references songs, so it must be removed first.
    for (const char* sql : {"DELETE FROM history WHERE song_uuid = :song_uuid",
                            "DELETE FROM songs WHERE song_uuid = :song_uuid"}) {
        QSqlQuery query(m_database);
        query.prepare(sql);
        query.bindValue(":song_uuid", songUuid);

        if (!query.exec()) {
            qWarning() << "Failed to delete song:" << query.lastError().text();
            m_database.rollback();
            return false;
        }
    }

    if (!m_database.commit()) {
        qWarning() << "Failed to commit song deletion:" << m_database.lastError().text();
        m_database.rollback();
        return false;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_rows.removeAt(row);
    endRemoveRows();

    return true;
}

void SongHistoryModel::sort(int column, Qt::SortOrder order) {
    if (column < 0 || column >= ColumnCount) {
        return;
    }

    m_sortColumn = column;
    m_sortOrder = order;

    emit layoutAboutToBeChanged({}, QAbstractItemModel::VerticalSortHint);

    const QModelIndexList oldIndexes = persistentIndexList();
    QVector<int> oldRows;
    for (const QModelIndex& index : oldIndexes) {
        oldRows.append(index.row());
    }

    // Track each row's identity through the sort so persistent indexes
    // (e.g. the selection) follow their rows.
    for (int i = 0; i < m_rows.size(); ++i) {
        m_rows[i].sortId = i;
    }

    sortRows();

    QVector<int> newRowForOld(m_rows.size());
    for (int i = 0; i < m_rows.size(); ++i) {
        newRowForOld[m_rows.at(i).sortId] = i;
    }

    QModelIndexList newIndexes;
    for (int i = 0; i < oldIndexes.size(); ++i) {
        newIndexes.append(index(newRowForOld.at(oldRows.at(i)), oldIndexes.at(i).column()));
    }
    changePersistentIndexList(oldIndexes, newIndexes);

    emit layoutChanged({}, QAbstractItemModel::VerticalSortHint);
}

void SongHistoryModel::sortRows() {
    const auto compare = [this](const Row& a, const Row& b) {
        switch (m_sortColumn) {
            case IdentifiedOnColumn: return a.identifiedOn < b.identifiedOn;
            case ArtistColumn:       return QString::compare(a.artist, b.artist, Qt::CaseInsensitive) < 0;
            case TitleColumn:        return QString::compare(a.title, b.title, Qt::CaseInsensitive) < 0;
            case AlbumColumn:        return QString::compare(a.album, b.album, Qt::CaseInsensitive) < 0;
            case TrackColumn:        return a.track < b.track;
            case CountColumn:        return a.count < b.count;
            case FavouriteColumn:    return a.favourite < b.favourite;
            default:                 return false;
        }
    };

    if (m_sortOrder == Qt::AscendingOrder) {
        std::stable_sort(m_rows.begin(), m_rows.end(), compare);
    } else {
        std::stable_sort(m_rows.begin(), m_rows.end(),
                         [&compare](const Row& a, const Row& b) { return compare(b, a); });
    }
}

int SongHistoryModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_rows.size();
}

int SongHistoryModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant SongHistoryModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_rows.size()) {
        return QVariant();
    }

    const Row& row = m_rows.at(index.row());

    if (role == Qt::CheckStateRole && index.column() == FavouriteColumn) {
        return row.favourite ? Qt::Checked : Qt::Unchecked;
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (index.column()) {
        case IdentifiedOnColumn: return row.identifiedOn;
        case ArtistColumn:       return row.artist;
        case TitleColumn:        return row.title;
        case AlbumColumn:        return row.album;
        case TrackColumn:        return row.track > 0 ? QVariant(row.track) : QVariant();
        case CountColumn:        return row.count;
        case ActionsColumn:      return QStringLiteral("...");
        default:                 return QVariant();
    }
}

bool SongHistoryModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.column() != FavouriteColumn || role != Qt::CheckStateRole) {
        return false;
    }

    const QString songUuid = m_rows.at(index.row()).songUuid;
    const bool favourite = (value.toInt() == Qt::Checked);

    QSqlQuery query(m_database);
    query.prepare("UPDATE songs SET favourite = :favourite WHERE song_uuid = :song_uuid");
    query.bindValue(":favourite", favourite);
    query.bindValue(":song_uuid", songUuid);

    if (!query.exec()) {
        qWarning() << "Failed to update song favourite:" << query.lastError().text();
        return false;
    }

    // The same song can appear in multiple history rows; keep them all in sync.
    for (int row = 0; row < m_rows.size(); ++row) {
        if (m_rows.at(row).songUuid == songUuid) {
            m_rows[row].favourite = favourite;
            const QModelIndex changedIndex = this->index(row, FavouriteColumn);
            emit dataChanged(changedIndex, changedIndex, {Qt::CheckStateRole});
        }
    }

    return true;
}

QVariant SongHistoryModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }

    static const QStringList headers = {"Identified On", "Artist", "Count", "Title", "Album", "Track", "Favourite", ""};

    if (section < 0 || section >= headers.size()) {
        return QVariant();
    }

    return headers.at(section);
}

Qt::ItemFlags SongHistoryModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (index.column() == FavouriteColumn) {
        flags |= Qt::ItemIsUserCheckable;
    }

    return flags;
}
