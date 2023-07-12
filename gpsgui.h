#ifndef GPSGUI_H
#define GPSGUI_H

#include <vector>

#include <QMainWindow>
#include <QThread>
#include <QIntValidator>
#include <QElapsedTimer>


#ifdef __APPLE__
#include "qcustomplot-source/qcustomplot.h"
#else
#include <qcustomplot.h>
#endif

#include "gpsnetwork.h"
#include "gpsbinaryreader.h"
#include "gpsbinaryfilereader.h"
#include "mapview.h"

QT_BEGIN_NAMESPACE
namespace Ui { class GpsGui; }
QT_END_NAMESPACE

class GpsGui : public QMainWindow
{
    Q_OBJECT

    QThread *gpsThread;
    gpsNetwork *gps;
    gpsMessage m;
    gpsBinaryFileReader *fileReader;
    QThread *replayThread;
    QString binaryLogReplayFilename;

    mapView *map;


    // Record 1 out of every 40 points for 5 Hz updates to plots
    // therefore, for 90 seconds of data, we need 90*5 = 450 point vectors
    uint16_t vecSize = 450;
    uint16_t vecPosAltHeading = 0;
    uint16_t vecPosPosition = 0;
    uint16_t vecPosSpeed = 0;

    uint16_t msgsReceivedCount = 0;

    // alt and heading
    QVector<double> headings;
    QVector<double> courses;

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
    QVector<double> groundVelos;

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
    void handleErrorMessage(QString);

signals:
    void connectToGPS(QString host, int port, QString binaryLogFilename);
    void disconnectFromGPS();
    void getDebugInfo();
    void setBinaryLogFilename(QString binlogname);
    void setBinaryLogReplayFilename(QString replayFilename);
    void startGPSReplay();
    void stopGPSReplay();
    void setGPSReplaySpeedupFactor(int factor);
    void sendMapCoordinates(double lat, double lng);
    void sendMapRotation(float angle);
    void startSecondaryLog(QString secondaryLogFilename);
    void stopSecondaryLog();

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

    void on_selFileBtn_clicked();

    void on_replayGPSBtn_clicked();

    void on_gpsBinLogOpenEdit_editingFinished();

    void on_stopReplayBtn_clicked();

    void on_showMapBtn_clicked();

    void on_startSecondLogBtn_clicked();

    void on_stopSecondLogBtn_clicked();

    void on_speedupFactorSpin_valueChanged(int arg1);

    void on_replayEnabledChk_toggled(bool checked);

    void on_replayEnabledMainChk_toggled(bool checked);

private:
    Ui::GpsGui *ui;
    dword priorAlgorithmStatus1 = 0;
    dword priorAlgorithmStatus2 = 0;
    dword priorAlgorithmStatus3 = 0;
    dword priorAlgorithmStatus4 = 0;
    dword oldCounter = 0;

    uint64_t droppedTotal = 0;

    void processGNSSInfo(int num);

    unsigned char getBit(uint32_t d, unsigned char bit);
    void processStickyStatus();
    void resetLEDs();
    void setupUI();

    // Sticky is true when in error or warning mode.
    // Sticky is false when in good mode.

    bool navStatusSticky = false;
    bool gpsReceivedSticky = false;
    bool gpsValidSticky = false;
    bool gpsWaitingSticky = false;
    bool gpsRejectedSticky = false;

    bool altitudeSaturationSticky = false;
    bool speedSaturationSticky = false;
    bool interpolationMissedSticky = false;

    bool systemReadySS3Sticky = false;
    bool gpsDetectedSS2Sticky = false;
    bool outputAFullSticky = false;
    bool outputBFullSticky = false;
    bool flashWriteErrorSticky = false;
    bool flashEraseErrorSticky = false;

    bool zuptActivatedSticky = false;
    bool zuptValidSticky = false;

    bool zuptRotationModeSticky = false;
    bool zuptRotationValidSticky = false;

    bool altitudeRejectionSticky = false;


};
#endif // GPSGUI_H
