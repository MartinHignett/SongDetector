#include "history_dialog.h"
#include "ui_history_dialog.h"

#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QStyleOptionButton>

namespace {

// Paints the "..." cell as a push button. Clicks are handled by the dialog.
class ActionButtonDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QStyleOptionButton button;
        button.rect = option.rect.adjusted(2, 2, -2, -2);
        button.text = index.data().toString();
        button.state = QStyle::State_Enabled | QStyle::State_Raised;
        QApplication::style()->drawControl(QStyle::CE_PushButton, &button, painter);
    }
};

}

HistoryDialog::HistoryDialog(QWidget *parent, SongDatabase *songDatabase) :
    QDialog(parent),
    ui(new Ui::HistoryDialog),
    m_songDatabase(songDatabase)
{
    ui->setupUi(this);

    ui->songsTable->setModel(&m_songsModel);
    ui->songsTable->setSortingEnabled(true);
    ui->songsTable->sortByColumn(SongHistoryModel::IdentifiedOnColumn, Qt::DescendingOrder);
    ui->songsTable->setItemDelegateForColumn(SongHistoryModel::ActionsColumn,
                                             new ActionButtonDelegate(ui->songsTable));
    connect(ui->songsTable, &QTableView::clicked, this, &HistoryDialog::showSongActions);
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

    ui->songsTable->resizeColumnsToContents();
    ui->albumsTable->resizeColumnsToContents();
}

void HistoryDialog::showSongActions(const QModelIndex& index)
{
    if (index.column() != SongHistoryModel::ActionsColumn) {
        return;
    }

    QMenu menu(this);
    QAction* deleteAction = menu.addAction(tr("Delete..."));

    const QRect cellRect = ui->songsTable->visualRect(index);
    const QPoint position = ui->songsTable->viewport()->mapToGlobal(cellRect.bottomLeft());

    if (menu.exec(position) != deleteAction) {
        return;
    }

    const QString artist = index.siblingAtColumn(SongHistoryModel::ArtistColumn).data().toString();
    const QString title = index.siblingAtColumn(SongHistoryModel::TitleColumn).data().toString();

    const auto answer = QMessageBox::question(
        this, tr("Delete Song"),
        tr("Delete \"%1\" by %2 and its detection history?").arg(title, artist));

    if (answer != QMessageBox::Yes) {
        return;
    }

    if (!m_songsModel.deleteSong(index.row())) {
        QMessageBox::warning(this, tr("Delete Song"), tr("The song could not be deleted."));
        return;
    }

    // Albums tab counts may have changed.
    m_albumsModel.load(m_songDatabase->database());
    ui->albumsTable->resizeColumnsToContents();
}
