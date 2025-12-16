#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QDebug>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QListWidgetItem>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Ремонтная мастерская");
    setStyleSheet(QString::fromUtf8(R"QSS(
/* Minimal modern-ish theme for Qt Widgets */
QMainWindow { background: #0f1115; }
QWidget { color: #e6e6e6; font-size: 12px; }
QLabel { color: #cfd3dc; }

QLabel#label_1, QLabel#label_2, QLabel#label_3 { color: #ffffff; }

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

    if (!connectToDatabase()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось подключиться к базе данных!");
        return;
    }
    refreshAllLists();
}

MainWindow::~MainWindow()
{
    if (db.isOpen()) {
        db.close();
    }
    delete ui;
}

bool MainWindow::connectToDatabase()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("C:/Users/user/Desktop/1/repair_requests.db");

    if (!db.open()) {
        qDebug() << "Ошибка подключения:" << db.lastError().text();
        return false;
    }

    QSqlQuery checkData;
    checkData.exec("SELECT status FROM repair_requests LIMIT 1");
    if (checkData.next()) {
        QString existingStatus = checkData.value(0).toString();
        qDebug() << "Существующий статус в БД:" << existingStatus;
    }

    return true;
}

void MainWindow::refreshAllLists()
{
    loadNewRequests();
    loadInProgress();
    loadReady();
}

void MainWindow::loadNewRequests()
{
    ui->newRequests->clear();

    QSqlQuery query;
    query.prepare("SELECT id, full_name, device_type, problem_description, created_at "
                  "FROM repair_requests WHERE status = 'Не принят' OR status IS NULL OR status = '' "
                  "ORDER BY created_at DESC");

    if (query.exec()) {
        qDebug() << "Найдено новых заявок:" << query.size();
        while (query.next()) {
            int id = query.value(0).toInt();
            QString fullName = query.value(1).toString();
            QString deviceType = query.value(2).toString();
            QString createdAt = query.value(4).toDateTime().toString("dd.MM.yyyy HH:mm");

            QString itemText = QString("#%1 | %2\n%3 | %4")
                                  .arg(id)
                                  .arg(fullName)
                                  .arg(deviceType)
                                  .arg(createdAt);

            QListWidgetItem *item = new QListWidgetItem(itemText);
            item->setData(Qt::UserRole, id);
            ui->newRequests->addItem(item);
        }
    }
    else {
        qDebug() << "Ошибка загрузки новых заявок:" << query.lastError().text();
    }
}

void MainWindow::loadInProgress()
{
    ui->inProgress->clear();

    QSqlQuery query;
    query.prepare("SELECT id, full_name, device_type, problem_description, created_at "
                  "FROM repair_requests WHERE status = 'В работе' "
                  "ORDER BY created_at DESC");

    if (query.exec()) {
        qDebug() << "Найдено заявок в работе:" << query.size();
        while (query.next()) {
            int id = query.value(0).toInt();
            QString fullName = query.value(1).toString();
            QString deviceType = query.value(2).toString();
            QString createdAt = query.value(4).toDateTime().toString("dd.MM.yyyy HH:mm");

            QString itemText = QString("#%1 | %2\n%3 | %4")
                                  .arg(id)
                                  .arg(fullName)
                                  .arg(deviceType)
                                  .arg(createdAt);

            QListWidgetItem *item = new QListWidgetItem(itemText);
            item->setData(Qt::UserRole, id);
            ui->inProgress->addItem(item);
        }
    }
    else {
        qDebug() << "Ошибка загрузки заявок в работе:" << query.lastError().text();
    }
}

void MainWindow::loadReady()
{
    ui->ready->clear();

    QSqlQuery query;
    query.prepare("SELECT id, full_name, device_type, problem_description, created_at "
                  "FROM repair_requests WHERE status = 'Готов' "
                  "ORDER BY created_at DESC");

    if (query.exec()) {
        qDebug() << "Найдено готовых заявок:" << query.size();
        while (query.next()) {
            int id = query.value(0).toInt();
            QString fullName = query.value(1).toString();
            QString deviceType = query.value(2).toString();
            QString createdAt = query.value(4).toDateTime().toString("dd.MM.yyyy HH:mm");

            QString itemText = QString("#%1 | %2\n%3 | %4")
                                  .arg(id)
                                  .arg(fullName)
                                  .arg(deviceType)
                                  .arg(createdAt);

            QListWidgetItem *item = new QListWidgetItem(itemText);
            item->setData(Qt::UserRole, id);
            ui->ready->addItem(item);
        }
    }
    else {
        qDebug() << "Ошибка загрузки готовых заявок:" << query.lastError().text();
    }
}

void MainWindow::updateRequestStatus(int id, const QString &status)
{
    QSqlQuery query;
    query.prepare("UPDATE repair_requests SET status = :status WHERE id = :id");
    query.bindValue(":status", status);
    query.bindValue(":id", id);

    if (query.exec()) {
        refreshAllLists();
    }
    else {
        QMessageBox::critical(this, "Ошибка", "Не удалось обновить статус: " + query.lastError().text());
    }
}

void MainWindow::showRequestDetails(int id)
{
    QSqlQuery query;
    query.prepare("SELECT full_name, device_type, problem_description, created_at, status "
                  "FROM repair_requests WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec() && query.next()) {
        QString fullName = query.value(0).toString();
        QString deviceType = query.value(1).toString();
        QString problemDescription = query.value(2).toString();
        QString createdAt = query.value(3).toDateTime().toString("dd.MM.yyyy HH:mm");
        QString status = query.value(4).toString();

        QString statusText = status;

        QString details = QString(
            "ID: %1\n"
            "ФИО: %2\n"
            "Тип устройства: %3\n"
            "Статус: %4\n"
            "Создана: %5\n\n"
            "Описание проблемы:\n%6")
            .arg(id)
            .arg(fullName)
            .arg(deviceType)
            .arg(statusText)
            .arg(createdAt)
            .arg(problemDescription);

        QMessageBox::information(this, "Детали заявки", details);
    }
}

void MainWindow::on_moveToInProgress_clicked()
{
    QListWidgetItem *currentItem = ui->newRequests->currentItem();
    if (currentItem) {
        int id = currentItem->data(Qt::UserRole).toInt();
        updateRequestStatus(id, "В работе");
    }
    else {
        QMessageBox::warning(this, "Предупреждение", "Выберите заявку для перемещения в работу!");
    }
}

void MainWindow::on_moveToReady_clicked()
{
    QListWidgetItem *currentItem = ui->inProgress->currentItem();
    if (currentItem) {
        int id = currentItem->data(Qt::UserRole).toInt();
        updateRequestStatus(id, "Готов");
    }
    else {
        QMessageBox::warning(this, "Предупреждение", "Выберите заявку для перемещения в готовые!");
    }
}

void MainWindow::on_moveToCompleted_clicked()
{
    QListWidgetItem *currentItem = ui->ready->currentItem();
    if (currentItem) {
        int id = currentItem->data(Qt::UserRole).toInt();

        QSqlQuery query;
        query.prepare("DELETE FROM repair_requests WHERE id = :id");
        query.bindValue(":id", id);

        if (query.exec()) {
            refreshAllLists();
            QMessageBox::information(this, "Успех", "Заявка завершена и удалена из системы!");
        }
        else {
            QMessageBox::critical(this, "Ошибка", "Не удалось удалить заявку: " + query.lastError().text());
        }
    }
    else {
        QMessageBox::warning(this, "Предупреждение", "Выберите заявку для завершения!");
    }
}

void MainWindow::on_newRequests_itemDoubleClicked(QListWidgetItem *item)
{
    if (item) {
        int id = item->data(Qt::UserRole).toInt();
        showRequestDetails(id);
    }
}

void MainWindow::on_inProgress_itemDoubleClicked(QListWidgetItem *item)
{
    if (item) {
        int id = item->data(Qt::UserRole).toInt();
        showRequestDetails(id);
    }
}

void MainWindow::on_ready_itemDoubleClicked(QListWidgetItem *item)
{
    if (item) {
        int id = item->data(Qt::UserRole).toInt();
        showRequestDetails(id);
    }
}
