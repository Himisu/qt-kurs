#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    isSearchMode(false),
    currentSearchId(-1)
{
    ui->setupUi(this);

    setWindowTitle("Ремонтная мастерская");
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

    ui->deviceComboBox->addItem("Телефон");
    ui->deviceComboBox->addItem("Компьютер");
    ui->deviceComboBox->addItem("Ноутбук");
    ui->deviceComboBox->addItem("Планшет");
    ui->deviceComboBox->addItem("Другое");

    ui->requestsTable->setColumnCount(5);
    QStringList headers;
    headers << "ID" << "ФИО" << "Устройство" << "Описание проблемы" << "Статус";
    ui->requestsTable->setHorizontalHeaderLabels(headers);

    ui->requestsTable->setColumnWidth(0, 50);   // ID
    ui->requestsTable->setColumnWidth(1, 150);  // ФИО
    ui->requestsTable->setColumnWidth(2, 100);  // Устройство
    ui->requestsTable->setColumnWidth(3, 200);  // Описание
    ui->requestsTable->setColumnWidth(4, 120);  // Статус

    ui->requestsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->requestsTable->setSelectionMode(QAbstractItemView::SingleSelection);

    if (!connectToDatabase()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось подключиться к базе данных!");
    }
    else {
        refreshRequestsTable();
    }
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

    QSqlQuery pragmaQuery("PRAGMA table_info(repair_requests)");
    bool hasUpdatedAt = false;

    while (pragmaQuery.next()) {
        if (pragmaQuery.value(1).toString() == "updated_at") {
            hasUpdatedAt = true;
            break;
        }
    }

    if (!hasUpdatedAt) {
        QSqlQuery createColumn;
        if (!createColumn.exec("ALTER TABLE repair_requests ADD COLUMN updated_at TEXT")) {
            qDebug() << "Не удалось добавить поле updated_at:" << createColumn.lastError().text();
        }
    }

    QSqlQuery checkData;
    checkData.exec("SELECT status FROM repair_requests LIMIT 1");
    if (checkData.next()) {
        QString existingStatus = checkData.value(0).toString();
        qDebug() << "Существующий статус в БД:" << existingStatus;
    }

    return true;
}

void MainWindow::refreshRequestsTable()
{
    if (isSearchMode && currentSearchId > 0) {
        showSearchResult(currentSearchId);
        return;
    }
    showAllRequests();
}

void MainWindow::showAllRequests()
{
    isSearchMode = false;
    currentSearchId = -1;
    ui->searchLineEdit->clear();
    ui->requestsTable->setRowCount(0);

    QSqlQuery query;
    query.prepare("SELECT id, full_name, device_type, problem_description, status "
                  "FROM repair_requests ORDER BY created_at DESC");

    if (query.exec()) {
        int row = 0;
        while (query.next()) {
            ui->requestsTable->insertRow(row);

            // ID
            QTableWidgetItem *idItem = new QTableWidgetItem(query.value(0).toString());
            idItem->setFlags(idItem->flags() ^ Qt::ItemIsEditable);
            ui->requestsTable->setItem(row, 0, idItem);

            // ФИО
            QTableWidgetItem *nameItem = new QTableWidgetItem(query.value(1).toString());
            nameItem->setFlags(nameItem->flags() ^ Qt::ItemIsEditable);
            ui->requestsTable->setItem(row, 1, nameItem);

            // Устройство
            QTableWidgetItem *deviceItem = new QTableWidgetItem(query.value(2).toString());
            deviceItem->setFlags(deviceItem->flags() ^ Qt::ItemIsEditable);
            ui->requestsTable->setItem(row, 2, deviceItem);

            // Описание проблемы
            QString problem = query.value(3).toString();
            if (problem.length() > 50) {
                problem = problem.left(50) + "...";
            }
            QTableWidgetItem *problemItem = new QTableWidgetItem(problem);
            problemItem->setFlags(problemItem->flags() ^ Qt::ItemIsEditable);
            problemItem->setToolTip(query.value(3).toString());
            ui->requestsTable->setItem(row, 3, problemItem);

            // Статус
            QString status = query.value(4).toString();
            QTableWidgetItem *statusItem = new QTableWidgetItem(status);
            statusItem->setFlags(statusItem->flags() ^ Qt::ItemIsEditable);

            // Цвет статуса
            if (status == "Не принят") {statusItem->setBackground(Qt::yellow);}
            else if (status == "В работе") {statusItem->setBackground(Qt::blue);statusItem->setForeground(Qt::white);}
            else if (status == "Готов") {statusItem->setBackground(Qt::green);}
            else if (status == "Завершен") {statusItem->setBackground(Qt::gray);}
            ui->requestsTable->setItem(row, 4, statusItem);
            row++;
        }
        ui->requestsTable->resizeRowsToContents();
    }
    else {qDebug() << "Ошибка загрузки заявок:" << query.lastError().text();}
}

