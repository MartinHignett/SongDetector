#include "album_history_model.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

AlbumHistoryModel::AlbumHistoryModel(QObject* parent)
    : QAbstractTableModel(parent) {
}

void AlbumHistoryModel::load(const QSqlDatabase& database) {
    beginResetModel();

    m_database = database;
    m_rows.clear();

    QSqlQuery query(m_database);
    query.exec(
        "SELECT albums.album_uuid,"
        "       albums.name,"
        "       labels.name,"
        "       albums.release_date,"
        "       albums.favourite "
        "FROM albums "
        "LEFT JOIN labels ON labels.label_uuid = albums.label_uuid "
        "ORDER BY albums.name"
    );

    if (query.lastError().isValid()) {
        qWarning() << "Failed to load album history:" << query.lastError().text();
    }

    while (query.next()) {
        Row row;
        row.albumUuid       = query.value(0).toString();
        row.name            = query.value(1).toString();
        row.label           = query.value(2).toString();
        row.releaseDate     = query.value(3).toString();
        row.favourite       = query.value(4).toBool();
        m_rows.append(row);
    }

    endResetModel();
}

int AlbumHistoryModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_rows.size();
}

int AlbumHistoryModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant AlbumHistoryModel::data(const QModelIndex& index, int role) const {
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
        case NameColumn:        return row.name;
        case LabelColumn:       return row.label;
        case ReleaseDateColumn: return row.releaseDate;
        default:                return QVariant();
    }
}

bool AlbumHistoryModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.column() != FavouriteColumn || role != Qt::CheckStateRole) {
        return false;
    }

    Row& row = m_rows[index.row()];
    const bool favourite = (value.toInt() == Qt::Checked);

    QSqlQuery query(m_database);
    query.prepare("UPDATE albums SET favourite = :favourite WHERE album_uuid = :album_uuid");
    query.bindValue(":favourite", favourite);
    query.bindValue(":album_uuid", row.albumUuid);

    if (!query.exec()) {
        qWarning() << "Failed to update album favourite:" << query.lastError().text();
        return false;
    }

    row.favourite = favourite;
    emit dataChanged(index, index, {Qt::CheckStateRole});

    return true;
}

QVariant AlbumHistoryModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }

    static const QStringList headers = {"Album", "Label", "Release Date", "Favourite"};

    if (section < 0 || section >= headers.size()) {
        return QVariant();
    }

    return headers.at(section);
}

Qt::ItemFlags AlbumHistoryModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (index.column() == FavouriteColumn) {
        flags |= Qt::ItemIsUserCheckable;
    }

    return flags;
}
