#include "partsdialog.h"
#include "ui_partsdialog.h"
#include "parteditdialog.h"
#include <QMessageBox>
#include <QDebug>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

PartsDialog::PartsDialog(QSqlDatabase db, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PartsDialog),
    m_db(db)
{
    ui->setupUi(this);
    setWindowTitle("Учёт запчастей");
    setStyleSheet(QString::fromUtf8(R"QSS(
QMainWindow { background: #0f1115; }
QWidget { color: #e6e6e6; font-size: 12px; }
QLabel { color: #cfd3dc; }
#titleLabel, #label_Title { color: #ffffff; }

QFrame#headerFrame,
QFrame#createCard, QFrame#searchCard, QFrame#tableCard {
  background: #151923;
  border: 1px solid #242b3d;
  border-radius: 12px;
}

QPushButton {
  background: #1f2635;
  border: 1px solid #2b3550;
  border-radius: 10px;
  padding: 8px 12px;
}
QPushButton:hover { background: #24304a; }
QPushButton:pressed { background: #1b2232; }

QLineEdit, QTextEdit, QComboBox {
  background: #0f1115;
  border: 1px solid #242b3d;
  border-radius: 10px;
  padding: 8px;
}
QTextEdit { padding: 10px; }
QComboBox::drop-down { border: 0px; }

QTabWidget::pane {
  border: 1px solid #242b3d;
  border-radius: 12px;
  top: -1px;
  background: #111521;
}
QTabBar::tab {
  background: #151923;
  border: 1px solid #242b3d;
  padding: 8px 12px;
  border-top-left-radius: 10px;
  border-top-right-radius: 10px;
  margin-right: 6px;
}
QTabBar::tab:selected { background: #111521; }

QListWidget {
  background: #0f1115;
  border: 1px solid #242b3d;
  border-radius: 12px;
  padding: 6px;
}
QListWidget::item { padding: 8px; border-radius: 8px; }
QListWidget::item:selected { background: #22304a; }

QTableWidget {
  background: #0f1115;
  border: 1px solid #242b3d;
  border-radius: 12px;
  gridline-color: #242b3d;
}
QHeaderView::section {
  background: #151923;
  border: 1px solid #242b3d;
  padding: 6px;
}
)QSS"));


    ui->tableParts->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableParts->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableParts->horizontalHeader()->setStretchLastSection(true);

    loadParts();
}

PartsDialog::~PartsDialog()
{
    delete ui;
}

void PartsDialog::loadParts()
{
    ui->tableParts->setRowCount(0);

    QSqlQuery query("SELECT id, part_name, part_number, category, quantity, price, min_quantity FROM repair_parts ORDER BY part_name");

    int row = 0;
    while (query.next()) {
        ui->tableParts->insertRow(row);

        for (int col = 0; col < 7; col++) {
            QTableWidgetItem *item = new QTableWidgetItem();

            if (col == 4 || col == 6) { // Количество и мин. запас
                item->setData(Qt::DisplayRole, query.value(col).toInt());
                item->setTextAlignment(Qt::AlignCenter);

                if (col == 4 && query.value(col).toInt() < query.value(6).toInt()) {
                    item->setBackground(Qt::red);
                    item->setForeground(Qt::white);
                }
            }
            else if (col == 5) { // Цена
                item->setData(Qt::DisplayRole, query.value(col).toDouble());
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
            else {
                item->setText(query.value(col).toString());
            }

            ui->tableParts->setItem(row, col, item);
        }

        row++;
    }

    ui->tableParts->setHorizontalHeaderLabels(
        QStringList() << "ID" << "Название" << "Артикул" << "Категория"
                      << "Количество" << "Цена" << "Мин. запас");
}

void PartsDialog::on_btnAddPart_clicked()
{
    PartEditDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QSqlQuery query;
        query.prepare("INSERT INTO repair_parts (part_name, part_number, category, quantity, price, min_quantity) "
                      "VALUES (?, ?, ?, ?, ?, ?)");
        query.addBindValue(dialog.partName());
        query.addBindValue(dialog.partNumber());
        query.addBindValue(dialog.category());
        query.addBindValue(dialog.quantity());
        query.addBindValue(dialog.price());
        query.addBindValue(dialog.minQuantity());

        if (query.exec()) {
            loadParts();
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось добавить запчасть: " + query.lastError().text());
        }
    }
}

void PartsDialog::on_btnEditPart_clicked()
{
    int row = ui->tableParts->currentRow();
    if (row >= 0) {
        int partId = ui->tableParts->item(row, 0)->text().toInt();
        PartEditDialog dialog(this, partId);
        if (dialog.exec() == QDialog::Accepted) {
            QSqlQuery query;
            query.prepare("UPDATE repair_parts SET part_name = ?, part_number = ?, category = ?, "
                          "quantity = ?, price = ?, min_quantity = ? WHERE id = ?");
            query.addBindValue(dialog.partName());
            query.addBindValue(dialog.partNumber());
            query.addBindValue(dialog.category());
            query.addBindValue(dialog.quantity());
            query.addBindValue(dialog.price());
            query.addBindValue(dialog.minQuantity());
            query.addBindValue(partId);

            if (query.exec()) {
                loadParts();
            } else {
                QMessageBox::critical(this, "Ошибка", "Не удалось обновить запчасть: " + query.lastError().text());
            }
        }
    } else {
        QMessageBox::warning(this, "Предупреждение", "Выберите запчасть для редактирования!");
    }
}

void PartsDialog::on_btnDeletePart_clicked()
{
    int row = ui->tableParts->currentRow();
    if (row >= 0) {
        int partId = ui->tableParts->item(row, 0)->text().toInt();
        QString partName = ui->tableParts->item(row, 1)->text();

        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Подтверждение",
            QString("Вы уверены, что хотите удалить запчасть \"%1\"?").arg(partName),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            QSqlQuery query;
            query.prepare("DELETE FROM repair_parts WHERE id = ?");
            query.addBindValue(partId);

            if (query.exec()) {
                loadParts();
            }
            else {
                QMessageBox::critical(this, "Ошибка", "Не удалось удалить запчасть: " + query.lastError().text());
            }
        }
    }
    else {
        QMessageBox::warning(this, "Предупреждение", "Выберите запчасть для удаления!");
    }
}

void PartsDialog::on_btnClose_clicked()
{
    accept();
}
