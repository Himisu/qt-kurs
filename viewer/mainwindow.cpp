#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDesktopServices>
#include "parteditdialog.h"
#include "partsdialog.h"
#include <QComboBox>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>

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

    QSqlQuery createPartsTable;
    createPartsTable.exec(
        "CREATE TABLE IF NOT EXISTS repair_parts ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "part_name TEXT NOT NULL,"
        "part_number TEXT,"
        "category TEXT,"
        "quantity INTEGER NOT NULL DEFAULT 0,"
        "price REAL,"
        "min_quantity INTEGER DEFAULT 5,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)"
    );

    QSqlQuery createUsageTable;
    createUsageTable.exec(
        "CREATE TABLE IF NOT EXISTS parts_usage ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "repair_id INTEGER NOT NULL,"
        "part_id INTEGER NOT NULL,"
        "quantity_used INTEGER NOT NULL DEFAULT 1,"
        "used_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (repair_id) REFERENCES repair_requests(id) ON DELETE CASCADE,"
        "FOREIGN KEY (part_id) REFERENCES repair_parts(id) ON DELETE CASCADE)"
    );

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
                  "FROM repair_requests WHERE status IS NULL OR status = '' OR status = 'Не принят' "
                  "ORDER BY created_at DESC");

    if (query.exec()) {
        int count = 0;
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
            count++;
        }
        qDebug() << "Найдено новых заявок:" << count;
    } else {
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
        int count = 0;
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
            count++;
        }
        qDebug() << "Найдено заявок в работе:" << count;
    } else {
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
        int count = 0;
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
            count++;
        }
        qDebug() << "Найдено готовых заявок:" << count;
    } else {
        qDebug() << "Ошибка загрузки готовых заявок:" << query.lastError().text();
    }
}

void MainWindow::updateRequestStatus(int id, const QString &status)
{
    QSqlQuery query;
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    query.prepare("UPDATE repair_requests SET status = :status, updated_at = :updated_at WHERE id = :id");
    query.bindValue(":status", status);
    query.bindValue(":updated_at", currentTime);
    query.bindValue(":id", id);

    if (query.exec()) {
        refreshAllLists();
    } else {
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
            .arg(status)
            .arg(createdAt)
            .arg(problemDescription);

        QMessageBox msgBox;
        msgBox.setWindowTitle("Детали заявки");
        msgBox.setText(details);

        QPushButton *viewPartsButton = msgBox.addButton("Просмотр запчастей", QMessageBox::ActionRole);
        QPushButton *usePartsButton = msgBox.addButton("Списать запчасти", QMessageBox::ActionRole);
        QPushButton *closeButton = msgBox.addButton("Закрыть", QMessageBox::AcceptRole);

        msgBox.exec();

        if (msgBox.clickedButton() == viewPartsButton) {
            showUsedParts(id);
        } else if (msgBox.clickedButton() == usePartsButton) {
            showUsePartsDialog(id);
        }
    }
}

void MainWindow::on_moveToInProgress_clicked()
{
    QListWidgetItem *currentItem = ui->newRequests->currentItem();
    if (currentItem) {
        int id = currentItem->data(Qt::UserRole).toInt();
        updateRequestStatus(id, "В работе");
    } else {
        QMessageBox::warning(this, "Предупреждение", "Выберите заявку для перемещения в работу!");
    }
}

void MainWindow::on_moveToReady_clicked()
{
    QListWidgetItem *currentItem = ui->inProgress->currentItem();
    if (currentItem) {
        int id = currentItem->data(Qt::UserRole).toInt();
        updateRequestStatus(id, "Готов");
    } else {
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
        } else {
            QMessageBox::critical(this, "Ошибка", "Не удалось удалить заявку: " + query.lastError().text());
        }
    } else {
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

void MainWindow::on_exportToCsv_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this,
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/history_export.csv", "CSV файлы (*.csv);;Все файлы (*.*)");

    if (!filename.isEmpty()) {
        exportToCsv(filename);
    }
}

