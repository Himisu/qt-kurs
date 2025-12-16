#ifndef PARTEDITDIALOG_H
#define PARTEDITDIALOG_H

#include <QDialog>

namespace Ui {
class PartEditDialog;
}

class PartEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PartEditDialog(QWidget *parent = nullptr, int partId = -1);
    ~PartEditDialog();

    QString partName() const;
    QString partNumber() const;
    QString category() const;
    int quantity() const;
    double price() const;
    int minQuantity() const;

private slots:
    void on_btnSave_clicked();
    void on_btnCancel_clicked();

private:
    Ui::PartEditDialog *ui;
    int m_partId;
    void loadPartData();
};

#endif // PARTEDITDIALOG_H
