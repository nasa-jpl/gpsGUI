#include "gpsgui.h"
#include "ui_gpsgui.h"

GpsGui::GpsGui(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GpsGui)
{
    ui->setupUi(this);

#ifndef QT_DEBUG
    ui->debugBtn->setEnabled(false);
    ui->debugBtn->setVisible(false);
#endif

#ifdef __APPLE__
    ui->gpsBinLogOpenEdit->setText(QDir::homePath());
#endif

    vecSize = 450;
    msgsReceivedCount = 0;

    headings.resize(vecSize);
    courses.resize(vecSize);
    rolls.resize(vecSize);
    pitches.resize(vecSize);
    lats.resize(vecSize);
    longs.resize(vecSize);
    alts.resize(vecSize);
    nVelos.resize(vecSize);
    eVelos.resize(vecSize);
    upVelos.resize(vecSize);
    groundVelos.resize(vecSize);
    timeAxis.resize(vecSize);

    preparePlots();
    resetLEDs();

    ui->statusConnectionLED->setText("");
    ui->statusConnectionLED->setState(QLedLabel::StateOkBlue);
    ui->statusConnectedLabel->setText("NC"); // not connected

    ui->statusCounterLED->setText("");
    ui->statusCounterLED->setState(QLedLabel::StateOkBlue);
    ui->statusCounterLabel->setText("0");

    ui->statusDecodeOkLED->setText("");
    ui->statusDecodeOkLED->setState(QLedLabel::StateOkBlue);
    ui->statusDecodeOkLabel->setText("NA"); // not applicable

    ui->statusHeartbeatLED->setText("");
    ui->statusSatelliteRxLED->setText("");
    ui->statusSatelliteRxLED->setToolTip("Red if too much time between GPS messages.");

    firstMessage = true;

    qRegisterMetaType<gpsMessage>();

    gps = new gpsNetwork();
    gpsThread = new QThread(this);

    gps->moveToThread(gpsThread);

    connect(gpsThread, &QThread::finished, gps, &QObject::deleteLater);

    fileReader = new gpsBinaryFileReader();
    replayThread = new QThread(this);
    fileReader->moveToThread(replayThread);
    connect(replayThread, &QThread::finished, fileReader, &QObject::deleteLater);


    connect(this, SIGNAL(connectToGPS(QString,int,QString)), gps, SLOT(connectToGPS(QString,int,QString)));
    connect(this, SIGNAL(disconnectFromGPS()), gps, SLOT(disconnectFromGPS()));
    connect(this, SIGNAL(getDebugInfo()), gps, SLOT(debugThis()));
    connect(gps, SIGNAL(haveGPSString(QString)), this, SLOT(handleGPSDataString(QString)));
    connect(gps, SIGNAL(statusMessage(QString)), this, SLOT(handleGPSStatusMessage(QString)));
    connect(gps, SIGNAL(connectionError(int)), this, SLOT(handleGPSConnectionError(int)));
    connect(gps, SIGNAL(connectionGood()), this, SLOT(handleGPSConnectionGood()));

    connect(gps, SIGNAL(haveGPSMessage(gpsMessage)), this, SLOT(receiveGPSMessage(gpsMessage)));
    //connect(this, SIGNAL(setBinaryLogFilename(QString)), gps, SLOT(setBinaryLoggingFilename(QString)));

    connect(this, SIGNAL(startSecondaryLog(QString)), gps, SLOT(beginSecondaryBinaryLog(QString)));
    connect(this, SIGNAL(stopSecondaryLog()), gps, SLOT(stopSecondaryBinaryLog()));

    gpsThread->start();

    connect(this, SIGNAL(setBinaryLogReplayFilename(QString)), fileReader, SLOT(setFilename(QString)));
    connect(fileReader, SIGNAL(haveErrorMessage(QString)), this, SLOT(handleErrorMessage(QString)));
    connect(fileReader, SIGNAL(haveStatusMessage(QString)), this, SLOT(handleGPSStatusMessage(QString)));
    connect(fileReader, SIGNAL(haveGPSMessage(gpsMessage)), this, SLOT(receiveGPSMessage(gpsMessage)));
    connect(this, SIGNAL(startGPSReplay()), fileReader, SLOT(beginWork()));
    connect(this, SIGNAL(stopGPSReplay()), fileReader, SLOT(stopWork()));
    connect(this, SIGNAL(setGPSReplaySpeedupFactor(int)), fileReader, SLOT(setSpeedupFactor(int)));
    replayThread->start();

    ui->gpsPort->setValidator( new QIntValidator(0,65535,this) );

    gpsMessageHeartbeat.setInterval(500); // half second, expected is 5ms.

    connect(&gpsMessageHeartbeat, SIGNAL(timeout()), this, SLOT(handleGPSTimeout()));

    map = new mapView();

    connect(this, SIGNAL(sendMapCoordinates(double,double)), map, SLOT(handleMapUpdatePosition(double,double)));
    connect(this, SIGNAL(sendMapRotation(float)), map, SLOT(handleMapUpdateRotation(float)));
}

