#include "gpsnetwork.h"

gpsNetwork::gpsNetwork(QObject *parent) : QObject(parent)
{
    connectedToHost = false;

    dataIn.setDevice(tcpsocket);
    tcpsocket = new QTcpSocket(this);

    dataIn.setDevice(tcpsocket);
    //dataIn.setVersion(QDataStream::);

    binLoggerPrimary.setPrimaryLogStatus(true);
    binLoggerPrimary.setFilename("/tmp/gps_DEFAULTFILENAME_primary.log"); // temporary filename

    binLoggerSecondary.setPrimaryLogStatus(false);
    binLoggerSecondary.setFilename("/tmp/gps_DEFAULTFILENAME_secondary.log");

    connect(tcpsocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleError(QAbstractSocket::SocketError)));
    connect(tcpsocket, &QIODevice::readyRead, this, &gpsNetwork::readData);
    connect(tcpsocket, SIGNAL(connected()), this, SLOT(setConnected()));

    connect(&binLoggerPrimary, &gpsBinaryLogger::haveFileIOError, this, &gpsNetwork::handleBinaryLoggingErrorNumber);
    connect(&binLoggerPrimary, &gpsBinaryLogger::haveStatusMessage, this, &gpsNetwork::handleBinaryLoggingStatusMessage);

    connect(&binLoggerSecondary, &gpsBinaryLogger::haveFileIOError, this, &gpsNetwork::handleBinaryLoggingErrorNumber);
    connect(&binLoggerSecondary, &gpsBinaryLogger::haveStatusMessage, this, &gpsNetwork::handleBinaryLoggingStatusMessage);

    connect(this, &gpsNetwork::haveBinaryLoggingFilenamePrimary, &binLoggerPrimary, &gpsBinaryLogger::setFilename);
    connect(this, &gpsNetwork::haveBinaryLoggingFilenameSecondary, &binLoggerSecondary, &gpsBinaryLogger::setFilename);

    connect(this, &gpsNetwork::startSecondaryBinaryLog, &binLoggerSecondary, &gpsBinaryLogger::beginLogToFilenameNow);
    connect(this, &gpsNetwork::sendStopSecondaryBinaryLog, &binLoggerSecondary, &gpsBinaryLogger::stopLogging);

}

gpsNetwork::~gpsNetwork()
{
    this->disconnectFromGPS();
    delete tcpsocket;

    binLoggerPrimary.stopLogging();
}

void gpsNetwork::setGPSHost(QString gpsHost, int gpsPort)
{
    this->gpsHost = gpsHost;
    this->gpsPort = gpsPort;
}

void gpsNetwork::setConnected()
{
    connectedToHost = true;
    emit connectionGood();
}

void gpsNetwork::disconnectFromGPS()
{
    tcpsocket->disconnectFromHost();
    connectedToHost = false;
}

void gpsNetwork::connectToGPS(QString gpsHost, int gpsPort, QString gpsBinaryLogFilename)
{
    // Public slot to start connection

    binLoggerPrimary.setFilename(gpsBinaryLogFilename);
    binLoggerPrimary.startLogging();

    this->gpsHost = gpsHost;
    this->gpsPort = gpsPort;
    emit statusMessage(QString("About to connect to host ") + gpsHost);
    this->createConnection();

}

void gpsNetwork::connectToGPS()
{
    // Public slot to start connection
    // Do not use this function, it is for testing only.
    this->createConnection();
    binLoggerPrimary.startLogging();
}

void gpsNetwork::setBinaryLoggingFilenamePrimary(QString binLogFilename)
{
    emit haveBinaryLoggingFilenamePrimary(binLogFilename);
}

void gpsNetwork::setBinaryLoggingFilenameSecondary(QString binLogFilenameSecondary)
{
    // This function only sets the secondary filename,
    // it does NOT start or stop the log.
    // You should probably not use this function.
    emit haveBinaryLoggingFilenameSecondary(binLogFilenameSecondary);
}

