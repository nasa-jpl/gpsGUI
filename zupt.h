#ifndef ZUPT_H
#define ZUPT_H

#include <QDebug>
#include <QObject>
#include <QTcpSocket>
#include <QTextStream>
#include <QMutexLocker>
#include <QMutex>


class zupt : public QObject
{
    Q_OBJECT
private:
    bool ready = false;
    QTcpSocket *socket;
    void sendZUpT_OFF_Cmd();
    QMutex textMutex;
    QString magicZUpT_OFF_text;
    void genStatusText(QString statusMessage);
    void genErrorText(QString errorMessage);

public:
    explicit zupt(QObject *parent = nullptr);

public slots:
    void disableZUpT(QString hostname, int port);
    void debugThis();

private slots:
    void connectedToHost();
    void disconnectFromHost();
    void handleDataFromHost();
    void handleHostError(QAbstractSocket::SocketError);

signals:
    void statusText(QString statusMessage);
    void errorText(QString errorMessage);
    void successfulConnection();
    void connectionError();

};

#endif // ZUPT_H