void MainWindow::showSearchResult(int id)
{
    isSearchMode = true;
    currentSearchId = id;
    ui->requestsTable->setRowCount(0);

    QSqlQuery query;
    query.prepare("SELECT id, full_name, device_type, problem_description, status "
                  "FROM repair_requests WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec() && query.next()) {
        ui->requestsTable->insertRow(0);

        // ID
        QTableWidgetItem *idItem = new QTableWidgetItem(query.value(0).toString());
        idItem->setFlags(idItem->flags() ^ Qt::ItemIsEditable);
        idItem->setBackground(Qt::lightGray); // Подсветка найденной заявки
        ui->requestsTable->setItem(0, 0, idItem);

        // ФИО
        QTableWidgetItem *nameItem = new QTableWidgetItem(query.value(1).toString());
        nameItem->setFlags(nameItem->flags() ^ Qt::ItemIsEditable);
        nameItem->setBackground(Qt::lightGray);
        ui->requestsTable->setItem(0, 1, nameItem);

        // Устройство
        QTableWidgetItem *deviceItem = new QTableWidgetItem(query.value(2).toString());
        deviceItem->setFlags(deviceItem->flags() ^ Qt::ItemIsEditable);
        deviceItem->setBackground(Qt::lightGray);
        ui->requestsTable->setItem(0, 2, deviceItem);

        // Описание проблемы
        QString problem = query.value(3).toString();
        QTableWidgetItem *problemItem = new QTableWidgetItem(problem);
        problemItem->setFlags(problemItem->flags() ^ Qt::ItemIsEditable);
        problemItem->setToolTip(problem);
        problemItem->setBackground(Qt::lightGray);
        ui->requestsTable->setItem(0, 3, problemItem);

        // Статус
        QString status = query.value(4).toString();
        QTableWidgetItem *statusItem = new QTableWidgetItem(status);
        statusItem->setFlags(statusItem->flags() ^ Qt::ItemIsEditable);

        // Цвет статуса
        if (status == "Не принят") {statusItem->setBackground(QColor(255, 255, 200));}
        else if (status == "В работе") {statusItem->setBackground(QColor(100, 100, 255));statusItem->setForeground(Qt::white);}
        else if (status == "Готов") {statusItem->setBackground(QColor(200, 255, 200));}
        else if (status == "Завершен") {statusItem->setBackground(QColor(220, 220, 220));}
        else {statusItem->setBackground(Qt::lightGray);}

        ui->requestsTable->setItem(0, 4, statusItem);
        ui->requestsTable->resizeRowsToContents();

    }
    else {
        QMessageBox::information(this, "Поиск",
            QString("Заявка с ID %1 не найдена.").arg(id));
        showAllRequests();
    }
}

void MainWindow::on_searchButton_clicked()
{
    QString searchText = ui->searchLineEdit->text().trimmed();

    if (searchText.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Введите ID для поиска!");
        return;
    }
    bool ok;
    int id = searchText.toInt(&ok);

    if (!ok || id <= 0) {
        QMessageBox::warning(this, "Предупреждение", "Введите корректный ID (положительное число)!");
        ui->searchLineEdit->clear();
        return;
    }
    showSearchResult(id);
}

void MainWindow::on_clearSearchButton_clicked()
{
    showAllRequests();
}

void MainWindow::on_searchLineEdit_returnPressed()
{
    on_searchButton_clicked();
}

void MainWindow::on_submitButton_clicked()
{
    QString fullName = ui->fullNameLineEdit->text().trimmed();
    QString deviceType = ui->deviceComboBox->currentText();
    QString problemDescription = ui->problemTextEdit->toPlainText().trimmed();

    if (fullName.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Пожалуйста, введите ФИО!");
        return;
    }

    if (problemDescription.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Пожалуйста, опишите проблему!");
        return;
    }

    QSqlQuery query;
    query.prepare("INSERT INTO repair_requests (full_name, device_type, problem_description, status) "
                  "VALUES (:full_name, :device_type, :problem_description, 'Не принят')");
    query.bindValue(":full_name", fullName);
    query.bindValue(":device_type", deviceType);
    query.bindValue(":problem_description", problemDescription);

    if (query.exec()) {
        QMessageBox::information(this, "Успех", "Заявка успешно отправлена!");

        int newId = query.lastInsertId().toInt();
        ui->fullNameLineEdit->clear();
        ui->deviceComboBox->setCurrentIndex(0);
        ui->problemTextEdit->clear();

        ui->searchLineEdit->setText(QString::number(newId));
        showSearchResult(newId);

    }
    else {QMessageBox::critical(this, "Ошибка","Не удалось отправить заявку: " + query.lastError().text());}
}