QString MainWindow::generateCsvContent()
{
    QString csvContent;

    csvContent += "ID;ФИО;Тип устройства;Описание проблемы;Статус;Дата создания;Дата обновления\n";

    QSqlQuery query;
    query.prepare("SELECT id, full_name, device_type, problem_description, status, "
                  "created_at, updated_at FROM repair_requests ORDER BY created_at DESC");

    if (!query.exec()) {
        qDebug() << "Ошибка выполнения запроса для экспорта:" << query.lastError().text();
        return "";
    }

    int recordCount = 0;
    while (query.next()) {
        int id = query.value(0).toInt();
        QString fullName = query.value(1).toString();
        QString deviceType = query.value(2).toString();
        QString problemDescription = query.value(3).toString();
        QString status = query.value(4).toString();

        QString createdAt = query.value(5).isNull() ? "Не указана" : query.value(5).toDateTime().toString("dd.MM.yyyy HH:mm");
        QString updatedAt = query.value(6).isNull() ? "Не указана" : query.value(6).toDateTime().toString("dd.MM.yyyy HH:mm");

        fullName = fullName.replace(';', ',').replace('\n', ' ').replace('\r', ' ');
        deviceType = deviceType.replace(';', ',').replace('\n', ' ').replace('\r', ' ');
        problemDescription = problemDescription.replace(';', ',').replace('\n', ' ').replace('\r', ' ');
        status = status.replace(';', ',').replace('\n', ' ').replace('\r', ' ');

        csvContent += QString("%1;\"%2\";\"%3\";\"%4\";\"%5\";%6;%7\n")
            .arg(id)
            .arg(fullName)
            .arg(deviceType)
            .arg(problemDescription)
            .arg(status)
            .arg(createdAt)
            .arg(updatedAt);

        recordCount++;
    }

    qDebug() << "Экспортировано записей:" << recordCount;
    return csvContent;
}

void MainWindow::exportToCsv(const QString &filename)
{
    QSqlQuery countQuery("SELECT COUNT(*) FROM repair_requests");
    int totalRecords = 0;

    if (countQuery.exec() && countQuery.next()) {
        totalRecords = countQuery.value(0).toInt();
        qDebug() << "Всего записей в базе:" << totalRecords;
    }

    if (totalRecords == 0) {
        QMessageBox::warning(this, "Предупреждение", "В базе данных нет записей для экспорта.");
        return;
    }

    QString csvContent = generateCsvContent();

    if (csvContent.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Не удалось сгенерировать данные для экспорта.");
        return;
    }

    QFile file(filename);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");

        stream << QChar(0xFEFF);

        stream << csvContent;
        file.close();

        QMessageBox::information(this, "Успех",
            QString("Данные успешно экспортированы в файл:\n%1\n\n"
                    "Количество экспортированных записей: %2")
                .arg(filename)
                .arg(totalRecords));

        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(filename).absolutePath()));
    } else {
        QMessageBox::critical(this, "Ошибка",
            QString("Не удалось создать файл:\n%1\n\nПричина: %2")
                .arg(filename)
                .arg(file.errorString()));
    }
}

void MainWindow::on_btnManageParts_clicked()
{
    showPartsDialog();
}

void MainWindow::showPartsDialog()
{
    PartsDialog dialog(db, this);
    dialog.exec();
}

void MainWindow::showUsedParts(int repairId)
{
    QSqlQuery query;
    query.prepare("SELECT p.part_name, u.quantity_used, p.price "
                  "FROM parts_usage u "
                  "JOIN repair_parts p ON u.part_id = p.id "
                  "WHERE u.repair_id = ?");
    query.addBindValue(repairId);

    if (query.exec()) {
        QStringList partsList;
        double total = 0;

        while (query.next()) {
            QString partName = query.value(0).toString();
            int quantity = query.value(1).toInt();
            double price = query.value(2).toDouble();
            double sum = quantity * price;
            total += sum;

            partsList << QString("%1 x %2 шт. = %3 руб.")
                         .arg(partName)
                         .arg(quantity)
                         .arg(sum, 0, 'f', 2);
        }

        if (partsList.isEmpty()) {
            QMessageBox::information(this, "Запчасти",
                QString("Для заявки #%1 не использованы запчасти.").arg(repairId));
        } else {
            QString message = QString("Использованные запчасти для заявки #%1:\n\n%2\n\nИтого: %3 руб.")
                                  .arg(repairId)
                                  .arg(partsList.join("\n"))
                                  .arg(total, 0, 'f', 2);
            QMessageBox::information(this, "Запчасти", message);
        }
    }
}

