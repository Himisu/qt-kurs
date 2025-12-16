#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QListWidgetItem>

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
    void on_moveToInProgress_clicked();
    void on_moveToReady_clicked();
    void on_moveToCompleted_clicked();
    void on_newRequests_itemDoubleClicked(QListWidgetItem *item);
    void on_inProgress_itemDoubleClicked(QListWidgetItem *item);
    void on_ready_itemDoubleClicked(QListWidgetItem *item);
    void refreshAllLists();

private:
    bool connectToDatabase();
    void loadNewRequests();
    void loadInProgress();
    void loadReady();
    void updateRequestStatus(int id, const QString &status);
    void showRequestDetails(int id);

    Ui::MainWindow *ui;
    QSqlDatabase db;
};

#endif // MAINWINDOW_H
