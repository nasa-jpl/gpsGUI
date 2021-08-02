#ifndef GPSNETWORK_H
#define GPSNETWORK_H

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
    gpsBinaryLogger binLogger;

    bool createConnection();

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
    void connectToGPS(QString gpsHost, int gpsPort, QString gpsBinaryLogFilename);
    void connectToGPS();
    void disconnectFromGPS();
    void setBinaryLoggingFilename(QString binLogFilename);
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
    void haveBinaryLoggingFilename(QString binLogFilename);

};

#endif // GPSNETWORK_H
