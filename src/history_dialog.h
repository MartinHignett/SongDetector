#ifndef HISTORY_H
#define HISTORY_H

#include <QDialog>

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
    explicit HistoryDialog(QWidget *parent = nullptr, SongDatabase *songDatabase = nullptr);
    ~HistoryDialog();

private:
    Ui::HistoryDialog     *ui;
    SongDatabase        *m_songDatabase;
    SongHistoryModel    m_songsModel;
    AlbumHistoryModel   m_albumsModel;

    void            refresh();
};

#endif // HISTORY_H