void MainWindow::showUsePartsDialog(int repairId)
{
    QDialog dialog(this);
    dialog.setWindowTitle("Списание запчастей - Заявка #" + QString::number(repairId));
    dialog.setMinimumSize(500, 400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QComboBox *comboParts = new QComboBox();
    comboParts->addItem("Выберите запчасть", -1);

    QSqlQuery query("SELECT id, part_name, quantity, price FROM repair_parts WHERE quantity > 0 ORDER BY part_name");
    while (query.next()) {
        int id = query.value(0).toInt();
        QString name = query.value(1).toString();
        int quantity = query.value(2).toInt();
        double price = query.value(3).toDouble();

        comboParts->addItem(QString("%1 (в наличии: %2 шт., %3 руб.)").arg(name).arg(quantity).arg(price, 0, 'f', 2), id);
    }

    layout->addWidget(new QLabel("Запчасть:"));
    layout->addWidget(comboParts);

    QSpinBox *spinQuantity = new QSpinBox();
    spinQuantity->setMinimum(1);
    spinQuantity->setMaximum(100);
    layout->addWidget(new QLabel("Количество:"));
    layout->addWidget(spinQuantity);

    QPushButton *btnAdd = new QPushButton("Добавить");
    layout->addWidget(btnAdd);

    QTableWidget *table = new QTableWidget(0, 3);
    table->setHorizontalHeaderLabels(QStringList() << "Запчасть" << "Количество" << "Сумма");
    layout->addWidget(table);

    QLabel *labelTotal = new QLabel("Итого: 0 руб.");
    layout->addWidget(labelTotal);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *btnSave = new QPushButton("Сохранить");
    QPushButton *btnCancel = new QPushButton("Отмена");
    buttonLayout->addWidget(btnSave);
    buttonLayout->addWidget(btnCancel);
    layout->addLayout(buttonLayout);

    struct UsedPart {
        int partId;
        QString name;
        int quantity;
        double price;
    };
    QList<UsedPart> usedParts;

    QObject::connect(btnAdd, &QPushButton::clicked, [&]() {
        int partId = comboParts->currentData().toInt();
        if (partId == -1) {
            QMessageBox::warning(&dialog, "Предупреждение", "Выберите запчасть!");
            return;
        }

        QSqlQuery checkQuery;
        checkQuery.prepare("SELECT quantity, price FROM repair_parts WHERE id = ?");
        checkQuery.addBindValue(partId);
        if (checkQuery.exec() && checkQuery.next()) {
            int available = checkQuery.value(0).toInt();
            double price = checkQuery.value(1).toDouble();
            int requested = spinQuantity->value();

            if (requested > available) {
                QMessageBox::warning(&dialog, "Предупреждение",
                    QString("Недостаточно на складе! Доступно: %1 шт.").arg(available));
                return;
            }

            for (int i = 0; i < usedParts.size(); i++) {
                if (usedParts[i].partId == partId) {
                    usedParts[i].quantity += requested;
                    break;
                }
            }

            if (std::none_of(usedParts.begin(), usedParts.end(),
                [partId](const UsedPart& p) { return p.partId == partId; })) {
                usedParts.append({partId, comboParts->currentText().split(" (").first(), requested, price});
            }

            table->setRowCount(usedParts.size());
            double total = 0;
            for (int i = 0; i < usedParts.size(); i++) {
                const UsedPart& part = usedParts[i];
                double sum = part.quantity * part.price;
                total += sum;

                table->setItem(i, 0, new QTableWidgetItem(part.name));
                table->setItem(i, 1, new QTableWidgetItem(QString::number(part.quantity)));
                table->setItem(i, 2, new QTableWidgetItem(QString::number(sum, 'f', 2) + " руб."));
            }

            labelTotal->setText(QString("Итого: %1 руб.").arg(total, 0, 'f', 2));
        }
    });

    QObject::connect(btnSave, &QPushButton::clicked, [&]() {
        if (usedParts.isEmpty()) {
            QMessageBox::warning(&dialog, "Предупреждение", "Не выбраны запчасти для списания!");
            return;
        }

        QSqlDatabase::database().transaction();

        try {
            for (const UsedPart& part : usedParts) {
                QSqlQuery usageQuery;
                usageQuery.prepare("INSERT INTO parts_usage (repair_id, part_id, quantity_used) VALUES (?, ?, ?)");
                usageQuery.addBindValue(repairId);
                usageQuery.addBindValue(part.partId);
                usageQuery.addBindValue(part.quantity);

                if (!usageQuery.exec()) {
                    throw usageQuery.lastError();
                }

                QSqlQuery updateQuery;
                updateQuery.prepare("UPDATE repair_parts SET quantity = quantity - ? WHERE id = ?");
                updateQuery.addBindValue(part.quantity);
                updateQuery.addBindValue(part.partId);

                if (!updateQuery.exec()) {
                    throw updateQuery.lastError();
                }
            }

            QSqlDatabase::database().commit();
            QMessageBox::information(&dialog, "Успех", "Запчасти успешно списаны!");
            dialog.accept();

        } catch (const QSqlError& error) {
            QSqlDatabase::database().rollback();
            QMessageBox::critical(&dialog, "Ошибка", "Ошибка при списании запчастей: " + error.text());
        }
    });

    QObject::connect(btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);

    dialog.exec();
}

void MainWindow::updatePartsQuantity(int partId, int quantity)
{
    QSqlQuery query;
    query.prepare("UPDATE repair_parts SET quantity = quantity - ? WHERE id = ?");
    query.addBindValue(quantity);
    query.addBindValue(partId);

    if (!query.exec()) {
        qDebug() << "Ошибка обновления количества запчастей:" << query.lastError().text();
    }
}
