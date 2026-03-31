#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "fileworker.h"
#include <QThread>
#include <QMessageBox>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_timer = new QTimer(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::validKeys(const QVector<QLineEdit*>& edits)
{
    auto isValidHexOrNum = [](QChar c) {
        c = c.toUpper();
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
    };

    for (auto* le : edits)
    {
        QString txt = le->text();
        if (txt.length() != 2) return false;
        for (QChar c : txt) {
            if (!isValidHexOrNum(c)) return false;
        }
    }
    return true;
}

void MainWindow::startWorker(const QString& fileIn,
                             const QString& dirOut,
                             uint64_t key,
                             const QVector<QString>& filter,
                             bool deleteInFile,
                             WriteMode mode)
{
    m_isProcessing = true;
    Worker* worker = new Worker;
    QThread* thread = new QThread;
    worker->moveToThread(thread);


    connect(worker, &Worker::changeProcProgressBar, ui->progressBar, &QProgressBar::setValue);
    connect(worker, &Worker::changeProcInfo, ui->lb_status, &QLabel::setText);
    connect(thread, &QThread::started, worker, [=]() {
        worker->processDir(fileIn, dirOut, key, filter, deleteInFile, mode);
        thread->quit();
    });

    auto finalize = [this]() {
        m_isProcessing = false;

        if (!m_timer->isActive()) {
            ui->pushButton->setEnabled(true);
            ui->lb_status->setText("");
            ui->progressBar->setValue(0);
        }
    };

    connect(worker, &Worker::sendError, this, [this, finalize](const QString& title, const QString& message) {
        QMessageBox::critical(this, title, message);
        finalize();
    });

    connect(worker, &Worker::finished, this, [this, finalize]() {

        if (!m_timer->isActive()) {
            QMessageBox::information(this, "Успех", "Обработка завершена");
        }
        finalize();
    });

    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void MainWindow::on_pushButton_clicked()
{

    if (m_timer->isActive()) {
        m_timer->stop();
        ui->pushButton->setText("Запустить");
        return;
    }


    QString fileIn = ui->le_fin->text().trimmed();
    QString dirOut = ui->le_fout->text().trimmed();
    bool deleteInFile = ui->cb_delete_file->isChecked();
    bool isEnableTime = ui->cb_timer->isChecked();
    int modeRewriteIndex = ui->cb_rewrite_mode->currentIndex();
    WriteMode mode = (modeRewriteIndex == 0) ? WriteMode::rewrite : WriteMode::addCounter;
    QString fileMask = ui->le_mask->text().trimmed();


    if (fileIn.isEmpty() || dirOut.isEmpty() || fileMask.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Заполните все поля");
        return;
    }

    QVector<QString> filter = fileMask.split(" ", Qt::SkipEmptyParts);
    QVector<QLineEdit*> keys = { ui->le_key_1, ui->le_key_2, ui->le_key_3, ui->le_key_4,
                                 ui->le_key_5, ui->le_key_6, ui->le_key_7, ui->le_key_8 };

    if (!validKeys(keys)) {
        QMessageBox::warning(this, "Ошибка", "Неверный формат ключа");
        return;
    }

    QString fullKey = "0x";
    for (const auto *le : keys) fullKey += le->text();
    bool flag = false;
    uint64_t keyInt = fullKey.toULongLong(&flag, 16);

    if(!flag) {
        QMessageBox::warning(this, "Ошибка", "Ошибка конвертации ключа");
        return;
    }

    if (isEnableTime) {

        m_timer->setInterval(ui->sp_timer->value() * 1000);

        m_timer->disconnect();

        connect(m_timer, &QTimer::timeout, this, [=,this]() {
            if (!m_isProcessing) {
                startWorker(fileIn, dirOut, keyInt, filter, deleteInFile, mode);
            }
        });

        m_timer->start();
        ui->pushButton->setText("Остановить");

        startWorker(fileIn, dirOut, keyInt, filter, deleteInFile, mode);
    } else {
        ui->pushButton->setEnabled(false);
        startWorker(fileIn, dirOut, keyInt, filter, deleteInFile, mode);
    }
}

void MainWindow::on_cb_timer_checkStateChanged(const Qt::CheckState &arg1)
{
    Q_UNUSED(arg1);
    if (ui->cb_timer->isChecked()) {
        ui->sp_timer->setEnabled(true);
    } else {
        ui->sp_timer->setEnabled(false);

        if (m_timer->isActive()) {
            m_timer->stop();
            ui->pushButton->setEnabled(true);
            ui->pushButton->setText("Старт");
        }
    }
}
