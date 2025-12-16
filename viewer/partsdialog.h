#ifndef PARTSDIALOG_H
#define PARTSDIALOG_H

#include <QDialog>
#include <QSqlDatabase>

namespace Ui {
class PartsDialog;
}

class PartsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PartsDialog(QSqlDatabase db, QWidget *parent = nullptr);
    ~PartsDialog();

private slots:
    void on_btnAddPart_clicked();
    void on_btnEditPart_clicked();
    void on_btnDeletePart_clicked();
    void on_btnClose_clicked();
    void loadParts();

private:
    Ui::PartsDialog *ui;
    QSqlDatabase m_db;
};

#endif // PARTSDIALOG_H
