#pragma once

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

/*
 * Read/write table model for the Songs history tab. One row per
 * detection event (joined through the history table), newest first.
 * Rows are fetched once via a hand-rolled join (rather than
 * QSqlQueryModel) so that the Favourite column can be toggled as a
 * checkbox and written back to the songs table, and so headers are
 * always shown, even with zero rows.
 */
class SongHistoryModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        IdentifiedOnColumn,
        ArtistColumn,
        TitleColumn,
        AlbumColumn,
        TrackColumn,
        FavouriteColumn,
        ColumnCount,
    };

    explicit SongHistoryModel(QObject* parent = nullptr);

    void load(const QSqlDatabase& database);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

private:
    struct Row {
        QString songUuid;
        QString identifiedOn;
        QString artist;
        QString title;
        QString album;
        int     track = 0;
        bool    favourite = false;
        int     sortId = 0;     // scratch: pre-sort position, for persistent indexes
    };

    void sortRows();

    QSqlDatabase    m_database;
    QVector<Row>    m_rows;
    int             m_sortColumn = IdentifiedOnColumn;
    Qt::SortOrder   m_sortOrder = Qt::DescendingOrder;
};
