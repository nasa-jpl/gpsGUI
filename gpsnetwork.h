#ifndef GPSNETWORK_H
#define GPSNETWORK_H

#include <QObject>
#include <QTcpSocket>
#include <QDataStream>

#include "gpsbinaryreader.h"

class gpsNetwork : public QObject
{
    Q_OBJECT

    QString gpsHost;
    int gpsPort;
    bool connectedToHost;
    QTcpSocket *tcpsocket;
    QDataStream dataIn;

    gpsBinaryReader reader;

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
    void connectToGPS(QString gpsHost, int gpsPort);
    void connectToGPS();
    void disconnectFromGPS();
    void debugThis();

signals:
    void statusMessage(QString);
    void connectionError(int e);
    void connectionGood();
    void haveGPSString(QString);
    void haveGPSMessage(gpsMessage);

};

#endif // GPSNETWORK_H
