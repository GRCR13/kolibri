#include "fileworker.h"



void Worker::processDir(const QString& inputPath, const QString& outDir, uint64_t key, const QVector<QString> & whiteFilter, bool deleteInputFile,WriteMode mode)
{
    QDir dir(inputPath);
    if (!dir.exists())
    {
        emit sendError("ошибка", "папки такой нет");
        return;
    }

    if (whiteFilter.isEmpty())
    {
        emit sendError("ошибка", "белый список расширений файлов пустой");
        return;
    }


    QStringList filters;

    for(const  auto &i : whiteFilter)
    {
        filters.append("*."+i);
    }

    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files | QDir::NoSymLinks |QDir::NoDotAndDotDot);

    QFileInfoList files = dir.entryInfoList();

    size_t index = 0;
    for(const auto & i :  files)
    {
        emit changeProcInfo("обработка " + i.fileName());
        processFile(i.filePath(),outDir,key,deleteInputFile,mode);
        emit changeProcProgressBar(index * 100 / files.count());
        ++index;

    }
    emit finished();
}

void Worker::processFile(const QString& inputPath, const QString& outPathFile, uint64_t key, bool deleteInputFile,WriteMode mode)
{
    QFile inFile (inputPath);

    QFileInfo info (inputPath);
    QDir dir (outPathFile);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QString outPath = dir.filePath(info.fileName());

    if (mode == WriteMode::addCounter)
    {
        QString fileName = info.completeBaseName();
        QString fileEx = info.suffix();
        size_t counter = 1;
        QString newOutPath = outPath;
        while (QFile::exists(newOutPath))
        {
            newOutPath = dir.filePath(fileName + "_" + QString::number(counter) + "." + fileEx);
            ++counter;
        }
        outPath = newOutPath;
    }

    QFile outFile(outPath);


    if (!inFile.open(QIODevice::ReadOnly))
    {
        emit sendError("ошибка", "не могу открыть inFile");
        return;
    }

    if (!outFile.open(QIODevice::ReadWrite))
    {
        emit sendError("ошибка", "не могу открыть outFile");
        inFile.close();
        return;
    }

    qint64 weight = inFile.size();

    if (weight == 0)
    {
        inFile.close();
        outFile.close();
        return;
    }

    if (!outFile.resize(weight))
    {
        emit sendError("ошибка","нет места на диске для записи выходного файла");

        inFile.close();
        outFile.close();
        return;
    }

    const qint64 windowSize = 32 * 1024 * 1024;

    for (qint64 offset = 0; offset < weight; offset += windowSize)
    {
        qint64 frame = qMin(windowSize, weight - offset);

        uchar *pIn = inFile.map(offset, frame);
        uchar *pOut = outFile.map(offset, frame);

        if (pIn && pOut)
        {
            emit changeProcProgressBar(static_cast<int>(offset * 100 / weight));

            size_t count = frame / sizeof(uint64_t);
            size_t rem = frame % sizeof(uint64_t);

            uint64_t* pIn64 = reinterpret_cast<uint64_t*>(pIn);
            uint64_t* pOut64 = reinterpret_cast<uint64_t*>(pOut);

            for (size_t i = 0; i < count; ++i) {
                pOut64[i] = pIn64[i] ^ key;
            }

            if (rem > 0) {
                size_t tailOffset = count * sizeof(uint64_t);
                uint8_t* keyBytes = reinterpret_cast<uint8_t*>(&key);
                for (size_t i = 0; i < rem; ++i) {
                    pOut[tailOffset + i] = pIn[tailOffset + i] ^ keyBytes[i];
                }
            }

            inFile.unmap(pIn);
            outFile.unmap(pOut);
        }
        else
        {
            emit sendError("Ошибка", "ошибка мапинга ");
            break;
        }
    }

    inFile.close();
    outFile.close();

    if (deleteInputFile)
    {
        inFile.remove();
    }

}