void gpsNetwork::beginSecondaryBinaryLog(QString secondaryLogFilename)
{
    // This function will set the filename for the secondary log,
    // and, will begin logging. If an existing secondary log
    // is already going, that log will be closed and a new one started.
    emit startSecondaryBinaryLog(secondaryLogFilename);
}

void gpsNetwork::stopSecondaryBinaryLog()
{
    emit sendStopSecondaryBinaryLog();
}

bool gpsNetwork::createConnection()
{
    if(!gpsHost.isEmpty() && gpsPort)
    {
        tcpsocket->connectToHost(gpsHost, gpsPort);
    }

    return true;
}

void gpsNetwork::readData()
{
    readingData.lock();
    QByteArray data;
    QByteArray dataPrimary;
    QByteArray dataSecondary;

    tcpsocket->startTransaction();
    data = tcpsocket->readAll();
    tcpsocket->commitTransaction();

    QString gpsdataString;
    gpsdataString = QString("Size: %1, start: 0x%2").arg(data.size()).arg((unsigned char)data.at(0), 2, 16, QChar('0'));

    // Begin decoding in the reader:
    reader.insertData(deepCopyData(data));
    gpsMessage m = reader.getMessage(); // copy of entire message
    //reader.debugThis();
    if(m.validDecode)
    {
        dataPrimary = deepCopyData(data);
        dataSecondary = deepCopyData(data);

        binLoggerPrimary.insertData(dataPrimary); // log to binary file
        binLoggerSecondary.insertData(dataSecondary); // secondary log
    } else {
        emit statusMessage(QString("WARNING: Bad GPS decode at counter %1. Error message: [%2] ").arg(m.counter).arg(m.lastDecodeErrorMessage));
    }

    //emit haveGPSString(gpsdataString);
    emit haveGPSMessage(m);
    readingData.unlock();
}

void gpsNetwork::handleError(QAbstractSocket::SocketError e)
{
    QString errorString;
    // The following is a prototype that we could use in the future
    // to take actions on specific types of errors:

//    switch(e)
//    {
//    case QAbstractSocket::ConnectionRefusedError:
//        errorString = "Connection regused";
//        break;
//    case QAbstractSocket::RemoteHostClosedError:
//        errorString = "Remote host closed";
//        break;
//    case QAbstractSocket::HostNotFoundError:
//        errorString = "Host not found";
//        break;
//    case QAbstractSocket::SocketAccessError:
//        errorString = "Socket Access Error";
//        break;
//    default:
//        errorString = QString("ErrorNumber %1").arg(e);
//        break;


//    }

    errorString = tcpsocket->errorString();
    emit statusMessage(QString("Socket Error: ") + errorString);
    emit connectionError(e);
}

void gpsNetwork::handleBinaryLoggingErrorNumber(int errorNum)
{
    qDebug() << __PRETTY_FUNCTION__ << "Binary log error number: " << errorNum;
    emit haveGPSString(QString("Binary log file I/O error: [%1].").arg(errorNum));
}

void gpsNetwork::handleBinaryLoggingStatusMessage(QString errorString)
{
    //qDebug() << __PRETTY_FUNCTION__ << "Binary log error string: " << errorString;
    emit haveGPSString(errorString);
}

bool gpsNetwork::checkConnected()
{
    return connectedToHost;
}

QByteArray gpsNetwork::deepCopyData(const QByteArray data)
{
    // This function assures a copy is made at some annoying expense
    uint16_t sourceLength = data.length();
    QByteArray dest;
    volatile char temp = 0;
    for(int i=0; i < sourceLength; i++)
    {
        temp = data.at(i);
        dest.append(temp);
    }
    return dest;
}

void gpsNetwork::debugThis()
{
    // place debug code here.
    // emit statusMessage("Debug reached");
    QString errorString = tcpsocket->errorString();
    emit statusMessage(QString("Debug: Last Socket Error: ") + errorString);
    reader.debugThis();
}