GpsGui::~GpsGui()
{
    fileReader->keepGoing = false;

    replayThread->quit();
    replayThread->wait();

    gpsThread->quit();
    gpsThread->wait();

    delete ui;
}

void GpsGui::showStatusMessage(QString s)
{
    ui->statusbar->showMessage(s, 2000);
}

void GpsGui::handleGPSStatusMessage(QString message)
{
    showStatusMessage(message);
    ui->logViewer->appendPlainText(message);
}

void GpsGui::handleGPSDataString(QString gpsString)
{
    //showStatusMessage(gpsString);
    ui->logViewer->appendPlainText(gpsString);
}

void GpsGui::receiveGPSMessage(gpsMessage m)
{
    // Reference for Q Flight Instruments:
    // https://github.com/marek-cel/QFlightinstruments

    float longitude;
    bool doStickyUpdate = false;

    if(m.validDecode)
    {
        this->m = m;
    } else {
        ui->statusDecodeOkLED->setState(QLedLabel::StateError);
        ui->statusDecodeOkLabel->setText("NG");
        qDebug() << "ERROR: Invalid message decode.";
        qDebug() << "Message debug dump:";
        on_debugBtn_clicked();
        return;
    }

    msgsReceivedCount++;

    if(firstMessage)
    {
        gnssStatusTime.start();
        ui->statusSatelliteRxLED->setState(QLedLabel::StateOk);
        //firstMessage = false; // set at end of function.
    }

    bool doPlotUpdate = ((msgsReceivedCount%40)==0) && ui->drawWidgetsChk->isChecked();
    bool doWidgetPaint = ((msgsReceivedCount%51)==0) && ui->drawWidgetsChk->isChecked();
    bool doLabelUpdate = (msgsReceivedCount%10)==0;
    bool doMapUpdate = ((msgsReceivedCount%50)==0) && ui->drawWidgetsChk->isChecked();


    gpsMessageHeartbeat.start();


    if(m.haveINSAlgorithmStatus)
    {
        if( (m.algorithmStatus1 == priorAlgorithmStatus1) && (!firstMessage))
        {
            // do nothing
        } else {
            if(getBit(m.algorithmStatus1, 0))
            {
                ui->navStatus->setState(QLedLabel::StateOk);
            } else {
                ui->navStatus->setState(QLedLabel::StateError);
                navStatusSticky = true;
            }

            bool alignmentPhase = false;
            bool fineAlignment = false;
            if(getBit(m.algorithmStatus1, 1))
            {
                // 5 minutes from power
                ui->gpsAlignment->setText("ALIGNMENT PHASE");
                ui->gpsAlignment->setToolTip("Do not move.\n Pitch, roll, and compass calibrating.");

                alignmentPhase = true;
            }
            if(getBit(m.algorithmStatus1, 2))
            {
                ui->gpsAlignment->setText("FINE");
                fineAlignment = true;
                ui->gpsAlignment->setToolTip("Movement ok. Some data at lower quality.");
            }
            if( (!fineAlignment) && (!alignmentPhase) )
            {
                // Course? We don't know what this state really is.
                // Maybe it means "complete".
                ui->gpsAlignment->setText("Complete");
                ui->gpsAlignment->setToolTip("Alignment complete, data ready.");
            }

            if(getBit(m.algorithmStatus1, 12))
            {
                ui->gpsReceivedStatus->setState(QLedLabel::StateOk);
            } else {
                ui->gpsReceivedStatus->setState(QLedLabel::StateError);
                gpsReceivedSticky = true;
            }
            if(getBit(m.algorithmStatus1, 13))
            {
                ui->gpsValidStatus->setState(QLedLabel::StateOk);
            } else {
                ui->gpsValidStatus->setState(QLedLabel::StateError);
                gpsValidSticky = true;
            }
            if(getBit(m.algorithmStatus1, 14))
            {
                ui->gpsWaitingStatus->setState(QLedLabel::StateWarning);
                gpsWaitingSticky = true;
            } else {
                ui->gpsWaitingStatus->setState(QLedLabel::StateOk);
            }
            if(getBit(m.algorithmStatus1, 15))
            {
                ui->gpsRejectedStatus->setState(QLedLabel::StateError);
                gpsRejectedSticky = true;
            } else {
                ui->gpsRejectedStatus->setState(QLedLabel::StateOk);
            }

            if(getBit(m.algorithmStatus1, 28)) {
                ui->altitudeSaturationStatus->setState(QLedLabel::StateError);
                altitudeSaturationSticky = true;
            } else {
                ui->altitudeSaturationStatus->setState(QLedLabel::StateOk);
            }

            if(getBit(m.algorithmStatus1, 29)) {
                ui->speedSaturationStatus->setState(QLedLabel::StateError);
                speedSaturationSticky = true;
            } else {
                ui->speedSaturationStatus->setState(QLedLabel::StateOk);
            }

            if(getBit(m.algorithmStatus1, 30)) {
                ui->interpolationMissedStatus->setState(QLedLabel::StateError);
                interpolationMissedSticky = true;
            } else {
                ui->interpolationMissedStatus->setState(QLedLabel::StateOk);
            }

            if(getBit(m.algorithmStatus4, 28)) {
                ui->flashWriteErrorStatus->setState(QLedLabel::StateError);
                flashWriteErrorSticky = true;
            } else {
                ui->flashWriteErrorStatus->setState(QLedLabel::StateOk);
            }

            if(getBit(m.algorithmStatus4, 29)) {
                ui->flashEraseErrorStatus->setState(QLedLabel::StateError);
                flashEraseErrorSticky = true;
            } else {
                ui->flashEraseErrorStatus->setState(QLedLabel::StateOk);
            }


            doStickyUpdate = true;


            priorAlgorithmStatus1 = m.algorithmStatus1;
            priorAlgorithmStatus2 = m.algorithmStatus2;
            priorAlgorithmStatus3 = m.algorithmStatus3;
            priorAlgorithmStatus4 = m.algorithmStatus4;

        }
    }

    if(m.haveINSSystemStatus)
    {
        if(getBit(m.systemStatus1, 17)) {
            ui->outputAFullStatus->setState(QLedLabel::StateError);
            outputAFullSticky = true;
        } else {
            ui->outputAFullStatus->setState(QLedLabel::StateOk);
        }

        if(getBit(m.systemStatus1, 18)) {
            ui->outputBFullStatus->setState(QLedLabel::StateError);
            outputBFullSticky = true;
        } else {
            ui->outputBFullStatus->setState(QLedLabel::StateOk);
        }

        if(getBit(m.systemStatus2, 2)) {
            ui->gpsDetectedSS2Status->setState(QLedLabel::StateOk);
        } else {
            ui->gpsDetectedSS2Status->setState(QLedLabel::StateError);
            gpsDetectedSS2Sticky = true;
        }

        if(getBit(m.systemStatus3, 18)) {
            ui->systemReadySS3Status->setState(QLedLabel::StateOk);
        } else {
            ui->systemReadySS3Status->setState(QLedLabel::StateError);
            systemReadySS3Sticky = true;
        }

        doStickyUpdate = true;
    }

    if(m.haveGNSSInfo1 || m.haveGNSSInfo2 || m.haveGNSSInfo3)
    {
        gnssStatusTime.restart();

        if(m.haveGNSSInfo1)
        {
            processGNSSInfo(1);
        }
        if(m.haveGNSSInfo2)
        {
            processGNSSInfo(2);
        }
        if(m.haveGNSSInfo3)
        {
            processGNSSInfo(3);
        }

    } else {
        if(gnssStatusTime.elapsed() > 5*1000)
        {
            ui->statusSatelliteRxLED->setState(QLedLabel::StateError);
            // qDebug() << "gnss Status Time elapsed > 5000, possible dropped messages.";
        } else if (gnssStatusTime.elapsed() > 2*1000)
        {
            // Expected interval is every 1 second.
            ui->statusSatelliteRxLED->setState(QLedLabel::StateWarning);
            // qDebug() << "gnss Status Time elapsed > 2000, possibly dropped messages.";
        }
    }




    if(doLabelUpdate)
    {
        if(m.numberDropped > 0)
        {
            ui->statusCounterLED->setState(QLedLabel::StateError);
            ui->statusCounterLabel->setText(QString("NG:%1").arg(m.numberDropped));
        } else {
            ui->statusCounterLED->setState(QLedLabel::StateOk);
            ui->statusCounterLabel->setText(QString("OK:%1").arg(m.numberDropped));
        }
        if(m.validDecode)
        {
            ui->statusDecodeOkLED->setState(QLedLabel::StateOk);
            ui->statusDecodeOkLabel->setText("OK");
        } else {
            ui->statusDecodeOkLED->setState(QLedLabel::StateError);
            ui->statusDecodeOkLabel->setText("NG");
        }

        uint64_t t = m.navDataValidityTime;
        int hour = t / ((float)1E4)/60.0/60.0;
        int minute = ( t / ((float)1E4)/60.0 ) - (hour*60) ;
        float second = ( t / ((float)1E4) ) - (hour*60.0*60.0) - (minute*60.0);

        QString time = QString("%1:%2:%3 UTC").arg(hour, 2, 10, QChar('0')).arg(minute, 2, 10, QChar('0')).arg(second, 6, 'f', 3, QChar('0'));
        ui->validityTimeLabel->setText(time);
    }


    if(m.haveAltitudeHeading)
    {
        if(doLabelUpdate)
        {
            ui->headingDataLabel->setText(QString("%1").arg(m.heading, 0, 'f', 6));
            ui->rollDataLabel->setText(QString("%1").arg(m.roll, 0, 'f', 6));
            ui->pitchDataLabel->setText(QString("%1").arg(m.pitch, 0, 'f', 6));
        }

        // Store for plots:
        if(doPlotUpdate)
        {
            //uint16_t vecPosAltHeadingMod = (vecPosAltHeading++) % vecSize;

            headings.push_front(m.heading);
            headings.pop_back();

            if(m.haveCourseSpeedGroundData)
            {
                courses.push_front(m.courseOverGround);
                courses.pop_back();
            }

            rolls.push_front(m.roll);
            rolls.pop_back();

            pitches.push_front(m.pitch);
            pitches.pop_back();
        }

        // place in flight widgets:
        ui->EADI->setHeading(m.heading);
        ui->EADI->setPitch(m.pitch * -1 );
        ui->EADI->setRoll(m.roll);

        ui->EHSI->setHeading(m.heading);
        ui->EHSI->setBearing(m.heading);
    }

    if(m.havePosition)
    {
        if(m.longitude > 180)
        {
            longitude = -360+m.longitude;
        } else {
            longitude = m.longitude;
        }
        if(doLabelUpdate)
        {
            ui->latitudeDataLabel->setText(QString("%1").arg(m.latitude, 0, 'f', 8));
            ui->longitudeDataLabel->setText(QString("%1").arg(longitude, 0, 'f', 8));
            ui->altitudeDataLabel->setText(QString("%1").arg(m.altitude, 0, 'f', 7));
        }
        if(doMapUpdate)
        {
            if(map->isVisible())
            {
                emit sendMapCoordinates(m.latitude, longitude);
            }
        }
        ui->EADI->setAltitude(m.altitude);

        if(doPlotUpdate)
        {
            //uint16_t vecPosPositionMod = (vecPosPosition++) % vecSize;

            lats.push_front(m.latitude);
            lats.pop_back();

            longs.push_front(m.longitude);
            longs.pop_back();

            alts.push_front(m.altitude);
            alts.pop_back();
        }

    }

    if(m.haveCourseSpeedGroundData)
    {
        if(m.speedOverGround > 0.1)
            ui->EHSI->setCourse(m.courseOverGround);
        ui->EADI->setAirspeed(m.speedOverGround);
        ui->airSpeedIndicator->setAirspeed(m.speedOverGround * 1.94384); // 1 meter per second = 1.94384 knots
        if(doLabelUpdate)
        {
            ui->groundSpeedDataLabel->setText(QString("%1").arg(m.speedOverGround * 1.94384));
            if(m.speedOverGround > 0.1)
            {
                if(doMapUpdate)
                {
                    if(map->isVisible())
                        emit sendMapRotation(m.courseOverGround);
                }
            }
        }
        if(doPlotUpdate)
        {
            groundVelos.push_front(m.speedOverGround);
            groundVelos.pop_back();
        }
    }
    if(m.haveSpeedData)
    {
        ui->verticalSpeedIndicator->setClimbRate(m.upVelocity * 196.85); // 1 meter per second = 196.85 feet per 100 minutes
        ui->EADI->setClimbRate(m.upVelocity * 196.85);

        if(doLabelUpdate)
        {
            ui->rateOfClimbDataLabel->setText(QString("%1").arg(m.upVelocity * 196.85));
        }
    }

    if(m.haveSpeedData)
    {
        if(doPlotUpdate)
        {
            //uint16_t vecPosSpeedMod = (vecPosSpeed++) % vecSize;

            nVelos.push_front(m.northVelocity);
            nVelos.pop_back();

            eVelos.push_front(m.eastVelocity);
            eVelos.pop_back();

            upVelos.push_front(m.upVelocity);
            upVelos.pop_back();
        }

    }

    if(m.haveSystemDateData)
    {
        QString date = QString("%1-%2-%3").arg(m.systemYear).arg(m.systemMonth, 2, 10, QChar('0')).arg(m.systemDay, 2, 10, QChar('0'));
        ui->utcDateLabel->setText(date);
    }

    if(m.haveUTC)
    {
        // This message is available every second, unless there is a
        // skip counter issue occuring, in which case it is skipped.

        // Note: This GPS device converts GPST to UTC for us.
        // Otherwise, we would need to account for the differences in the various GNSS satellite systems, which do not all follow GPST or UTC.

        // The units are in 100 micro-seconds.
        uint64_t t = m.UTCdataValidityTime;
        int hour = t / ((float)1E4)/60.0/60.0;
        int minute = ( t / ((float)1E4)/60.0 ) - (hour*60) ;
        //int second = ( t / ((float)1E4) ) - (hour*60.0*60.0) - (minute*60.0);
        float secondD = ( t / ((float)1E4) ) - (hour*60.0*60.0) - (minute*60.0);

        QString time = QString("%1:%2:%3 UTC").arg(hour, 2, 10, QChar('0')).arg(minute, 2, 10, QChar('0')).arg(secondD, 6, 'f', 3, QChar('0'));

        ui->utcTimeLabel->setText(time);
        //qDebug() << "UTC time received: " << t << " (100 micro-seconds) hours: " << hour << " minute: " << minute << " second: " << second << " string: " << time;

        uint64_t tN = m.navDataValidityTime;
        int hourN = tN / ((float)1E4)/60.0/60.0;
        int minuteN = ( tN / ((float)1E4)/60.0 ) - (hourN*60) ;
        float secondN = ( tN / ((float)1E4) ) - (hourN*60.0*60.0) - (minuteN*60.0);

        float deltaT = 0.0;
        deltaT = secondN - secondD; // navValidTime - utcDataValidityTime
        qDebug() << "Seconds N: " << QString("%1").arg(secondN, 0, 'f', 10) << ", Seconds D: " << secondD << ", DeltaT: " << QString("%1").arg(deltaT, 0, 'f', 10) << "Counter: " << m.counter << "Old Counter: " << oldCounter << "DeltaCounter: " << m.counter-oldCounter;
        ui->deltaTimeLabel->setText(QString("%1").arg(deltaT, 0, 'f', 10));
        oldCounter = m.counter;
    }


    if(doWidgetPaint)
    {
        ui->EHSI->redraw();
        ui->EADI->redraw();
        ui->airSpeedIndicator->redraw();
        ui->verticalSpeedIndicator->redraw();
    }
    if(doPlotUpdate)
        updatePlots();

    if(doStickyUpdate)
        processStickyStatus();

    if(firstMessage)
    {
        firstMessage = false;
    }

}

