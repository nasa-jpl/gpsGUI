#ifndef GPSGUI_H
#define GPSGUI_H

#include <vector>

#include <QMainWindow>
#include <QThread>
#include <QIntValidator>
#include <QElapsedTimer>

#include <qcustomplot.h>

#include "gpsnetwork.h"
#include "gpsbinaryreader.h"

QT_BEGIN_NAMESPACE
namespace Ui { class GpsGui; }
QT_END_NAMESPACE

class GpsGui : public QMainWindow
{
    Q_OBJECT

    QThread *gpsThread;
    gpsNetwork *gps;
    gpsMessage m;

    // Record 1 out of every 40 points for 5 Hz updates to plots
    // therefore, for 90 seconds of data, we need 90*5 = 450 point vectors
    uint16_t vecSize = 450;
    uint16_t vecPosAltHeading = 0;
    uint16_t vecPosPosition = 0;
    uint16_t vecPosSpeed = 0;

    uint16_t msgsReceivedCount = 0;

    // alt and heading
    QVector<double> headings;
    QVector<double> rolls;
    QVector<double> pitches;

    // position
    QVector<double> lats;
    QVector<double> longs;
    QVector<double> alts;

    // speed
    QVector<double> nVelos;
    QVector<double> eVelos;
    QVector<double> upVelos;

    // time axis:
    QVector<double> timeAxis;

    void preparePlots();
    void updatePlots();
    void setTimeAxis(QCPAxis *x);
    void setPlotTitle(QCustomPlot *p, QString title);

    void setPlotColors(QCustomPlot *p, bool dark);

    QTimer gpsMessageHeartbeat;

    QElapsedTimer gnssStatusTime;
    bool firstMessage;

    void showStatusMessage(QString);

public:
    GpsGui(QWidget *parent = nullptr);
    ~GpsGui();

public slots:
    void receiveGPSMessage(gpsMessage m);

signals:
    void connectToGPS(QString host, int port);
    void disconnectFromGPS();
    void getDebugInfo();

private slots:
    void handleGPSStatusMessage(QString message);
    void handleGPSDataString(QString gpsString);
    void handleGPSConnectionError(int error);
    void handleGPSConnectionGood();
    void handleGPSTimeout();
    void on_connectBtn_clicked();

    void on_disconnectBtn_clicked();

    void on_debugBtn_clicked();

    void on_clearBtn_clicked();

    void on_clearErrorBtn_clicked();

private:
    Ui::GpsGui *ui;
};
#endif // GPSGUI_H
