#include "gpsnetwork.h"

gpsNetwork::gpsNetwork(QObject *parent) : QObject(parent)
{
    connectedToHost = false;

    dataIn.setDevice(tcpsocket);
    tcpsocket = new QTcpSocket(this);

    dataIn.setDevice(tcpsocket);
    dataIn.setVersion(QDataStream::Qt_5_12);

    connect(tcpsocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleError(QAbstractSocket::SocketError)));
    connect(tcpsocket, &QIODevice::readyRead, this, &gpsNetwork::readData);
    connect(tcpsocket, SIGNAL(connected()), this, SLOT(setConnected()));

    connect(&binLogger, &gpsBinaryLogger::haveFileIOError, this, &gpsNetwork::handleBinaryLoggingErrorNumber);
    connect(&binLogger, &gpsBinaryLogger::haveStatusMessage, this, &gpsNetwork::handleBinaryLoggingStatusMessage);

    connect(this, &gpsNetwork::haveBinaryLoggingFilename, &binLogger, &gpsBinaryLogger::setFilename);

    binLogger.setFilename("/tmp/gps.log"); // temporary filename

}

gpsNetwork::~gpsNetwork()
{
    this->disconnectFromGPS();
    delete tcpsocket;

    binLogger.stopLogging();
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

    binLogger.setFilename(gpsBinaryLogFilename);
    binLogger.startLogging();

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
    binLogger.startLogging();
}

void gpsNetwork::setBinaryLoggingFilename(QString binLogFilename)
{
    emit haveBinaryLoggingFilename(binLogFilename);
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
    QByteArray data;

    tcpsocket->startTransaction();
    data = tcpsocket->readAll();
    tcpsocket->commitTransaction();

    QString gpsdataString;
    gpsdataString = QString("Size: %1, start: 0x%2").arg(data.size()).arg((unsigned char)data.at(0), 2, 16, QChar('0'));

    // Begin decoding in the reader:

    binLogger.insertData(data); // log to binary file
    reader.insertData(data);

    //messageKinds gpsMsgType = reader.getMessageType();
    gpsMessage m = reader.getMessage(); // copy of entire message

    //reader.debugThis();

    //gpsdataString.append(QString(", Data type: %1, version %2, counter: %3, Heading: ").arg(gpsMsgType).arg(reader.getVersion()).arg(reader.getCounter()));
    //gpsdataString.append(reader.debugString());
    //gpsdataString.append(QString(", Heading: %1, Roll: %2, Pitch: %3").arg(reader.getHeading(), 4, 'f', 2).arg(reader.getRoll(), 4, 'f', 2).arg(reader.getPitch(), 4, 'f', 2));
    //gpsdataString.append(QString(", long: %1, lat: %2, alt: %3").arg(m.longitude).arg(m.latitude).arg(m.altitude));

    // For the ASCII modes, thsi is good for debug:
    //gpsdataString = QString::fromLocal8Bit(data);
    //gpsdataString = gpsdataString.trimmed();

    //emit haveGPSString(gpsdataString);
    emit haveGPSMessage(m);
}

void gpsNetwork::handleError(QAbstractSocket::SocketError e)
{
    QString errorString;
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
}

void gpsNetwork::handleBinaryLoggingStatusMessage(QString errorString)
{
    qDebug() << __PRETTY_FUNCTION__ << "Binary log error string: " << errorString;
}

bool gpsNetwork::checkConnected()
{
    return connectedToHost;
}

void gpsNetwork::debugThis()
{
    // place debug code here.
    // emit statusMessage("Debug reached");
    QString errorString = tcpsocket->errorString();
    emit statusMessage(QString("Debug: Last Socket Error: ") + errorString);
    reader.debugThis();
}