void GpsGui::preparePlots()
{
    // set titles, axis, range, etc

    uint16_t t=0;
    for(int i=vecSize; i > 0; i--)
    {
        timeAxis[i-1] = t++;
    }

    // Add graph 0:

    ui->plotLatLong->addGraph(); // Lat
    ui->plotAltitude->addGraph();
    ui->plotSpeed->addGraph(); // X
    ui->plotSpeed->addGraph(0,0); // Y
    ui->plotSpeed->addGraph(0,0); // Z
    ui->plotSpeed->addGraph(0,0); // Ground

    ui->plotPitch->addGraph();
    ui->plotRoll->addGraph();

    ui->plotHeading->addGraph(); // magnetic
    ui->plotHeading->addGraph(0,0); // course
    ui->plotHeading->graph(0)->setName("Magnetic");
    ui->plotHeading->graph(1)->setName("Course");
    ui->plotHeading->yAxis->setRange(0, 360*1.1); // raw data is 0-360 but we will show it like this.
    ui->plotHeading->graph(0)->setPen(QPen(Qt::blue));
    ui->plotHeading->graph(1)->setPen(QPen(Qt::red));
    ui->plotHeading->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft|Qt::AlignTop);
    ui->plotHeading->legend->setBrush(QColor("#85ffffff")); // 85% opaque white
    ui->plotHeading->legend->setVisible(true);

    // Y-Axis Range:
    //ui->plotLatLong->yAxis->setRange(-90*1.1, 90*1.1); // Lat
    ui->plotLatLong->yAxis->setRange(-360*1.25, 360*1.25); // Lat

    //ui->plotLatLong->yAxis2->setRange(0, 360*1.25); // Long
    //ui->plotLatLong->addGraph(ui->plotLatLong->xAxis, ui->plotLatLong->yAxis ); // Lat
    //ui->plotLatLong->addGraph(ui->plotLatLong->yAxis2, ui->plotLatLong->xAxis ); // Long
    ui->plotLatLong->addGraph(ui->plotLatLong->xAxis, ui->plotLatLong->yAxis ); // Long



    ui->plotAltitude->yAxis->setRange(0, 7620); // Altitude at 25k feet

    ui->plotSpeed->yAxis->setRange(-275, 275); // 128 meters per second is 250 knots

    ui->plotPitch->yAxis->setRange(-90*1.1, 90*1.1);
    ui->plotRoll->yAxis->setRange(-180*1.1, 180*1.1);

    // X-Axis range:
    setTimeAxis(ui->plotLatLong->xAxis);
    setTimeAxis(ui->plotAltitude->xAxis);
    setTimeAxis(ui->plotSpeed->xAxis);
    setTimeAxis(ui->plotPitch->xAxis);
    setTimeAxis(ui->plotRoll->xAxis);
    setTimeAxis(ui->plotHeading->xAxis);

    // titles:
    setPlotTitle(ui->plotLatLong, QString("Latitude and Longitude"));
    setPlotTitle(ui->plotAltitude, QString("Altitude above MSL"));
    setPlotTitle(ui->plotSpeed, QString("Speed: X,Y,Z and Ground"));
    setPlotTitle(ui->plotPitch, QString("Pitch"));
    setPlotTitle(ui->plotRoll, QString("Roll"));
    setPlotTitle(ui->plotHeading, QString("Heading (Mag, Course)"));

    // set labels:

    // colors:
    ui->plotLatLong->graph(0)->setPen(QPen(Qt::blue));
    ui->plotLatLong->graph(1)->setPen(QPen(Qt::red));
    ui->plotLatLong->graph(0)->setName("Lat");
    ui->plotLatLong->graph(1)->setName("Long");
    ui->plotLatLong->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft|Qt::AlignTop);
    ui->plotLatLong->legend->setBrush(QColor("#85ffffff")); // 85% opaque white
    ui->plotLatLong->legend->setVisible(true);


    ui->plotSpeed->graph(0)->setPen(QPen(Qt::red));    // X
    ui->plotSpeed->graph(1)->setPen(QPen(Qt::blue));   // Y
    ui->plotSpeed->graph(2)->setPen(QPen(Qt::darkMagenta)); // Z
    ui->plotSpeed->graph(3)->setPen(QPen(Qt::green));  // Ground
    ui->plotSpeed->graph(0)->setName("X");
    ui->plotSpeed->graph(1)->setName("Y");
    ui->plotSpeed->graph(2)->setName("Z");
    ui->plotSpeed->graph(3)->setName("GND");
    ui->plotSpeed->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignLeft|Qt::AlignTop);
    ui->plotSpeed->legend->setBrush(QColor("#85ffffff")); // 85% opaque white
    ui->plotSpeed->legend->setVisible(true);

    setPlotColors(ui->plotHeading, false);

}

