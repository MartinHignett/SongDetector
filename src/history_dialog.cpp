#include "history_dialog.h"
#include "ui_history_dialog.h"

HistoryDialog::HistoryDialog(QWidget *parent, SongDatabase *songDatabase) :
    QDialog(parent),
    ui(new Ui::HistoryDialog),
    m_songDatabase(songDatabase)
{
    ui->setupUi(this);

    ui->tableView->setModel(&m_songsModel);
    ui->albumsTable->setModel(&m_albumsModel);

    refresh();
}

HistoryDialog::~HistoryDialog()
{
    delete ui;
}

void HistoryDialog::refresh()
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
