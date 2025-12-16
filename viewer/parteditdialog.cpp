#include "parteditdialog.h"
#include "ui_parteditdialog.h"
#include <QSqlQuery>
#include <QDebug>
#include <QMessageBox>

PartEditDialog::PartEditDialog(QWidget *parent, int partId) :
    QDialog(parent),
    ui(new Ui::PartEditDialog),
    m_partId(partId)
{
    ui->setupUi(this);

    setStyleSheet(QString::fromUtf8(R"QSS(
/* Minimal modern-ish theme for Qt Widgets */
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


    if (m_partId != -1) {
        setWindowTitle("Редактирование запчасти");
        loadPartData();
    } else {
        setWindowTitle("Добавление запчасти");
    }
}

PartEditDialog::~PartEditDialog()
{
    delete ui;
}

void PartEditDialog::loadPartData()
{
    QSqlQuery query;
    query.prepare("SELECT part_name, part_number, category, quantity, price, min_quantity "
                  "FROM repair_parts WHERE id = ?");
    query.addBindValue(m_partId);

    if (query.exec() && query.next()) {
        ui->editPartName->setText(query.value(0).toString());
        ui->editPartNumber->setText(query.value(1).toString());

        int index = ui->comboCategory->findText(query.value(2).toString());
        if (index != -1) {
            ui->comboCategory->setCurrentIndex(index);
        }

        ui->spinQuantity->setValue(query.value(3).toInt());
        ui->spinPrice->setValue(query.value(4).toDouble());
        ui->spinMinQuantity->setValue(query.value(5).toInt());
    }
}

QString PartEditDialog::partName() const
{
    return ui->editPartName->text();
}

QString PartEditDialog::partNumber() const
{
    return ui->editPartNumber->text();
}

QString PartEditDialog::category() const
{
    return ui->comboCategory->currentText();
}

int PartEditDialog::quantity() const
{
    return ui->spinQuantity->value();
}

double PartEditDialog::price() const
{
    return ui->spinPrice->value();
}

int PartEditDialog::minQuantity() const
{
    return ui->spinMinQuantity->value();
}

void PartEditDialog::on_btnSave_clicked()
{
    if (ui->editPartName->text().isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Введите название запчасти!");
        return;
    }

    accept();
}

void PartEditDialog::on_btnCancel_clicked()
{
    reject();
}
