#ifndef GPSCOMMS_H
#define GPSCOMMS_H

#include <mutex>

#include <QObject>
#include <QTcpSocket>
#include <QtSerialPort/QSerialPort>
// include <QSerialPort>
#include <QDataStream>
#include <QIODevice>

#include "gpsbinaryreader.h"
#include "gpsbinarylogger.h"

class gpsComms : public QObject
{
    Q_OBJECT

public:
    enum ConnectionType {
        Network,
        Serial
    };

private:
    // Connection type
    ConnectionType connectionType;

    // Network-related members
    QString gpsHost;
    int gpsPort;

    // Serial-related members
    QString serialPortName;
    int serialBaudRate;

    // Shared connection state
    bool connectedToHost;
    QIODevice *ioDevice;
    QTcpSocket *tcpsocket;
    QSerialPort *serialport;
    QDataStream dataIn;
    QByteArray serialReadBuffer;  // Buffer for incomplete serial data

    gpsBinaryReader reader;
    gpsBinaryLogger binLoggerPrimary;
    gpsBinaryLogger binLoggerSecondary;

    bool createConnection();
    std::mutex readingData;
    QByteArray deepCopyData(const QByteArray data);
    QByteArray deepCopyData(const QByteArray data, int maxLength);

private slots:
    // for TCP socket and serial port:
    void readData();
    void handleError();
    void setConnected();

public:
    explicit gpsComms(QObject *parent = nullptr);
    ~gpsComms();
    bool checkConnected();

public slots:
    // Network-related slots
    void setGPSHost(QString gpsHost, int gpsPort);
    void connectToGPSNetwork(QString gpsHost, int gpsPort, QString gpsBinaryPrimaryLogFilename);

    // Serial-related slots
    void setSerialPort(QString portName, int baudRate);
    void connectToGPSSerial(QString portName, int baudRate, QString gpsBinaryPrimaryLogFilename);

    // Common slots
    void connectToGPS();
    void disconnectFromGPS();
    void beginSecondaryBinaryLog(QString secondaryLogFilename);
    void stopSecondaryBinaryLog();
    void setBinaryLoggingFilenamePrimary(QString binLogFilename);
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

#endif // GPSCOMMS_H