void GpsGui::setPlotTitle(QCustomPlot *p, QString title)
{

    p->plotLayout()->insertRow(0);

#if QCUSTOMPLOT_VERSION < 0x020001

    // Earlier QCP versions:
    p->plotLayout()->addElement(0, 0, new QCPPlotTitle(p, title));
#else

    // QCP 2.0:
    p->plotLayout()->addElement(0, 0, new QCPTextElement(p, title, QFont("sans", 8, QFont::Bold)));
#endif


}

void GpsGui::setTimeAxis(QCPAxis *x)
{
    x->setRange(0, vecSize);
    x->setLabel("Time (relative units)");
}

void GpsGui::setPlotColors(QCustomPlot *p, bool dark)
{
    if(dark)
    {
        p->setBackground(QBrush(Qt::black));
        p->xAxis->setBasePen(QPen(Qt::yellow)); // lower line of axis
        p->yAxis->setBasePen(QPen(Qt::yellow)); // left line of axis
        p->xAxis->setTickPen(QPen(Qt::yellow));
        p->yAxis->setTickPen(QPen(Qt::yellow));
        p->xAxis->setLabelColor(QColor(Qt::white));
        p->yAxis->setLabelColor(QColor(Qt::white));
        p->graph()->setPen(QPen(Qt::red));
        //p->graph()->setBrush(QBrush(Qt::yellow)); // sets an underfill for the line
    } else {
        p->setBackground(QBrush(Qt::white));
        //p->graph()->setBrush(QBrush(Qt::black)); // sets an underfill for the line
    }
}

