#ifndef GPSBINARYFILEREADER_H
#define GPSBINARYFILEREADER_H

#include <unistd.h>

#include <QObject>

#include "gpsbinaryreader.h"

class gpsBinaryFileReader : public QObject
{
    Q_OBJECT

    QString filename;
    FILE *binFilePtr = NULL;
    char *rawData; // memory location for reading in portions of the file
    uint16_t maximumMessageSize;
    uint16_t messageSizeBytes;
    unsigned int messageDelayMicroSeconds;
    QByteArray binMessage;
    uint64_t messagesRead = 0;
    int headerV5sizeBytes = 27;
    int lastFileError = 0;
    bool readFirstLine = false;
    bool filenameSet = false;
    bool fileOpen = false;
    void openFile();
    void closeFile();

    gpsBinaryReader reader;
    gpsMessage m;

    gpsBinaryReader debugReader;

    void readSingleMessage();
    void readFile();

    void startProcessFile();
    bool findMessage(); // move file to start of message
    uint16_t readV5header(); // return the size
    bool readRestOfMessage(); // read messageSizeBytes, place into rawData at +2




public:
    gpsBinaryFileReader();
    ~gpsBinaryFileReader();
    bool keepGoing = true;


public slots:
    void setFilename(QString filename);
    void beginWork(); // start from beginning of the file
    void stopWork();

signals:
    void haveGPSMessage(gpsMessage msg);
    void haveFileReadError(int errorNum);
    void haveErrorMessage(QString errorMessage);
    void haveStatusMessage(QString statusMessage);
};

#endif // GPSBINARYFILEREADER_H
