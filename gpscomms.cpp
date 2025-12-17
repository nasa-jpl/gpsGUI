#include "gpscomms.h"

gpsComms::gpsComms(QObject *parent) : QObject(parent)
{
    connectionType = Network;  // default
    connectedToHost = false;
    gpsPort = 0;
    serialBaudRate = 115200;

    // Initialize network socket
    tcpsocket = new QTcpSocket(this);
    // Initialize serial port
    serialport = new QSerialPort(this);

    // Set network socket as default ioDevice
    ioDevice = tcpsocket;
    dataIn.setDevice(ioDevice);

    binLoggerPrimary.setPrimaryLogStatus(true);
    binLoggerPrimary.setFilename("/tmp/gps_DEFAULTFILENAME_primary.log");

    binLoggerSecondary.setPrimaryLogStatus(false);
    binLoggerSecondary.setFilename("/tmp/gps_DEFAULTFILENAME_secondary.log");

    // Network socket connections
    connect(tcpsocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, [this](QAbstractSocket::SocketError) { this->handleError(); });

    //connect(tcpsocket, &QAbstractSocket::error, this, &gpsComms::handleError);
    connect(tcpsocket, &QIODevice::readyRead, this, &gpsComms::readData);
    connect(tcpsocket, &QAbstractSocket::connected, this, &gpsComms::setConnected);

    // Serial port connections
    connect(serialport, &QSerialPort::readyRead, this, &gpsComms::readData);
    connect(serialport, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::error),
            this, [this](QSerialPort::SerialPortError) { this->handleError(); });
    //connect(serialport, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::error), this, &gpsComms::handleError);

    // Binary logger connections
    connect(&binLoggerPrimary, &gpsBinaryLogger::haveFileIOError, this, &gpsComms::handleBinaryLoggingErrorNumber);
    connect(&binLoggerPrimary, &gpsBinaryLogger::haveStatusMessage, this, &gpsComms::handleBinaryLoggingStatusMessage);

    connect(&binLoggerSecondary, &gpsBinaryLogger::haveFileIOError, this, &gpsComms::handleBinaryLoggingErrorNumber);
    connect(&binLoggerSecondary, &gpsBinaryLogger::haveStatusMessage, this, &gpsComms::handleBinaryLoggingStatusMessage);

    connect(this, &gpsComms::haveBinaryLoggingFilenamePrimary, &binLoggerPrimary, &gpsBinaryLogger::setFilename);
    connect(this, &gpsComms::haveBinaryLoggingFilenameSecondary, &binLoggerSecondary, &gpsBinaryLogger::setFilename);

    connect(this, &gpsComms::startSecondaryBinaryLog, &binLoggerSecondary, &gpsBinaryLogger::beginLogToFilenameNow);
    connect(this, &gpsComms::sendStopSecondaryBinaryLog, &binLoggerSecondary, &gpsBinaryLogger::stopLogging);

    qDebug() << "GPS Comms Constructed.";
}

gpsComms::~gpsComms()
{
    this->disconnectFromGPS();
    delete tcpsocket;
    delete serialport;

    binLoggerPrimary.stopLogging();
}

void gpsComms::setGPSHost(QString gpsHost, int gpsPort)
{
    this->gpsHost = gpsHost;
    this->gpsPort = gpsPort;
    connectionType = Network;
}

void gpsComms::setSerialPort(QString portName, int baudRate)
{
    this->serialPortName = portName;
    this->serialBaudRate = baudRate;
    connectionType = Serial;
}

void gpsComms::setConnected()
{
    connectedToHost = true;
    emit connectionGood();
}

void gpsComms::disconnectFromGPS()
{
    if(connectionType == Network)
    {
        emit statusMessage("Closing GPS network connection.");
        tcpsocket->disconnectFromHost();
    }
    else if(connectionType == Serial)
    {
        if(serialport->isOpen()) {
            emit statusMessage("Closing GPS serial connection.");
            serialport->close();
            serialReadBuffer.clear();
        }
    } else {
        emit statusMessage("Unsure how to close GPS connection for unknown connection type.");
    }
    connectedToHost = false;
}