void GpsGui::updatePlots()
{
    // Called when there are new data

    ui->plotLatLong->graph(0)->setData(timeAxis, lats);
    ui->plotLatLong->graph(1)->setData(timeAxis, longs); // should be longs

    ui->plotAltitude->graph()->setData(timeAxis, alts);
    ui->plotHeading->graph(0)->setData(timeAxis, headings);
    ui->plotHeading->graph(1)->setData(timeAxis, courses);

    ui->plotPitch->graph()->setData(timeAxis, pitches);
    ui->plotRoll->graph()->setData(timeAxis, rolls);

    ui->plotSpeed->graph(0)->setData(timeAxis, nVelos);
    ui->plotSpeed->graph(1)->setData(timeAxis, eVelos);
    ui->plotSpeed->graph(2)->setData(timeAxis, upVelos);
    ui->plotSpeed->graph(3)->setData(timeAxis, groundVelos);

    ui->plotLatLong->replot();
    ui->plotAltitude->replot();
    ui->plotHeading->replot();
    ui->plotPitch->replot();
    ui->plotRoll->replot();
    ui->plotSpeed->replot();

}

unsigned char GpsGui::getBit(uint32_t d, unsigned char bit)
{
    //qDebug() << "d: " << d << " bit: " << bit << ", result: " << ((d & ( 1 << bit )) >> bit);
    return((d & ( 1 << bit )) >> bit);
}

