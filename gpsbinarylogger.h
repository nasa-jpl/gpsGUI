#ifndef GPSBINARYLOGGER_H
#define GPSBINARYLOGGER_H

#include <vector>
#include <mutex>

#include <QObject>
#include <QByteArray>
#include <QString>


class gpsBinaryLogger : public QObject
{
    Q_OBJECT

    void setupBuffer();
    uint16_t getBufferSize();
    void clearBuffer();
    void writeBufferToFile();
    void openFileWriting();
    void closeFileWriting();
    bool amWritingFile = false;
    bool fileIsOpen = false;
    bool filenameSet = false;
    bool loggingAllowed = false;
    int lastFileIOError = 0;
    uint16_t idealBufferSize;
    uint64_t messageCount = 0;

    std::mutex bufferMutex;
    std::vector<QByteArray> buffer;

    std::mutex fileMutex;
    FILE *fileWritePtr = NULL;
    QString filename;


public:
    gpsBinaryLogger();
    ~gpsBinaryLogger();


public slots:
    void setFilename(QString filename);
    void startLogging();
    void stopLogging();
    void insertData(QByteArray raw);
    void getFilenameSetStatus();
    void getLifetimeMessageCount();
    void debugThis();

signals:
    void haveLifetimeMessageCount(uint64_t count);
    void haveFilename(bool have);
    void haveFileIOError(int errornum);
    void haveStatusMessage(QString statusMessage);


};

#endif // GPSBINARYLOGGER_H