void gpsComms::connectToGPSNetwork(QString gpsHost, int gpsPort, QString gpsBinaryPrimaryLogFilename)
{
    // Network connection public slot
    this->gpsHost = gpsHost;
    this->gpsPort = gpsPort;
    connectionType = Network;
    ioDevice = tcpsocket;
    dataIn.setDevice(ioDevice);

    binLoggerPrimary.setFilename(gpsBinaryPrimaryLogFilename);
    binLoggerPrimary.startLogging();

    emit statusMessage(QString("About to connect to host %1 on port %2").arg(gpsHost).arg(gpsPort));
    this->createConnection();
}

void gpsComms::connectToGPSSerial(QString portName, int baudRate, QString gpsBinaryPrimaryLogFilename)
{
    // Serial connection public slot
    this->serialPortName = portName;
    this->serialBaudRate = baudRate;
    connectionType = Serial;
    ioDevice = serialport;
    dataIn.setDevice(ioDevice);

    binLoggerPrimary.setFilename(gpsBinaryPrimaryLogFilename);
    binLoggerPrimary.startLogging();

    emit statusMessage(QString("About to connect to serial port %1 at %2 baud").arg(portName).arg(baudRate));
    this->createConnection();
}

void gpsComms::connectToGPS()
{
    // Generic connection using previously set parameters
    this->createConnection();
    binLoggerPrimary.startLogging();
}

void gpsComms::setBinaryLoggingFilenamePrimary(QString binLogFilename)
{
    emit haveBinaryLoggingFilenamePrimary(binLogFilename);
}

void gpsComms::setBinaryLoggingFilenameSecondary(QString binLogFilenameSecondary)
{
    emit haveBinaryLoggingFilenameSecondary(binLogFilenameSecondary);
}

void gpsComms::beginSecondaryBinaryLog(QString secondaryLogFilename)
{
    emit startSecondaryBinaryLog(secondaryLogFilename);
}

void gpsComms::stopSecondaryBinaryLog()
{
    emit sendStopSecondaryBinaryLog();
}

bool gpsComms::createConnection()
{
    if(connectionType == Network)
    {
        if(!gpsHost.isEmpty() && gpsPort)
        {
            emit statusMessage(QString("tcpsocket connect to host running for %1:%2").arg(gpsHost).arg(gpsPort));
            tcpsocket->connectToHost(gpsHost, gpsPort);
        }
    }
    else if(connectionType == Serial)
    {
        if(!serialPortName.isEmpty())
        {
            serialport->setPortName(serialPortName);
            serialport->setBaudRate(serialBaudRate);
            serialport->setDataBits(QSerialPort::Data8);
            serialport->setStopBits(QSerialPort::OneStop);
            serialport->setParity(QSerialPort::NoParity);
            serialport->setFlowControl(QSerialPort::NoFlowControl);

            if(serialport->open(QIODevice::ReadWrite))
            {
                emit statusMessage(QString("Serial port opened successfully"));
                setConnected();
            }
            else
            {
                emit statusMessage(QString("Failed to open serial port: %1").arg(serialport->errorString()));
                emit connectionError(serialport->error());
            }
        }
    } else {
        emit statusMessage("Error, don't know what type of connection to make (serial or network).");
    }

    return true;
}