void GpsGui::processGNSSInfo(int num)
{
    gnssInfo *g = &m.gnss[num-1];
    QString l;
    switch(g->gnssGPSQuality)
    {
        case gpsQualityInvalid:
            l="Invalid";
            break;
        case gpsQualityNatural_10m:
            l="Natural 10M";
            break;
        case gpsQualityDifferential_3m:
            l="Differential 3M";
            break;
        case gpsQualityMilitary_10m:
            l="Military 10M";
            break;
        case gpsQualityRTK_0p1m:
            l="RTK 0.1M";
            break;
        case gpsQualityFloatRTK_0p3m:
            l="Float RTK 0.3M";
            break;
        case gpsQualityOther:
            l="Other Invalid";
            break;
        default:
            l="Undefined";
            break;
    }
    ui->gpsQualityLabel->setText(l);
    if(num != 1)
    {
        qDebug() << "GNSS Quality for N=" << num << ": " << l;
        // 1: Internal GNSS
        // 2: Additional GNSS
        // 3: "Manual" GNSS
    }
}

void GpsGui::processStickyStatus()
{
    if(navStatusSticky)
        ui->navigationSticky->setState(QLedLabel::StateError);
    else
        ui->navigationSticky->setState(QLedLabel::StateOk);

    if(gpsReceivedSticky)
        ui->gpsReceivedSticky->setState(QLedLabel::StateError);
    else
        ui->gpsReceivedSticky->setState(QLedLabel::StateOk);

    if(gpsValidSticky)
        ui->gpsValidSticky->setState(QLedLabel::StateError);
    else
        ui->gpsValidSticky->setState(QLedLabel::StateOk);

    if(gpsWaitingSticky)
        ui->gpsWaitingSticky->setState(QLedLabel::StateError);
    else
        ui->gpsWaitingSticky->setState(QLedLabel::StateOk);

    if(gpsRejectedSticky)
        ui->gpsRejectedSticky->setState(QLedLabel::StateError);
    else
        ui->gpsRejectedSticky->setState(QLedLabel::StateOk);

    if(altitudeSaturationSticky)
        ui->altitudeSaturationSticky->setState(QLedLabel::StateError);
    else
        ui->altitudeSaturationSticky->setState(QLedLabel::StateOk);

    if(speedSaturationSticky)
        ui->speedSaturationSticky->setState(QLedLabel::StateError);
    else
        ui->speedSaturationSticky->setState(QLedLabel::StateOk);

    if(interpolationMissedSticky)
        ui->interpolationMissedSticky->setState(QLedLabel::StateError);
    else
        ui->interpolationMissedSticky->setState(QLedLabel::StateOk);

    if(systemReadySS3Sticky)
        ui->systemReadySS3Sticky->setState(QLedLabel::StateError);
    else
        ui->systemReadySS3Sticky->setState(QLedLabel::StateOk);

    if(systemReadySS3Sticky)
        ui->systemReadySS3Sticky->setState(QLedLabel::StateError);
    else
        ui->systemReadySS3Sticky->setState(QLedLabel::StateOk);

    if(gpsDetectedSS2Sticky)
        ui->gpsDetectedSS2Sticky->setState(QLedLabel::StateError);
    else
        ui->gpsDetectedSS2Sticky->setState(QLedLabel::StateOk);

    if(outputAFullSticky)
        ui->outputAFullSticky->setState(QLedLabel::StateError);
    else
        ui->outputAFullSticky->setState(QLedLabel::StateOk);

    if(outputBFullSticky)
        ui->outputBFullSticky->setState(QLedLabel::StateError);
    else
        ui->outputBFullSticky->setState(QLedLabel::StateOk);

    if(flashWriteErrorSticky)
        ui->flashWriteErrorSticky->setState(QLedLabel::StateError);
    else
        ui->flashWriteErrorSticky->setState(QLedLabel::StateOk);

    if(flashEraseErrorSticky)
        ui->flashEraseErrorSticky->setState(QLedLabel::StateError);
    else
        ui->flashEraseErrorSticky->setState(QLedLabel::StateOk);

}

