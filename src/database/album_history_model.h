#pragma once

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

/*
 * Read/write table model for the Albums history tab. See
 * SongHistoryModel for why this isn't a QSqlQueryModel.
 */
class AlbumHistoryModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        NameColumn,
        LabelColumn,
        ReleaseDateColumn,
        FavouriteColumn,
        ColumnCount,
    };

    explicit AlbumHistoryModel(QObject* parent = nullptr);

    void load(const QSqlDatabase& database);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
    struct Row {
        QString albumUuid;
        QString name;
        QString label;
        QString releaseDate;
        bool    favourite = false;
    };

    QSqlDatabase    m_database;
    QVector<Row>    m_rows;
};
