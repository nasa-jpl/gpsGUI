#include "zupt.h"

zupt::zupt(QObject *parent)
    : QObject{parent}
{
    socket = new QTcpSocket(this);
    connect(socket, SIGNAL(connected()), this, SLOT(connectedToHost()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnectFromHost()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(handleDataFromHost()));
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
    connect(socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this, SLOT(handleHostError(QAbstractSocket::SocketError)));
#else
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleHostError(QAbstractSocket::SocketError)));
#endif

    magicZUpT_OFF_text = "$PIXSE,CONFIG,ZUP___,0*41";
    ready = true;
}


void zupt::disableZUpT(QString hostname, int port) {
    genStatusText(QString("About to send data to the Atlans A7 to disable ZUpT. Host: %1:%2").arg(hostname).arg(port));
    socket->connectToHost(hostname, port);
}

void zupt::disconnectFromHost() {
    genStatusText("Disconnected from host.");
}

void zupt::connectedToHost() {
    genStatusText("Connected to host, sending ZUpT OFF cmd.");
    QMutexLocker lock(&textMutex);
    QTextStream outText(socket);
    outText << magicZUpT_OFF_text;
    outText.flush();
    emit successfulConnection();
    genStatusText("Disconnecting from host...");
    socket->close();
}

void zupt::handleHostError(QAbstractSocket::SocketError error)
{
    switch(error)
    {
    case QAbstractSocket::RemoteHostClosedError:
        //qInfo(logLogger()) << "Disconnected from host.";
        break;
    default:
        qWarning() << "Error connecting to ATLANS A7 host. Check internet connection. Error code: " << error;
        genErrorText(QString("Error connecting to ATLANS A7 host. Check internet connection. Error code: %1").arg(error));
        emit connectionError();
        break;
    }
}

void zupt::handleDataFromHost() {
    // Do nothing. The A7 does send data to us,
    // but it is a large amount of redundant data,
    // which we do not use in this module of code.
    // The format is plan text "PIXSE".

    //qDebug() << "Receiving data from logging host.";
    //QByteArray data = socket->readAll();
    return;
    //void(data);
}

void zupt::debugThis() {
    qDebug() << "Reached debug function inside zupt commander.";
    genStatusText("Reached debug function inside zupt commander.");
}

void zupt::genStatusText(QString statusMessage) {
    statusMessage.prepend("[A7 ZUpT]: ");
    emit statusText(statusMessage);
}

void zupt::genErrorText(QString errorMessage) {
    errorMessage.prepend("[A7 ZUpT]: ERROR: ");
    emit errorText(errorMessage);
}