void GpsGui::on_connectBtn_clicked()
{
    QString binaryLogFilename = ui->gpsBinLogEdit->text();
    if(binaryLogFilename.isEmpty())
    {
        binaryLogFilename = "/tmp/gpsbinary--DEFAULT--.log";
    }
    emit connectToGPS(ui->gpsHostEdit->text(), ui->gpsPort->text().toInt(), binaryLogFilename);
    gnssStatusTime.restart();
}

void GpsGui::on_disconnectBtn_clicked()
{
    emit disconnectFromGPS();
    firstMessage = true;
}

void GpsGui::on_debugBtn_clicked()
{
    gpsBinaryReader r;
    r.printMessage(m);
    //emit getDebugInfo();
}

void GpsGui::on_clearBtn_clicked()
{
    ui->logViewer->clear();
}

void GpsGui::handleGPSConnectionError(int errorNumber)
{
    ui->statusConnectionLED->setState(QLedLabel::StateError);
    ui->statusConnectedLabel->setText(QString("NG:%1").arg(errorNumber));
    ui->statusbar->showMessage(QString("GPS Connection Error: %1").arg(errorNumber), 5000);
}

void GpsGui::handleGPSConnectionGood()
{
    ui->statusConnectionLED->setState(QLedLabel::StateOk);
    ui->statusHeartbeatLED->setState(QLedLabel::StateOk);

    ui->statusConnectedLabel->setText("OK");
    ui->statusbar->showMessage("GPS Connection GOOD", 2000);
}

void GpsGui::handleGPSTimeout()
{
    ui->statusbar->showMessage("ERROR: GPS Data Timeout", 2000);
    ui->statusHeartbeatLED->setState(QLedLabel::StateError);
}

void GpsGui::handleErrorMessage(QString errorMessage)
{
    ui->logViewer->appendPlainText(errorMessage);
}

