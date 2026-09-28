#include "history.h"
#include "ui_history.h"

History::History(QWidget *parent, SongDatabase *songDatabase) :
    QDialog(parent),
    ui(new Ui::History),
    m_songDatabase(songDatabase)
{
    ui->setupUi(this);

    ui->tableView->setModel(&m_songsModel);
    ui->albumsTable->setModel(&m_albumsModel);

    refresh();
}

History::~History()
{
    delete ui;
}

void History::refresh()
{
    if (m_songDatabase == nullptr) {
        return;
    }

    const QSqlDatabase db = m_songDatabase->database();

    m_songsModel.load(db);
    m_albumsModel.load(db);

    ui->tableView->resizeColumnsToContents();
    ui->albumsTable->resizeColumnsToContents();
}
