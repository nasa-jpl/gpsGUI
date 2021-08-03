#include "gpsgui.h"
#include "ui_gpsgui.h"

GpsGui::GpsGui(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GpsGui)
{
    ui->setupUi(this);

    vecSize = 450;
    msgsReceivedCount = 0;

    headings.resize(vecSize);
    rolls.resize(vecSize);
    pitches.resize(vecSize);
    lats.resize(vecSize);
    longs.resize(vecSize);
    alts.resize(vecSize);
    nVelos.resize(vecSize);
    eVelos.resize(vecSize);
    upVelos.resize(vecSize);
    timeAxis.resize(vecSize);

    preparePlots();

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
    connect(this, SIGNAL(setBinaryLogFilename(QString)), gps, SLOT(setBinaryLoggingFilename(QString)));
    gpsThread->start();

    connect(this, SIGNAL(setBinaryLogReplayFilename(QString)), fileReader, SLOT(setFilename(QString)));
    connect(fileReader, SIGNAL(haveErrorMessage(QString)), this, SLOT(handleErrorMessage(QString)));
    connect(fileReader, SIGNAL(haveStatusMessage(QString)), this, SLOT(handleGPSStatusMessage(QString)));
    connect(fileReader, SIGNAL(haveGPSMessage(gpsMessage)), this, SLOT(receiveGPSMessage(gpsMessage)));
    connect(this, SIGNAL(startGPSReplay()), fileReader, SLOT(beginWork()));
    connect(this, SIGNAL(stopGPSReplay()), fileReader, SLOT(stopWork()));
    replayThread->start();

    ui->gpsPort->setValidator( new QIntValidator(0,65535,this) );

    gpsMessageHeartbeat.setInterval(500); // half second, expected is 5ms.

    connect(&gpsMessageHeartbeat, SIGNAL(timeout()), this, SLOT(handleGPSTimeout()));

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

    if(m.validDecode)
    {
        this->m = m;
    } else {
        ui->statusDecodeOkLED->setState(QLedLabel::StateError);
        ui->statusDecodeOkLabel->setText("NG");
        return;
    }

    msgsReceivedCount++;

    if(firstMessage)
    {
        gnssStatusTime.start();
        ui->statusSatelliteRxLED->setState(QLedLabel::StateOk);
        firstMessage = false;
    }

    gpsMessageHeartbeat.start();

    if(m.haveGNSSInfo1 || m.haveGNSSInfo2 || m.haveGNSSInfo3)
    {
        gnssStatusTime.restart();
    } else {
        if(gnssStatusTime.elapsed() > 5*1000)
        {
            ui->statusSatelliteRxLED->setState(QLedLabel::StateError);
        } else if (gnssStatusTime.elapsed() > 2*1000)
        {
            // Expected interval is every 1 second.
            ui->statusSatelliteRxLED->setState(QLedLabel::StateWarning);
        }
    }



    bool doPlotUpdate = (msgsReceivedCount%40)==0;
    bool doWidgetPaint = (msgsReceivedCount%10)==0;
    bool doLabelUpdate = (msgsReceivedCount%10)==0;

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
        ui->EHSI->setCourse(m.courseOverGround);
        ui->EADI->setAirspeed(m.speedOverGround);
        ui->airSpeedIndicator->setAirspeed(m.speedOverGround * 1.94384); // 1 meter per second = 1.94384 knots
        if(doLabelUpdate)
        {
            ui->groundSpeedDataLabel->setText(QString("%1").arg(m.speedOverGround * 1.94384));
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
        // Note: This GPS device converts GPST to UTC for us.
        // Otherwise, we would need to account for the differences in the various GNSS satellite systems, which do not all follow GPST or UTC.

        // The units are in 100 micro-seconds.
        uint64_t t = m.UTCdataValidityTime;
        int hour = t / ((float)1E4)/60.0/60.0;
        int minute = ( t / ((float)1E4)/60.0 ) - (hour*60) ;
        int second = ( t / ((float)1E4) ) - (hour*60.0*60.0) - (minute*60.0);

        QString time = QString("%1:%2:%3 UTC").arg(hour, 2, 10, QChar('0')).arg(minute, 2, 10, QChar('0')).arg(second, 2, 10, QChar('0'));

        ui->utcTimeLabel->setText(time);
        //qDebug() << "UTC time received: " << t << " (100 micro-seconds) hours: " << hour << " minute: " << minute << " second: " << second << " string: " << time;

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
    ui->plotHeading->addGraph();

    // Y-Axis Range:
    //ui->plotLatLong->yAxis->setRange(-90*1.1, 90*1.1); // Lat
    ui->plotLatLong->yAxis->setRange(-360*1.25, 360*1.25); // Lat

    //ui->plotLatLong->yAxis2->setRange(0, 360*1.25); // Long
    ui->plotLatLong->addGraph(ui->plotLatLong->xAxis, ui->plotLatLong->yAxis ); // Lat
    //ui->plotLatLong->addGraph(ui->plotLatLong->yAxis2, ui->plotLatLong->xAxis ); // Long
    ui->plotLatLong->addGraph(ui->plotLatLong->xAxis, ui->plotLatLong->yAxis ); // Long



    ui->plotAltitude->yAxis->setRange(0, 7620); // Altitude at 25k feet

    ui->plotSpeed->yAxis->setRange(-128, 128); // 128 meters per second is 250 knots

    ui->plotPitch->yAxis->setRange(-90*1.1, 90*1.1);
    ui->plotRoll->yAxis->setRange(-180*1.1, 180*1.1);
    ui->plotHeading->yAxis->setRange(0, 360*1.1); // raw data is 0-360 but we will show it like this.

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
    setPlotTitle(ui->plotHeading, QString("Magnetic Heading"));

    // set labels:

    // colors:
    ui->plotLatLong->graph(0)->setPen(QPen(Qt::blue));
    ui->plotLatLong->graph(1)->setPen(QPen(Qt::red));

    ui->plotSpeed->graph(0)->setPen(QPen(Qt::red));
    ui->plotSpeed->graph(1)->setPen(QPen(Qt::blue));
    ui->plotSpeed->graph(2)->setPen(QPen(Qt::yellow));
    ui->plotSpeed->graph(3)->setPen(QPen(Qt::green));

    setPlotColors(ui->plotHeading, true);

}

void GpsGui::setPlotTitle(QCustomPlot *p, QString title)
{

    p->plotLayout()->insertRow(0);
    p->plotLayout()->addElement(0, 0, new QCPTextElement(p, title, QFont("sans", 8, QFont::Bold)));
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
    ui->plotHeading->graph()->setData(timeAxis, headings);
    ui->plotPitch->graph()->setData(timeAxis, pitches);
    ui->plotRoll->graph()->setData(timeAxis, rolls);

    ui->plotSpeed->graph(0)->setData(timeAxis, nVelos);
    ui->plotSpeed->graph(1)->setData(timeAxis, eVelos);
    ui->plotSpeed->graph(2)->setData(timeAxis, upVelos);

    ui->plotLatLong->replot();
    ui->plotAltitude->replot();
    ui->plotHeading->replot();
    ui->plotPitch->replot();
    ui->plotRoll->replot();
    ui->plotSpeed->replot();

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
    gpsMessageHeartbeat.start();
    ui->statusbar->showMessage("NOTE: Resetting error and warning LEDs", 2000);
}

void GpsGui::on_selFileBtn_clicked()
{
    QString filename;
    filename = QFileDialog::getOpenFileName(this,
        tr("Open Binary GPS Log"), "/home", tr("Log Files (*.txt *.log *.bin)"));
    if(!filename.isEmpty())
    {
        ui->gpsBinLogOpenEdit->setText(filename);
        emit setBinaryLogReplayFilename(ui->gpsBinLogOpenEdit->text());
    }
}

void GpsGui::on_replayGPSBtn_clicked()
{
    //emit setBinaryLogReplayFilename(ui->gpsBinLogOpenEdit->text());
    ui->statusHeartbeatLED->setState(QLedLabel::StateOk);
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
    emit stopGPSReplay();
}
