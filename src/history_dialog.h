#ifndef HISTORY_H
#define HISTORY_H

#include <QDialog>
#include <QSettings>

#include "database/album_history_model.h"
#include "database/song_database.h"
#include "database/song_history_model.h"

namespace Ui {
class HistoryDialog;
}

class HistoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HistoryDialog(QWidget *parent = nullptr, SongDatabase *songDatabase = nullptr, QSettings *settings = nullptr);
    ~HistoryDialog();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    Ui::HistoryDialog   *ui;
    SongDatabase        *m_songDatabase;
    QSettings           *m_settings;
    SongHistoryModel    m_songsModel;
    AlbumHistoryModel   m_albumsModel;

    void            refresh();
    void            fillSongsTableWidth();
    void            showSongActions(const QModelIndex& index);
};

#endif // HISTORY_H
