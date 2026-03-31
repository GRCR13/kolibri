#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "fileworker.h"
#include <QLineEdit>
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void on_cb_timer_checkStateChanged(const Qt::CheckState &arg1);

private:
    Ui::MainWindow *ui;
    bool m_isProcessing = false;
    QTimer *m_timer = nullptr;


    void startWorker(const QString& fileIn,
                     const QString& dirOut,
                     uint64_t key,
                     const QVector<QString>& filter,
                     bool deleteInFile,
                     WriteMode mode);

    bool validKeys(const QVector<QLineEdit*>& edits);

};
#endif // MAINWINDOW_H
