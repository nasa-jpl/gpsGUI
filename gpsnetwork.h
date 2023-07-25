#ifndef GPSNETWORK_H
#define GPSNETWORK_H

#include <mutex>

#include <QObject>
#include <QTcpSocket>
#include <QDataStream>

#include "gpsbinaryreader.h"
#include "gpsbinarylogger.h"

class gpsNetwork : public QObject
{
    Q_OBJECT

    QString gpsHost;
    int gpsPort;
    bool connectedToHost;
    QTcpSocket *tcpsocket;
    QDataStream dataIn;

    gpsBinaryReader reader;
    gpsBinaryLogger binLoggerPrimary;
    gpsBinaryLogger binLoggerSecondary;

    bool createConnection();
    std::mutex readingData;
    QByteArray deepCopyData(const QByteArray data);
    QByteArray deepCopyData(const QByteArray data, int maxLength);

private slots:
    // for the TCP socket:
    void readData();
    void handleError(QAbstractSocket::SocketError);
    void setConnected();


public:
    explicit gpsNetwork(QObject *parent = nullptr);
    ~gpsNetwork();
    bool checkConnected();

public slots:
    void setGPSHost(QString gpsHost, int gpsPort);
    void connectToGPS(QString gpsHost, int gpsPort, QString gpsBinaryPrimaryLogFilename);
    //void changePrimaryLogFilename(QString primaryLogFilename);
    void beginSecondaryBinaryLog(QString secondaryLogFilename); // Entry point to start 2nd logger.
    void stopSecondaryBinaryLog();
    void connectToGPS();
    void disconnectFromGPS();
    void setBinaryLoggingFilenamePrimary(QString binLogFilename); // TODO: Allow to "change" existing log
    void setBinaryLoggingFilenameSecondary(QString binLogFilename);
    void debugThis();

    // Binary logging slots:
    void handleBinaryLoggingStatusMessage(QString errorString);
    void handleBinaryLoggingErrorNumber(int errorNum);

signals:
    void statusMessage(QString);
    void connectionError(int e);
    void connectionGood();
    void haveGPSString(QString);
    void haveGPSMessage(gpsMessage);
    void haveBinaryLoggingFilenamePrimary(QString binLogFilenamePrimary);
    void haveBinaryLoggingFilenameSecondary(QString binLogFilenameSecondary);
    void startSecondaryBinaryLog(QString secondaryLogFilename);
    void sendStopSecondaryBinaryLog();
};

#endif // GPSNETWORK_H