void GpsGui::on_clearErrorBtn_clicked()
{
    ui->statusConnectionLED->setState(QLedLabel::StateOk);
    ui->statusCounterLED->setState(QLedLabel::StateOk);
    ui->statusDecodeOkLED->setState(QLedLabel::StateOk);
    ui->statusSatelliteRxLED->setState(QLedLabel::StateOk);
    ui->statusHeartbeatLED->setState(QLedLabel::StateOk);

    navStatusSticky = false;
    gpsReceivedSticky = false;
    gpsValidSticky = false;
    gpsWaitingSticky = false;
    gpsRejectedSticky = false;
    altitudeSaturationSticky = false;
    speedSaturationSticky = false;
    interpolationMissedSticky = false;

    systemReadySS3Sticky = false;
    gpsDetectedSS2Sticky = false;
    outputAFullSticky = false;
    outputBFullSticky = false;
    flashWriteErrorSticky = false;
    flashEraseErrorSticky = false;

    processStickyStatus();

    firstMessage = true;

    gpsMessageHeartbeat.start();
    ui->statusbar->showMessage("NOTE: Resetting error and warning LEDs", 2000);
}

void GpsGui::on_selFileBtn_clicked()
{
    QString filename;
#ifdef __APPLE__
    filename = QFileDialog::getOpenFileName(this,
                                            tr("Open Binary GPS Log"), QDir::homePath(), tr("Log Files (*)"));
#else
    filename = QFileDialog::getOpenFileName(this,
                                            tr("Open Binary GPS Log"), "/home", tr("Log Files (*)"));
#endif

    if(!filename.isEmpty())
    {
        ui->gpsBinLogOpenEdit->setText(filename);
        emit setBinaryLogReplayFilename(ui->gpsBinLogOpenEdit->text());
    }
}

void GpsGui::resetLEDs()
{
    ui->statusHeartbeatLED->setState(QLedLabel::StateOk);
    on_clearErrorBtn_clicked();
    ui->gpsAlignment->setText("Unknown");
    ui->gpsAlignment->setToolTip("Status not yet received.");
    ui->navStatus->setState(QLedLabel::StateOkBlue);
    ui->gpsReceivedStatus->setState(QLedLabel::StateOkBlue);
    ui->gpsValidStatus->setState(QLedLabel::StateOkBlue);
    ui->gpsWaitingStatus->setState(QLedLabel::StateOkBlue);
    ui->gpsRejectedStatus->setState(QLedLabel::StateOkBlue);
    ui->altitudeSaturationStatus->setState(QLedLabel::StateOkBlue);
    ui->speedSaturationStatus->setState(QLedLabel::StateOkBlue);

    ui->systemReadySS3Status->setState(QLedLabel::StateOkBlue);
    ui->gpsDetectedSS2Status->setState(QLedLabel::StateOkBlue);
    ui->interpolationMissedStatus->setState(QLedLabel::StateOkBlue);
    ui->outputAFullStatus->setState(QLedLabel::StateOkBlue);
    ui->outputBFullStatus->setState(QLedLabel::StateOkBlue);
    ui->flashEraseErrorStatus->setState(QLedLabel::StateOkBlue);
    ui->flashWriteErrorStatus->setState(QLedLabel::StateOkBlue);

    ui->gpsQualityLabel->setText("UNKNOWN");
}

void GpsGui::on_replayGPSBtn_clicked()
{
    //emit setBinaryLogReplayFilename(ui->gpsBinLogOpenEdit->text());
    resetLEDs();
    firstMessage = true;

    qDebug() << "Starting replay of file:" << ui->gpsBinLogOpenEdit->text();
    emit startGPSReplay();
}

void GpsGui::on_gpsBinLogOpenEdit_editingFinished()
{
    if(!ui->gpsBinLogOpenEdit->text().isEmpty())
    {
        emit setBinaryLogReplayFilename(ui->gpsBinLogOpenEdit->text());
    }
}

void GpsGui::on_stopReplayBtn_clicked()
{
    fileReader->keepGoing = false;
    qDebug() << "Stopping log replay.";
    emit stopGPSReplay();
}

void GpsGui::on_showMapBtn_clicked()
{
    map->show();
}

void GpsGui::on_startSecondLogBtn_clicked()
{
    if(!ui->secondaryLogEdit->text().isEmpty())
    {
        emit startSecondaryLog(ui->secondaryLogEdit->text());
    }
}

void GpsGui::on_stopSecondLogBtn_clicked()
{
    emit stopSecondaryLog();
}

void GpsGui::on_speedupFactorSpin_valueChanged(int arg1)
{
    if(arg1 > 0)
    {
        fileReader->messageDelayMicroSeconds = (1E6 / 200.0) / arg1;
        emit setGPSReplaySpeedupFactor(arg1);
    }
}

void GpsGui::on_replayEnabledChk_toggled(bool checked)
{
    fileReader->paused = !checked;
}

void GpsGui::on_replayEnabledMainChk_toggled(bool checked)
{
    fileReader->paused = !checked;
}