void gpsComms::readData()
{
    readingData.lock();
    QByteArray data;
    QByteArray dataReadFromDevice;
    QByteArray dataPrimary;
    QByteArray dataSecondary;

    if(connectionType == Network)
    {
        tcpsocket->startTransaction();
        dataReadFromDevice = tcpsocket->readAll();
        tcpsocket->commitTransaction();
    }
    else if(connectionType == Serial)
    {
        dataReadFromDevice = serialport->readAll();
        serialReadBuffer.append(dataReadFromDevice);
        if(serialReadBuffer.length() < 300) {
#ifdef QT_DEBUG
            emit statusMessage(QString("Buffering serial data... %1 bytes so far.").arg(serialReadBuffer.length()));
#endif
            readingData.unlock();
            return;  // Wait for more data
        }
        dataReadFromDevice = serialReadBuffer;
        serialReadBuffer.clear();

#ifdef QT_DEBUG
            emit statusMessage(QString("Serial data buffer is %1 bytes. Handing data to decoder now.").arg(dataReadFromDevice.length()));
#endif
    }

    data = deepCopyData(dataReadFromDevice);

    int decodeCount = 0;

    uint16_t networkDataSize = (uint16_t)data.size();
    uint16_t decodedDataSize = 0;
    uint16_t decodedDataSizeCumulative = 0;
    uint16_t decodedRoundCount = 0;

    volatile int debugCounter = 0;

    do {
        reader.insertData(deepCopyData(data));
        gpsMessage m = reader.getMessage();

        if(m.validDecode)
        {
            dataPrimary = deepCopyData(data, reader.getDataPos());
            dataSecondary = deepCopyData(data, reader.getDataPos());

            binLoggerPrimary.insertData(dataPrimary);
            binLoggerSecondary.insertData(dataSecondary);
        } else {
            debugCounter++;
            emit statusMessage(QString("WARNING: Bad GPS decode at counter %1. Error message: [%2] ").arg(m.counter).arg(m.lastDecodeErrorMessage));
        }

        emit haveGPSMessage(m);
        decodedDataSizeCumulative += reader.getDataPos();
        decodedDataSize = reader.getDataPos();
        decodedRoundCount++;

        if(data.size() < 392) {
            emit statusMessage(QString("Error: Data too small to search further. Giving up on transaction. Transaction had %1 attempted decodes.").arg(decodeCount));
            readingData.unlock();
            return;
        }

        if(networkDataSize > decodedDataSizeCumulative) {
            if(decodedDataSize == 0) {
                if(data.size() > 1) {
                    data.remove(0,1);
                } else {
                    emit statusMessage(QString("Error: Data size zero, cannot search further. Giving up on transaction. Transaction had %1 attempted decodes.").arg(decodeCount));
                    readingData.unlock();
                    return;
                }
            } else if (data.size() > decodedDataSize) {
                data.remove(0, decodedDataSize);
            } else {
                emit statusMessage(QString("Error: Data too small to search further. Giving up on transaction. Transaction had %1 attempted decodes.").arg(decodeCount));
                readingData.unlock();
                return;
            }

            emit statusMessage(QString("Decoding additional transaction data at counter %1, round %2.").arg(m.counter).arg(decodedRoundCount));
        }

        decodeCount++;
        if(decodeCount > 393) {
            emit statusMessage(QString("Error: Too many (%1) bad decode attempts, giving up on TCP transaction.").arg(decodedRoundCount));
            emit statusMessage(QString("Status at error: Data size: %1, cumulative decode size: %2.").arg(data.size()).arg(decodedDataSizeCumulative));
            readingData.unlock();
            return;
        }
    } while( (decodedDataSizeCumulative < networkDataSize) && (networkDataSize !=0) && (data.size() > 300));

    readingData.unlock();
}

void gpsComms::handleError()
{
    QString errorString;

    if(connectionType == Network)
    {
        errorString = tcpsocket->errorString();
        emit statusMessage(QString("GPS Socket Error: ") + errorString);
        emit connectionError((int)tcpsocket->error());
    }
    else if(connectionType == Serial)
    {
        errorString = serialport->errorString();
        emit statusMessage(QString("GPS Serial Port Error: ") + errorString);
        emit connectionError((int)serialport->error());
    }
}

void gpsComms::handleBinaryLoggingErrorNumber(int errorNum)
{
    qDebug() << __PRETTY_FUNCTION__ << "Binary log error number: " << errorNum;
    emit haveGPSString(QString("Binary log file I/O error: [%1].").arg(errorNum));
}

void gpsComms::handleBinaryLoggingStatusMessage(QString errorString)
{
    emit haveGPSString(errorString);
}

bool gpsComms::checkConnected()
{
    return connectedToHost;
}

QByteArray gpsComms::deepCopyData(const QByteArray data)
{
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

QByteArray gpsComms::deepCopyData(const QByteArray data, int maxLength)
{
    uint16_t sourceLength = data.length();
    if(maxLength < sourceLength)
        sourceLength = maxLength;
    QByteArray dest;
    volatile char temp = 0;
    for(int i=0; i < sourceLength; i++)
    {
        temp = data.at(i);
        dest.append(temp);
    }
    return dest;
}

void gpsComms::debugThis()
{
    if(connectionType == Network)
    {
        QString errorString = tcpsocket->errorString();
        emit statusMessage(QString("Debug: Last Socket Error: ") + errorString);
    }
    else if(connectionType == Serial)
    {
        QString errorString = serialport->errorString();
        emit statusMessage(QString("Debug: Last Serial Error: ") + errorString);
    }
    reader.debugThis();
}
