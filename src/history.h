#ifndef HISTORY_H
#define HISTORY_H

#include <QDialog>

#include "database/album_history_model.h"
#include "database/song_database.h"
#include "database/song_history_model.h"

namespace Ui {
class History;
}

class History : public QDialog
{
    Q_OBJECT

public:
    explicit History(QWidget *parent = nullptr, SongDatabase *songDatabase = nullptr);
    ~History();

private:
    Ui::History         *ui;
    SongDatabase        *m_songDatabase;
    SongHistoryModel    m_songsModel;
    AlbumHistoryModel   m_albumsModel;

    void            refresh();
};

#endif // HISTORY_H
