#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QTableWidgetItem>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_submitButton_clicked();
    void refreshRequestsTable();
    bool connectToDatabase();
    void on_searchButton_clicked();
    void on_clearSearchButton_clicked();
    void on_searchLineEdit_returnPressed();

private:
    void showAllRequests();
    void showSearchResult(int id);

    Ui::MainWindow *ui;
    QSqlDatabase db;
    bool isSearchMode;
    int currentSearchId;
};

#endif // MAINWINDOW_H
