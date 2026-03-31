#ifndef FILEWORKER_H
#define FILEWORKER_H
#include <QDir>
#include <QFile>
#include <QObject>

enum class WriteMode
{
    rewrite,
    addCounter
};


class Worker : public QObject
{
    Q_OBJECT

    public:
        explicit Worker(QObject* parent = nullptr) : QObject(parent) {}

        void processDir(const QString& inputPath, const QString& outDir, uint64_t key, const QVector<QString> & whiteFilter, bool deleteInputFile,WriteMode mode);

        void processFile(const QString& inputPath, const QString& outDir, uint64_t key, bool deleteInputFile,WriteMode mode);
    signals:
        void sendError(const QString& title, const QString& message);
        void finished();

        void changeProcInfo(const QString& status);
        void changeProcProgressBar(int value);


};


#endif // FILEWORKER_H
