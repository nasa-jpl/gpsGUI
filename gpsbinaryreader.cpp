#include "gpsbinaryreader.h"

gpsBinaryReader::gpsBinaryReader()
{
    oldCounter = 0;
    firstRun = true;
    initialize();
}

gpsBinaryReader::gpsBinaryReader(QByteArray rawData) : rawData(rawData)
{
    initialize();
    oldCounter = 0;
    firstRun = true;
    memset(&m, 0x0, sizeof(gpsMessage));
    if(rawData.length())
        processData();
}

void gpsBinaryReader::insertData(QByteArray rawData)
{
    initialize();
    this->rawData = rawData;
    memset(&m, 0x0, sizeof(gpsMessage));
    if(rawData.length())
        processData();
    oldCounter = m.counter;
}

gpsMessage gpsBinaryReader::getMessage()
{
    if(!m.validDecode)
    {
        qDebug() << "Warning, returning invalid data with counter " << m.counter;
    }
    return m;
}

void gpsBinaryReader::initialize()
{
    decodeInvalid = true;
    dataPos = 0;
    gpsMessage blank;
    m = blank; // copy
    m.counter = 0;
    // todo: clear out m. m.HaveThings = false;
}

// Decoder functions:

void gpsBinaryReader::processData()
{
    mtx.lock();

    copyQStringToCharArray( m.lastDecodeErrorMessage, QString("NONE") );
    detMsgType();
    m.protoVers = rawData.at(2);

    bool foundErrors = false;
    m.validDecode = false;
    decodeInvalid = true;

    if(m.mType != msgIX_outputNav)
    {
        m.validDecode = false;
        decodeInvalid = true;
        copyQStringToCharArray( m.lastDecodeErrorMessage, QString("UNSUPPORTED MSG TYPE") );
        mtx.unlock();
        return;
    }

    dataPos = 3;

    switch(m.protoVers)
    {
    case 2:
        processNavOutHeaderV2();
        break;
    case 3:
        processNavOutHeaderV3();
        break;
    case 5:
        processNavOutHeaderV5();
        break;
    default:
        // Invalid data
        m.validDecode = false;
        decodeInvalid = true;
        copyQStringToCharArray( m.lastDecodeErrorMessage, QString("UNK PROTO  ") );
        mtx.unlock();
        return;
        break;
    }

    // These must be called in a specific order,
    // since the offset into the message is dependant upon
    // how many blocks of data have been read.

    if(getBit(m.navDataBlockBitmask, 0))
    {
        //qDebug() << "Have Altitude and Heading Data";
        processAltitudeHeadingData();
    }
    if(getBit(m.navDataBlockBitmask, 1))
    {
        //qDebug() << "Have Alt and Head Std Dev";
        processAltitudeHeadingStdDevData();
    }
    // Mystery data at bits 2 and 3:
    if(getBit(m.navDataBlockBitmask, 2))
    {
        processRealTimeHeaveSurgeSwayData();
    }
    if(getBit(m.navDataBlockBitmask, 3))
    {
        processSmartHeaveData();
    }

    if(getBit(m.navDataBlockBitmask, 4))
    {
        //qDebug() << "Have Heading RollPitchRate";
        processHeadingRollPitchRateData();
    }
    if(getBit(m.navDataBlockBitmask, 5))
    {
        //qDebug() << "Have Body Rotation Rate";
        processBodyRotationRate();
    }
    if(getBit(m.navDataBlockBitmask, 6))
    {
        //qDebug() << "Have acceleration data";
        processAccelerationData();
    }
    if(getBit(m.navDataBlockBitmask, 7))
    {
        //qDebug() << "Have Position Data!";
        processPositionData();
    }
    if(getBit(m.navDataBlockBitmask, 8))
    {
        //qDebug() << "Have Position StdDev Data!";
        processPositionStdDevData();
    }
    if(getBit(m.navDataBlockBitmask, 9))
    {
        //qDebug() << "Have Speed Data!";
        processSpeedData();
    }
    if(getBit(m.navDataBlockBitmask, 10))
    {
        //qDebug() << "Have Speed Standard Deviation Data";
        processSpeedStdDevData();
    }
    if(getBit(m.navDataBlockBitmask, 11))
    {
        //qDebug() << "Have ocean current data";
        processCurrentData();
    }
    if(getBit(m.navDataBlockBitmask, 12))
    {
        //qDebug() << "Have ocean current std dev data";
        processCurrentStdDevData();
    }
    if(getBit(m.navDataBlockBitmask, 13))
    {
        //qDebug() << "Have System Date data";
        processSystemDateData();
    }
    if(getBit(m.navDataBlockBitmask, 14 ))
    {
        //qDebug() << "Have INS Sensor Status";
        processINSSensorStatus();
    }
    if(getBit(m.navDataBlockBitmask, 15))
    {
        //qDebug() << "Have INS Algorithm Status";
        processINSAlgorithmStatus();
    }
    if(getBit(m.navDataBlockBitmask, 16))
    {
        //qDebug() << "Have INS System Status";
        processINSSystemStatus();
    }
    if(getBit(m.navDataBlockBitmask, 17))
    {
        //qDebug() << "Have INS User Status";
        processINSUserStatus();
    }

    // Mystery data at Bit 21:
    if(getBit(m.navDataBlockBitmask, 21))
    {
        //qDebug() << "Have Mystery Data at Nav Bit 21!";
        processHeaveSurgeSwaySpeed();
    }

    if(getBit(m.navDataBlockBitmask, 22))
    {
        //qDebug() << "Have vessel speed data";
        processVesselVelocity();
    }
    if(getBit(m.navDataBlockBitmask, 23))
    {
        //qDebug() << "Have accel geographic frame";
        processAccelGeographic();
    }
    if(getBit(m.navDataBlockBitmask, 24))
    {
        //qDebug() << "Have course and speed over ground";
        processCourseSpeedGround();
    }
    if(getBit(m.navDataBlockBitmask, 25))
    {
        //qDebug() << "Have Temperatures (of system)";
        processTemperature();
    }
    if(getBit(m.navDataBlockBitmask, 26))
    {
        //qDebug() << "Have attitude quaternion";
        processAttitudeQuaternion();
    }
    if(getBit(m.navDataBlockBitmask, 27))
    {
        //qDebug() << "Have attitude quaternion std dev";
        processAttitudeQuaternationStdDev();
    }
    if(getBit(m.navDataBlockBitmask, 28))
    {
        //qDebug() << "Have raw accel in vessel frame";
        processVesselAccel();
    }
    if(getBit(m.navDataBlockBitmask, 29))
    {
        //qDebug() << "Have acceleration std dev in vessel frame";
        processVesselAccelStdDev();
    }
    if(getBit(m.navDataBlockBitmask, 30))
    {
        //qDebug() << "Have vessel rotation rate std dev";
        processVesselRotationRateStdDev();
    }


    // exteNDED Nav Data Blocks:

    if( (m.extendedNavDataBlockBitmask != 0x00000007) && ( m.extendedNavDataBlockBitmask!= 0x00000000) )
    {
        foundErrors = true;
        decodeInvalid = true;
        m.validDecode = false;
        copyQStringToCharArray( m.lastDecodeErrorMessage, QString("UNK Extended Nav Data  ") );
        qDebug() << "WARNING: Invalid decode at count " << m.counter << ", extended nav data found: " << QString("0x%1").arg(m.extendedNavDataBlockBitmask, 8, 16, QChar('0'));
    }

    if(getBit(m.extendedNavDataBlockBitmask, 0))
    {
        processExtendedRotationAccelData();
    }
    if(getBit(m.extendedNavDataBlockBitmask, 1))
    {
        processExtendedRotationAccelStdDevData();
    }
    if(getBit(m.extendedNavDataBlockBitmask, 2))
    {
        processExtendedRawRotationRateData();
    }


    // exteRNAL sensor data blocks.

    if(getBit(m.externDataBitMask, 0))
    {
        // UTC
        processUTC();
    }
    if(getBit(m.externDataBitMask, 1))
    {
        // GNSS 1 status
        processGNSS(1);
        m.haveGNSSInfo1 = true;
    }
    if(getBit(m.externDataBitMask, 2))
    {
        // GNSS 2 status
        processGNSS(2);
        m.haveGNSSInfo2 = true;
    }
    if(getBit(m.externDataBitMask, 3))
    {
        // GNSS Manual status "3"
        processGNSS(3);
        m.haveGNSSInfo3 = true;
    }



    if(firstRun)
    {
        firstRun = false;
    } else {
        if(oldCounter+1 != m.counter)
        {
            qDebug() << __PRETTY_FUNCTION__ << "Warning: Dropped " << m.counter-oldCounter << " GPS messages at counter " << m.counter;
            m.numberDropped = m.counter-oldCounter;
        } else {
            m.numberDropped = 0;
        }
    }

    getMessageSum();

    uint32_t oldPos = dataPos; // retain previous position in case it is needed later.
    m.claimedMessageSum = makeDWord(rawData, rawData.length() - 4);

    (void)oldPos;

    if(m.claimedMessageSum != messageSum)
    {
        copyQStringToCharArray( m.lastDecodeErrorMessage, QString("BAD Checksum  ") );
        qDebug() << "Warning: invalid checksum in message with counter " << m.counter << ", message checksum: " << m.claimedMessageSum << ", calculated checksum: " << messageSum;
        decodeInvalid = true;
        m.validDecode = false;
        foundErrors = true;
    }

    // A little debug checking:
//    if(m.navDataBlockBitmask != 0x7fe3ffff)
//    {
//        qDebug() << "Navigation data block bitmask: counter = " << m.counter;
//        printBinary(m.navDataBlockBitmask);
//    }

//    if(m.extendedNavDataBlockBitmask != 0x00000007)
//    {
//        qDebug() << "Extended data block bitmask: counter = " << m.counter;
//        printBinary(m.extendedNavDataBlockBitmask);
//    }

//    if(m.externDataBitMask != 0)
//    {
//        qDebug() << "External sensor bitmask: counter = " << m.counter;
//        printBinary(m.externDataBitMask);
//        printMessage();

//    }

    // HERE is where we place the final Good / No-Good into the message:
    if(foundErrors)
    {
        qDebug() << "Found errors at counter: " << m.counter;
        m.validDecode = false;
        decodeInvalid = true;
    } else {
        //qDebug() << "Did not find errors at counter: " << m.counter;
        m.validDecode = true;
        decodeInvalid = false;
    }

    // DEBUG for AV3 testing:
    if(m.haveINSAlgorithmStatus)
    {
        if(priorAlgorithmStatus1 != m.algorithmStatus1)
        {
            //qDebug() << "Changed algorithmStatus bits.";
            //printAlgorithmStatusMessages(m);
            priorAlgorithmStatus1 = m.algorithmStatus1;
        }
    }

    mtx.unlock();

}

void gpsBinaryReader::detMsgType()
{
    unsigned int messageType = 0;

    messageType = rawData.at(0) | (rawData.at(1) << 8);

    switch(messageType)
    {
    case 'C'|('M' << 8):
        // CM = input message
        m.mType = msgCM_input;
        break;
    case 'A'|('N' << 8):
        // AN = output message, answer to query.
        m.mType = msgAN_outputAnswer;
        break;
    case 'I'|('X' << 8):
        // IX = Output Message, Navigation data
        m.mType = msgIX_outputNav;
        break;
    default:
        // unknown
        m.mType = msgUnknown;
        break;
    }
}

void gpsBinaryReader::processNavOutHeaderV2()
{
    m.navDataBlockBitmask = makeDWord(rawData, dataPos);
    m.externDataBitMask = makeDWord(rawData, dataPos);
    m.totalTelegramSize = makeWord(rawData, dataPos);
    m.navDataValidityTime = makeDWord(rawData, dataPos);
    m.counter = makeDWord(rawData, dataPos);
}

void gpsBinaryReader::processNavOutHeaderV3()
{
    m.navDataBlockBitmask = makeDWord(rawData, dataPos);
    m.extendedNavDataBlockBitmask = makeDWord(rawData, dataPos);
    m.externDataBitMask = makeDWord(rawData, dataPos);

    m.totalTelegramSize = makeWord(rawData, dataPos);
    m.navDataValidityTime = makeDWord(rawData, dataPos);
    m.counter = makeDWord(rawData, dataPos);
}

void gpsBinaryReader::processNavOutHeaderV5()
{
    m.navDataBlockBitmask = makeDWord(rawData, dataPos);
    m.extendedNavDataBlockBitmask = makeDWord(rawData, dataPos);
    m.externDataBitMask = makeDWord(rawData, dataPos);
    m.navigationDataSize = makeWord(rawData, dataPos);
    m.totalTelegramSize = makeWord(rawData, dataPos);
    m.navDataValidityTime = makeDWord(rawData, dataPos);
    m.counter = makeDWord(rawData, dataPos);
    //qDebug() << "Counter: " << m.counter;
}

void gpsBinaryReader::processAltitudeHeadingData()
{
    // Bit 0
    //qDebug() << "alt heading data at pos: " << dataPos;
    m.heading = makeFloat(rawData, dataPos);
    m.roll = makeFloat(rawData, dataPos);
    m.pitch = makeFloat(rawData, dataPos);
    m.haveAltitudeHeading = true;
}

void gpsBinaryReader::processAltitudeHeadingStdDevData()
{
    // Bit 1
    //qDebug() << "alt heading data stddev at pos: " << dataPos;
    m.headingStardardDeviation = makeFloat(rawData, dataPos);
    m.rollStandardDeviation = makeFloat(rawData, dataPos);
    m.pitchStandardDeviation = makeFloat(rawData, dataPos);
    m.haveAltitudeHeadingStdDev = true;
}

void gpsBinaryReader::processRealTimeHeaveSurgeSwayData()
{
    // Bit 2, RealTimeHeaveSurgeSway data
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.rt_heave_withoutBdL = makeFloat(rawData, dataPos); /*! Meters - positive UP in horizontal vehicle frame */
    m.rt_heave_atBdL = makeFloat(rawData, dataPos);      /*! Meters - positive UP in horizontal vehicle frame */
    m.rt_surge_atBdL = makeFloat(rawData, dataPos); /*! Meters - positive FORWARD in horizontal vehicle frame */
    m.rt_sway_atBdL = makeFloat(rawData, dataPos);  /*! Meters - positive PORT SIDE in horizontal vehicle frame */
    m.haveRealTimeHeaveSurgeSwayData = true;
}

void gpsBinaryReader::processSmartHeaveData()
{
    // Bit 3, Smart Heave
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.smartHeaveValidityTime_100us = makeDWord(rawData, dataPos);
    m.smartHeave_m = makeDWord(rawData, dataPos);
    m.haveSmartHeaveData = true;
}

void gpsBinaryReader::processHeadingRollPitchRateData()
{
    // Bit 4
    //qDebug() << "heading roll pitch rate at pos: " << dataPos;
    m.headingRotationRate = makeFloat(rawData, dataPos);
    m.rollRotationRate = makeFloat(rawData, dataPos);
    m.pitchRotationRate = makeFloat(rawData, dataPos);
    m.haveHeadingRollPitchRate = true;
}

void gpsBinaryReader::processBodyRotationRate()
{
    // Bit 5
    //qDebug() << "heading roll pitch rotation rate at pos: " << dataPos;
    m.rotationRateXV1 = makeFloat(rawData, dataPos);
    m.rotationRateXV2 = makeFloat(rawData, dataPos);
    m.rotationRateXV3 = makeFloat(rawData, dataPos);
    m.haveBodyRotationRate = true;
}

void gpsBinaryReader::processAccelerationData()
{
    // Bit 6
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.accelXV1 = makeFloat(rawData, dataPos);
    m.accelXV2 = makeFloat(rawData, dataPos);
    m.accelXV3 = makeFloat(rawData, dataPos);
    m.haveAccel = true;
}

void gpsBinaryReader::processPositionData()
{
    // Bit 7
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.latitude = makeDouble(rawData, dataPos);
    m.longitude = makeDouble(rawData, dataPos);
    //qDebug() << "Lat: " << m.latitude << ", long: " << m.longitude;
    m.altitudeReference = rawData.at(dataPos); dataPos++;
    m.altitude = makeFloat(rawData, dataPos);
    m.havePosition = true;
}

void gpsBinaryReader::processPositionStdDevData()
{
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.northStdDev = makeFloat(rawData, dataPos);
    m.eastStdDev = makeFloat(rawData, dataPos);
    m.neCorrelation = makeFloat(rawData, dataPos);
    m.altitudStdDev = makeFloat(rawData, dataPos);
    m.havePositionStdDev = true;
}

void gpsBinaryReader::processSpeedData()
{
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.northVelocity = makeFloat(rawData, dataPos);
    m.eastVelocity = makeFloat(rawData, dataPos);
    m.upVelocity = makeFloat(rawData, dataPos);
    m.haveSpeedData = true;
}

void gpsBinaryReader::processSpeedStdDevData()
{
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.northVelocityStdDev = makeFloat(rawData, dataPos);
    m.eastVelocityStdDev = makeFloat(rawData, dataPos);
    m.upVelocityStdDev = makeFloat(rawData, dataPos);
    m.haveSpeedStdDev = true;
}

void gpsBinaryReader::processCurrentData()
{
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.northCurrent = makeFloat(rawData, dataPos);
    m.eastCurrent = makeFloat(rawData, dataPos);
    m.haveCurrentData = true;
}

void gpsBinaryReader::processCurrentStdDevData()
{
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.northCurrentStdDev = makeFloat(rawData, dataPos);
    m.eastCurrentStdDev = makeFloat(rawData, dataPos);
    m.haveCurrentData = true;
}

void gpsBinaryReader::processSystemDateData()
{
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.systemDay = (unsigned char)rawData.at(dataPos); dataPos++;
    m.systemMonth = (unsigned char)rawData.at(dataPos); dataPos++;
    m.systemYear = makeWord(rawData, dataPos);
    m.haveSystemDateData = true;
}

void gpsBinaryReader::processINSSensorStatus()
{
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.insSensorStatus1 = makeDWord(rawData, dataPos);
    m.insSensorStatus2 = makeDWord(rawData, dataPos);
    m.haveINSSensorStatus = true;
}

void gpsBinaryReader::processINSAlgorithmStatus()
{
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.algorithmStatus1 = makeDWord(rawData, dataPos);
    m.algorithmStatus2 = makeDWord(rawData, dataPos);
    m.algorithmStatus3 = makeDWord(rawData, dataPos);
    m.algorithmStatus4 = makeDWord(rawData, dataPos);
    m.haveINSAlgorithmStatus = true;
}

void gpsBinaryReader::processINSSystemStatus()
{
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.systemStatus1 = makeDWord(rawData, dataPos);
    m.systemStatus2 = makeDWord(rawData, dataPos);
    m.systemStatus3 = makeDWord(rawData, dataPos);
    m.haveINSSystemStatus = true;
}

void gpsBinaryReader::processINSUserStatus()
{
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.INSuserStatus = makeDWord(rawData, dataPos);
    m.haveINSUserStatus = true;
}

void gpsBinaryReader::processHeaveSurgeSwaySpeed()
{
    // Bit 21:
    m.realtime_heave_speed = makeFloat(rawData, dataPos);
    m.surge_speed = makeFloat(rawData, dataPos);
    m.sway_speed = makeFloat(rawData, dataPos);
    m.haveHeaveSurgeSwaySpeedData = true;
}

void gpsBinaryReader::processVesselVelocity()
{
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.vesselXV1Velocity = makeFloat(rawData, dataPos);
    m.vesselXV2Velocity = makeFloat(rawData, dataPos);
    m.vesselXV3Velocity = makeFloat(rawData, dataPos);
    m.haveSpeedVesselData = true;
}

void gpsBinaryReader::processAccelGeographic()
{
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.geographicNorthAccel = makeFloat(rawData, dataPos);
    m.geographicEastAccel = makeFloat(rawData, dataPos);
    m.geographicVertAccel = makeFloat(rawData, dataPos);
    m.haveAccelGeographicData = true;
}

void gpsBinaryReader::processCourseSpeedGround()
{
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.courseOverGround = makeFloat(rawData, dataPos);
    m.speedOverGround = makeFloat(rawData, dataPos);
    m.haveCourseSpeedGroundData = true;
}

void gpsBinaryReader::processTemperature()
{
    //qDebug() << __PRETTY_FUNCTION__ << dataPos;
    m.meanTempFOG = makeFloat(rawData, dataPos);
    m.meanTempACC = makeFloat(rawData, dataPos);
    m.meanTempSensor = makeFloat(rawData, dataPos);
    m.haveTempData = true;
    //qDebug() << "FOG temp: " << m.meanTempFOG << ", ACC temp: " << m.meanTempACC << ", Sensor temp: " << m.meanTempSensor;
}

void gpsBinaryReader::processAttitudeQuaternion()
{
    m.attitudeQCq0 = makeFloat(rawData, dataPos);
    m.attitudeQCq1 = makeFloat(rawData, dataPos);
    m.attitudeQCq2 = makeFloat(rawData, dataPos);
    m.attitudeQCq3 = makeFloat(rawData, dataPos);
    m.haveAttitudeQuaternionData = true;
}

void gpsBinaryReader::processAttitudeQuaternationStdDev()
{
    m.attitudeQE1 = makeFloat(rawData, dataPos);
    m.attitudeQE2 = makeFloat(rawData, dataPos);
    m.attitudeQE3 = makeFloat(rawData, dataPos);
    m.haveAttitudeQEData = true;
}

void gpsBinaryReader::processVesselAccel()
{
    m.vesselAccelXV1 = makeFloat(rawData, dataPos);
    m.vesselAccelXV2 = makeFloat(rawData, dataPos);
    m.vesselAccelXV3 = makeFloat(rawData, dataPos);
    m.haveVesselAccel = true;
}

void gpsBinaryReader::processVesselAccelStdDev()
{
    m.vesselAccelXV1StdDev = makeFloat(rawData, dataPos);
    m.vesselAccelXV2StdDev = makeFloat(rawData, dataPos);
    m.vesselAccelXV3StdDev = makeFloat(rawData, dataPos);
    m.haveVesselAccelStdDev = true;
}

void gpsBinaryReader::processVesselRotationRateStdDev()
{
    m.vesselRotationRateXV1StdDev = makeFloat(rawData, dataPos);
    m.vesselRotationRateXV2StdDev = makeFloat(rawData, dataPos);
    m.vesselRotationRateXV3StdDev = makeFloat(rawData, dataPos);
    m.haveVesselRotationRateStdDev = true;
}

void gpsBinaryReader::processExtendedRotationAccelData()
{
    m.rawRotationAccelXV1 = makeFloat(rawData, dataPos);
    m.rawRotationAccelXV2 = makeFloat(rawData, dataPos);
    m.rawRotationAccelXV3 = makeFloat(rawData, dataPos);
    m.haveExtendedRawRotationAccelData = true;
}

void gpsBinaryReader::processExtendedRotationAccelStdDevData()
{
    m.rawRotationAccelStdDevXV1 = makeFloat(rawData, dataPos);
    m.rawRotationAccelStdDevXV2 = makeFloat(rawData, dataPos);
    m.rawRotationAccelStdDevXV3 = makeFloat(rawData, dataPos);
    m.haveExtendedRawRotationAccelStdDevData = true;
}

void gpsBinaryReader::processExtendedRawRotationRateData()
{
    m.rawRotationRateXV1 = makeFloat(rawData, dataPos);
    m.rawRotationRateXV2 = makeFloat(rawData, dataPos);
    m.rawRotationRateXV3 = makeFloat(rawData, dataPos);
    m.haveExtendedRawRotationRateData = true;
}

void gpsBinaryReader::processUTC()
{
    m.UTCdataValidityTime = makeDWord(rawData, dataPos);
    m.UTCSource = (unsigned char)rawData.at(dataPos); dataPos++;
    m.haveUTC = true;
}

void gpsBinaryReader::processGNSS(int num)
{
    gnssInfo *g = &m.gnss[num-1];

    g->gnssDataValidityTime = makeLong(rawData, dataPos);
    g->gnssIdentification = (unsigned char)rawData.at(dataPos); dataPos++;
    g->gnssQuality = (unsigned char)rawData.at(dataPos); dataPos++;
    g->gnssGPSQuality = static_cast<gpsQualityKinds>(g->gnssQuality);
    g->gnssLatitude = makeDouble(rawData, dataPos);
    g->gnssLongitude = makeDouble(rawData, dataPos);
    g->gnssAltitude = makeFloat(rawData, dataPos);
    g->gnssLatStdDev = makeFloat(rawData, dataPos);
    g->gnssLongStddev = makeFloat(rawData, dataPos);
    g->gnssAltStdDev = makeFloat(rawData, dataPos);
    g->LatLongCovariance = makeFloat(rawData, dataPos);
    g->geoidalSep = makeFloat(rawData, dataPos);

    g->populated = true;
}


// Helper functions:

unsigned char gpsBinaryReader::getBit(uint32_t d, unsigned char bit)
{
    //qDebug() << "d: " << d << " bit: " << bit << ", result: " << ((d & ( 1 << bit )) >> bit);
    return((d & ( 1 << bit )) >> bit);
}
// startPos is the MSB
dword gpsBinaryReader::makeDWord(QByteArray d, uint16_t startPos)
{
    // Unsigned 32-Bit Int
    dataPos +=4;
    return (unsigned char)d.at(startPos+3) | ((unsigned char)d.at(startPos+2) << 8) | ((unsigned char)d.at(startPos+1) << 16) | ((unsigned char)d.at(startPos+0) << 24);
}

word gpsBinaryReader::makeWord(QByteArray d, uint16_t startPos)
{
    // Unsigned 16-Bit Int
    dataPos +=2;
    return (unsigned char)d.at(startPos+1) | ((unsigned char)d.at(startPos+0) << 8);
}

short gpsBinaryReader::makeShort(QByteArray d, uint16_t startPos)
{
    // Signed 16-Bit Int
    dataPos +=2;
    return (unsigned char)d.at(startPos+1) | ((unsigned char)d.at(startPos+0) << 8);
}

long gpsBinaryReader::makeLong(QByteArray d, uint16_t startPos)
{
    // Signed 32-Bit Int
    dataPos +=4;
    return (unsigned char)d.at(startPos+3) | ((unsigned char)d.at(startPos+2) << 8) | ((unsigned char)d.at(startPos+1) << 16) | ((unsigned char)d.at(startPos+3) << 24);
}

float gpsBinaryReader::makeFloat(QByteArray d, uint16_t startPos)
{
    // IEEE 32-Bit Float
    dataPos +=4;
    float f=0.0;
    unsigned char bytes[] = { (unsigned char)d.at(startPos+3),   (unsigned char)d.at(startPos+2),
                              (unsigned char)d.at(startPos+1), (unsigned char)d.at(startPos) };

    memcpy(&f, bytes, sizeof(f));
    return f;
}

double gpsBinaryReader::makeDouble(QByteArray a, uint16_t startPos)
{
    // IEEE 64-Bit Float
    //qDebug() << "Reading 64-bit 'double' from position: " << dataPos;
    dataPos +=8;
    double d=0.0;

    unsigned char bytes[] = {  (unsigned char)a.at(startPos+7), (unsigned char)a.at(startPos+6),
                               (unsigned char)a.at(startPos+5), (unsigned char)a.at(startPos+4),
                               (unsigned char)a.at(startPos+3), (unsigned char)a.at(startPos+2),
                               (unsigned char)a.at(startPos+1), (unsigned char)a.at(startPos) };

    memcpy(&d, bytes, sizeof(d));
    return d;
}

void gpsBinaryReader::copyQStringToCharArray(char *array, QString s)
{
    // This is a helper function for people that like to live dangerously.
    if(s.length() > 64-2)
        s.truncate(64-2);
    for(int i=0; i < s.length(); i++)
    {
        array[i] = s[i].toLatin1();
    }
    array[s.length()] = '\0';
}

void gpsBinaryReader::getMessageSum()
{
    //messageSum = std::accumulate(rawData.begin(), rawData.end()-4, 0);

    uint32_t sum = 0;
    for(int i=0; i < rawData.length()-4; i++)
    {
        sum += (unsigned char)rawData.at(i);
    }

    messageSum = sum;
    m.calculatedChecksum = sum;
    // Nominal size: usually 392, once per second 397 and 438 bytes. The -4 is because the checksum
    // does not include the size of the sum.
    //qDebug() << "Byte Array length: " << rawData.length() << ", length-4: " << rawData.length()-4;
}

// Public Access Functions:

messageKinds gpsBinaryReader::getMessageType()
{
    return m.mType;
}

uint32_t gpsBinaryReader::getCounter()
{
    return m.counter;
}

unsigned char gpsBinaryReader::getVersion()
{
    return m.protoVers;
}

float gpsBinaryReader::getHeading()
{
    return m.heading;
}

float gpsBinaryReader::getRoll()
{
    return m.roll;
}

float gpsBinaryReader::getPitch()
{
    return m.pitch;
}

// Public Access Utility Functions:

QString gpsBinaryReader::debugString()
{
    // Use this to quickly return hex printouts as strings for debugging:
    QString s;
    s = QString("0x%1 0x%2 0x%3 0x%4").arg(rawData.at(27), 4, 16, QChar('0')).arg(rawData.at(28), 4, 16, QChar('0')).arg(rawData.at(29), 4, 16, QChar('0'));
    return s;
}

void gpsBinaryReader::debugThis()
{
    // Place any desired debug functions here:
    //    qDebug() << "---------- New Data Dump: ----------";
    //    qDebug() << "Counter: " << getCounter();
    //    printBinary(rawData);

    printMessage();
    //    qDebug() << "Nav Data Block Bitmask: ";
    //    printBinary(m.navDataBlockBitmask);

    //    qDebug() << "Extended Nav Data Block Bitmask: ";
    //    printBinary(m.extendedNavDataBlockBitmask);

    //    qDebug() << "External Sensor Data Block Bitmask: ";
    //    printBinary(m.externDataBitMask);
}

void gpsBinaryReader::printMessage(gpsMessage g)
{
    // This is a stand-alone GPS message printer,
    // DO NOT access this function if you are streaming live data to this function
    // Make a separate instance of the binary reader first.
    this->m = g; // copy over
    this->printMessage();
}

//bool gpsBinaryReader::getBit(uint32_t data, int bit)
//{
//    return (bool)((data<<bit)&0x01);
//}

void gpsBinaryReader::printAlgorithmStatusMessages(gpsMessage g)
{
    if(g.haveINSAlgorithmStatus)
    {
        qDebug() << m.counter << ": " << getBit(g.algorithmStatus1, 0)
                 << getBit(g.algorithmStatus1, 1)
                 << getBit(g.algorithmStatus1, 2)
                 << getBit(g.algorithmStatus1, 12)
                 << getBit(g.algorithmStatus1, 13)
                 << getBit(g.algorithmStatus1, 14)
                 << getBit(g.algorithmStatus1, 15);

        //if( (!getBit(g.algorithmStatus1, 13)) || (getBit(g.algorithmStatus1, 15)) )
//        if( (!getBit(g.algorithmStatus1, 13)) )
//        {
//            qDebug() << "Algorithm Status word 1:";
//            qDebug() << "[ 0] GPS Nav mode: " << getBit(g.algorithmStatus1, 0);
//            qDebug() << "[ 1] GPS Alignment phase: " << getBit(g.algorithmStatus1, 1);
//            qDebug() << "[ 2] GPS Fine Alignment: " << getBit(g.algorithmStatus1, 2);
//            qDebug() << "[12] GPS Received: " << getBit(g.algorithmStatus1, 12);
//            qDebug() << "[13] GPS Valid: " << getBit(g.algorithmStatus1, 13);
//            qDebug() << "[14] GPS Waiting: " << getBit(g.algorithmStatus1, 14);
//            qDebug() << "[15] GPS Rejected: " << getBit(g.algorithmStatus1, 15);
//        }
    }
}

void gpsBinaryReader::printMessage()
{
    qDebug() << "---------- BEGIN printing GPS message for counter " << m.counter << ": ----------";
    qDebug() << "validDecode: " << m.validDecode;
    qDebug() << "Last decoder error message: " << m.lastDecodeErrorMessage;
    qDebug() << "Number of recently dropped mesages: " << m.numberDropped;
    qDebug() << "---Header:---";
    qDebug() << "messageType: " << m.mType;
    qDebug() << "protocol version: " << (unsigned int)m.protoVers;
    qDebug() << "navigation data block bitmask: " << m.navDataBlockBitmask;
    qDebug() << "Binary decode of bitmask: ";
    printBinary(m.navDataBlockBitmask);
    qDebug() << "Extended Nav Data Bitmask: " << m.extendedNavDataBlockBitmask;
    qDebug() << "Binary decode of Extended Nav Data Bitmask: ";
    printBinary(m.extendedNavDataBlockBitmask);
    qDebug() << "External Data Bitmask: " << m.externDataBitMask;
    qDebug() << "Binary decode of External data bitmasl: ";
    printBinary(m.externDataBitMask);
    qDebug() << "navDataValidity time: " << m.navDataValidityTime;
    qDebug() << "counter: " << m.counter;
    qDebug() << "--- end header.";
    qDebug();
    qDebug() << "--- Avaialble data: ---";
    qDebug() << "haveAltitudeHeading: " << m.haveAltitudeHeading;
    qDebug() << "haveAltitudeHeadingStdDev: " << m.haveAltitudeHeadingStdDev;
    qDebug() << "haveRealTimeHeaveSurgeSwayData: " << m.haveRealTimeHeaveSurgeSwayData;
    qDebug() << "haveSmartHeaveData: " << m.haveSmartHeaveData;
    qDebug() << "haveHeadingRollPitchRate: " << m.haveHeadingRollPitchRate;
    qDebug() << "haveBodyRotationRate: " << m.haveBodyRotationRate;
    qDebug() << "haveAccel vessel frame: " << m.haveAccel;
    qDebug() << "havePosition: " << m.havePosition;
    qDebug() << "havePositionStdDev: " << m.havePositionStdDev;
    qDebug() << "haveSpeedData: " << m.haveSpeedData;
    qDebug() << "haveSpeedStdDev: " << m.haveSpeedStdDev;
    qDebug() << "haveCurrentData: " << m.haveCurrentData;
    qDebug() << "haveCurrentStdDev: " << m.haveCurrentStdDev;
    qDebug() << "haveSystemDateData: " << m.haveSystemDateData;
    qDebug() << "haveINSSensorStatus: " << m.haveINSSensorStatus;
    qDebug() << "haveINSAlgorithmStatus: " << m.haveINSAlgorithmStatus;
    qDebug() << "haveINSSystemStatus: " << m.haveINSSystemStatus;
    qDebug() << "haveINSUserStatus: " << m.haveINSUserStatus;
    qDebug() << "processHeaveSurgeSwaySpeed: " << m.haveHeaveSurgeSwaySpeedData;
    qDebug() << "haveSpeedVesselData: " << m.haveSpeedVesselData;
    qDebug() << "haveAccelGeographicData: " << m.haveAccelGeographicData;
    qDebug() << "haveCourseSpeedGroundData: " << m.haveCourseSpeedGroundData;
    qDebug() << "haveTempData: " << m.haveTempData;
    qDebug() << "haveAttitudeQuaternionData: " << m.haveAttitudeQuaternionData;
    qDebug() << "haveAttitudeQEData: " << m.haveAttitudeQEData;
    qDebug() << "haveVesselAccel: " << m.haveVesselAccel;
    qDebug() << "haveVesselAccelStdDev: " << m.haveVesselAccelStdDev;
    qDebug() << "haveVesselRotationRateStdDev: " << m.haveVesselRotationRateStdDev;
    qDebug() << "haveUTC: " << m.haveUTC;
    qDebug() << "have GNSS 1: " << m.haveGNSSInfo1;
    qDebug() << "have GNSS 2: " << m.haveGNSSInfo2;
    qDebug() << "have GNSS 3: " << m.haveGNSSInfo3;
    qDebug() << "--- end list of available data.";

    if(m.haveAltitudeHeading)
    {
        qDebug() << "--- Altitude Heading: ---";
        qDebug() << "heading: " << m.heading;
        qDebug() << "roll: " << m.roll;
        qDebug() << "pitch: " << m.pitch;
    }

    if(m.haveAltitudeHeadingStdDev)
    {
        qDebug() << "--- Altitude Heading StdDev: ---";
        qDebug() << "heading std dev: " << m.headingStardardDeviation;
        qDebug() << "roll std dev: " << m.rollStandardDeviation;
        qDebug() << "pitch std dev: " << m.pitchStandardDeviation;
    }
    if(m.haveRealTimeHeaveSurgeSwayData)
    {
        qDebug() << "--- Navigation Bit 2 Real-time Surge Sway Data: ---";
        qDebug() << "rt_heave_withoutBdL: " << m.rt_heave_withoutBdL;
        qDebug() << "rt_heave_atBdL: " << m.rt_heave_atBdL;
        qDebug() << "rt_surge_atBdL: " << m.rt_surge_atBdL;
        qDebug() << "rt_sway_atBdL: " << m.rt_sway_atBdL;
    }
    if(m.haveSmartHeaveData)
    {
        qDebug() << "--- Navigation Bit 3 Smart Heave Data: ---";
        qDebug() << "smartHeaveValidityTime_100us: " << m.smartHeaveValidityTime_100us;
        qDebug() << "smartHeave_m: " << m.smartHeave_m;
    }
    if(m.haveHeadingRollPitchRate)
    {
        qDebug() << "--- Heading Roll Pitch Rate: ---";
        qDebug() << "heading rotation rate: " << m.headingRotationRate;
        qDebug() << "roll rotation rate: " << m.rollRotationRate;
        qDebug() << "pitch rotation rate: " << m.pitchRotationRate;
    }
    if(m.haveBodyRotationRate)
    {
        qDebug() << "--- Body rotation rate: ---";
        qDebug() << "rotationRateXV1: " << m.rotationRateXV1;
        qDebug() << "rotationRateXV2: " << m.rotationRateXV2;
        qDebug() << "rotationRateXV3: " << m.rotationRateXV3;
    }
    if(m.haveAccel)
    {
        qDebug() << "--- Vessel Accel (bit 6): ---";
        qDebug() << "accelXV1: " << m.accelXV1;
        qDebug() << "accelXV2: " << m.accelXV2;
        qDebug() << "accelXV3: " << m.accelXV3;
    }
    if(m.havePosition)
    {
        qDebug() << "--- Position Data (bit 7): ---";
        qDebug() << "latitude: " << m.latitude;
        qDebug() << "longitude: " << m.longitude;
        qDebug() << "altitude reference: " << (unsigned int) m.altitudeReference;
        qDebug() << "altitude: " << m.altitude;
    }

    if(m.havePositionStdDev)
    {
        qDebug() << "--- havePositionStdDev Data (bit 8): ---";
        qDebug() << "northStdDev: " << m.northStdDev;
        qDebug() << "eastStdDev: " << m.eastStdDev;
        qDebug() << "neCorrelation: " << m.neCorrelation;
        qDebug() << "altitudStdDev: " << m.altitudStdDev;
    }

    if(m.haveSpeedData)
    {
        qDebug() << "--- have Speed Data (bit 9): ---";
        qDebug() << "northVelocity: " << m.northVelocity;
        qDebug() << "eastVelocity: " << m.eastVelocity;
        qDebug() << "upVelocity: " << m.upVelocity;
    }

    if(m.haveSpeedStdDev)
    {
        qDebug() << "--- have speed std dev data (bit 10): ---";
        qDebug() << "northVelocityStdDev: " << m.northVelocityStdDev;
        qDebug() << "eastVelocityStdDev: " << m.eastVelocityStdDev;
        qDebug() << "upVelocityStdDev: " << m.upVelocityStdDev;
    }
    if(m.haveCurrentData)
    {
        qDebug() << "--- have ocean current data (bit 12): ---";
        qDebug() << "northCurrentStdDev: " << m.northCurrentStdDev;
        qDebug() << "eastCurrentStdDev: " << m.eastCurrentStdDev;
    }

    if(m.haveSystemDateData)
    {
        qDebug() << "--- System time and date: (bit 13): ---";
        qDebug() << "systemDay: " << (unsigned int)m.systemDay;
        qDebug() << "systemMonth: " << (unsigned int)m.systemMonth;
        qDebug() << "systemYear: " << (unsigned int)m.systemYear;
    }


    if(m.haveINSSensorStatus)
    {
        qDebug() << "--- have INS SENSOR Status (bit 14): ---";
        qDebug() << "insSensorStatus1: " << m.insSensorStatus1;
        printBinary(m.insSensorStatus1);
        qDebug() << "insSensorStatus2: " << m.insSensorStatus2;
        printBinary(m.insSensorStatus2);
    }

    if(m.haveINSAlgorithmStatus)
    {
        qDebug() << "--- have INS Algorithm Status (bit 15): ---";
        qDebug() << "algorithmStatus1: " << m.algorithmStatus1;
        printBinary(m.algorithmStatus1);
        qDebug() << "algorithmStatus2: " << m.algorithmStatus2;
        printBinary(m.algorithmStatus2);
        qDebug() << "algorithmStatus3: " << m.algorithmStatus3;
        printBinary(m.algorithmStatus3);
        qDebug() << "algorithmStatus4: " << m.algorithmStatus4;
        printBinary(m.algorithmStatus4);
        printAlgorithmStatusMessages(m);
    }
    if(m.haveINSSystemStatus)
    {
        qDebug() << "--- have INS SYSTEM Status (bit 16): ---";
        qDebug() << "systemStatus1: " << m.systemStatus1;
        printBinary(m.systemStatus1);
        qDebug() << "systemStatus2: " << m.systemStatus2;
        printBinary(m.systemStatus2);
        qDebug() << "systemStatus3: " << m.systemStatus3;
        printBinary(m.systemStatus3);
    }
    if(m.haveINSUserStatus)
    {
        qDebug() << "--- have INS USER Status (bit 17): ---";
        qDebug() << "INSuserStatus: " << m.INSuserStatus;
        printBinary(m.INSuserStatus);
    }

    // Note: Navigation bitmask bits 18, 19,and 20 have always been zero.

    if(m.haveHeaveSurgeSwaySpeedData)
    {
        qDebug() << "--- Heave Surge Sway Speed Data (bit 21): --";
        qDebug() << "realtime_heave_speed: " << m.realtime_heave_speed;
        qDebug() << "surge_speed: " << m.surge_speed;
        qDebug() << "sway_speed: " << m.sway_speed;
    }

    if(m.haveSpeedVesselData)
    {
        qDebug() << "--- Vessel speed block Data (bit 22): ---";
        qDebug() << "vesselXV1Velocity: " << m.vesselXV1Velocity;
        qDebug() << "vesselXV2Velocity: " << m.vesselXV2Velocity;
        qDebug() << "vesselXV3Velocity: " << m.vesselXV3Velocity;
    }
    if(m.haveAccelGeographicData)
    {
        qDebug() << "--- Accel Geographic Data Data (bit 23): ---";
        qDebug() << "geographicNorthAccel: " << m.geographicNorthAccel;
        qDebug() << "geographicEastAccel: " << m.geographicEastAccel;
        qDebug() << "geographicVertAccel: " << m.geographicVertAccel;
    }
    if(m.haveCourseSpeedGroundData)
    {
        qDebug() << "--- Course and Speed over ground (bit 24): ---";
        qDebug() << "courseOverGround (degrees): " << m.courseOverGround;
        qDebug() << "speedOverGround: " << m.speedOverGround;
    }
    if(m.haveTempData)
    {
        qDebug() << "--- Temperature data (bit 25): ---";
        qDebug() << "meanTempFOG: " << m.meanTempFOG;
        qDebug() << "meanTempACC: " << m.meanTempACC;
        qDebug() << "meanTempSensor: "<< m.meanTempSensor;
    }

    if(m.haveAttitudeQuaternionData)
    {
        qDebug() << "--- Attitude Quaternion Data (bit 26): ---";
        qDebug() << "attitudeQCq0: " << m.attitudeQCq0;
        qDebug() << "attitudeQCq1: " << m.attitudeQCq1;
        qDebug() << "attitudeQCq2: " << m.attitudeQCq2;
        qDebug() << "attitudeQCq3: " << m.attitudeQCq3;
    }
    if(m.haveAttitudeQEData)
    {
        qDebug() << "--- Attitude Quaternion E-Data (bit 27): ---";
        qDebug() << "attitudeQE1: " << m.attitudeQE1;
        qDebug() << "attitudeQE2: " << m.attitudeQE2;
        qDebug() << "attitudeQE3: " << m.attitudeQE3;
    }
    if(m.haveVesselAccel)
    {
        qDebug() << "--- Raw Vessel Acceleration Data (bit 28): ---";
        qDebug() << "vesselAccelXV1: " << m.vesselAccelXV1;
        qDebug() << "vesselAccelXV1: " << m.vesselAccelXV1;
        qDebug() << "vesselAccelXV1: " << m.vesselAccelXV1;
    }
    if(m.haveVesselAccelStdDev)
    {
        qDebug() << "--- Vessel Acceleration Std Dev (bit 29): ---";
        qDebug() << "vesselAccelXV1StdDev: " << m.vesselAccelXV1StdDev;
        qDebug() << "vesselAccelXV2StdDev: " << m.vesselAccelXV2StdDev;
        qDebug() << "vesselAccelXV3StdDev: " << m.vesselAccelXV3StdDev;
    }
    if(m.haveVesselRotationRateStdDev)
    {
        qDebug() << "--- Vessel Rotation Rate Std Dev (bit 30): ---";
        qDebug() << "vesselRotationRateXV1StdDev: " << m.vesselRotationRateXV1StdDev;
        qDebug() << "vesselRotationRateXV2StdDev: " << m.vesselRotationRateXV2StdDev;
        qDebug() << "vesselRotationRateXV3StdDev: " << m.vesselRotationRateXV3StdDev;
    }

    qDebug() << "----- END Navigation Data Blocks -----";

    qDebug() << "----- BEGIN Extended Navigation Data Blocks -----";
    if(m.haveExtendedRawRotationAccelData)
    {
        qDebug() << "--- Extended Raw Rotation AccelData (bit 0): ---";
        qDebug() << "rawRotationAccelXV1: " << m.rawRotationAccelXV1;
        qDebug() << "rawRotationAccelXV2: " << m.rawRotationAccelXV2;
        qDebug() << "rawRotationAccelXV3: " << m.rawRotationAccelXV3;
    }
    if(m.haveExtendedRawRotationAccelStdDevData)
    {
        qDebug() << "--- Extended Raw Rotation Accel Std Dev Data (bit 1): ---";
        qDebug() << "rawRotationAccelStdDevXV1: " << m.rawRotationAccelStdDevXV1;
        qDebug() << "rawRotationAccelStdDevXV2: " << m.rawRotationAccelStdDevXV2;
        qDebug() << "rawRotationAccelStdDevXV3: " << m.rawRotationAccelStdDevXV3;
    }
    if(m.haveExtendedRawRotationRateData)
    {
        qDebug() << "--- Extended Raw Rotation Rate Data (bit 2): ---";
        qDebug() << "rawRotationRateXV1: " << m.rawRotationRateXV1;
        qDebug() << "rawRotationRateXV2: " << m.rawRotationRateXV2;
        qDebug() << "rawRotationRateXV3: " << m.rawRotationRateXV3;
    }
    qDebug() << "----- END Extended Navigation Data Blocks -----";


    //if(m.externDataBitMask != 0)

        qDebug() << "----- BEGIN External Data Blocks: ";
        if(m.haveUTC)
        {
            qDebug() << "--- UTC: ---";
            qDebug() << "UTC Data Validity time: " << m.UTCdataValidityTime;
            qDebug() << "UTC Source: " << m.UTCSource;
        }

        if(m.haveGNSSInfo1)
        {
            qDebug() << "--- GNSS 1: ---";
            printGNSSInfo(1);
        }
        if(m.haveGNSSInfo2)
        {
            qDebug() << "--- GNSS 2: ---";
            printGNSSInfo(2);
        }
        if(m.haveGNSSInfo3)
        {
            qDebug() << "--- GNSS 3: ---";
            printGNSSInfo(3);
        }
        qDebug() << "----- END External Data Blocks. -----";


    // Repeat for good measure:
    qDebug() << "Valid Decode: " << m.validDecode;
    qDebug() << "Last error: " << m.lastDecodeErrorMessage;

    qDebug() << "Calculated Message Sum: " << messageSum;
    qDebug() << "Claimed Message Sum:    " << m.claimedMessageSum;
    qDebug() << "Checksum good?:         " << (messageSum==m.claimedMessageSum);
    qDebug() << "---------- END message decode for counter " << m.counter << " ----------";

}

void gpsBinaryReader::printGNSSInfo(int num)
{
    gnssInfo *g = &m.gnss[num-1];
    if(!g->populated)
    {
        qDebug() << "ERROR: GNSS info " << num << " not populated.";
        return;
    }

    qDebug() << "gnssDataValidityTime: " << g->gnssDataValidityTime;
    qDebug() << "gnssIdentification: " << g->gnssIdentification;
    qDebug() << "gnssQuality: " << g->gnssQuality;
    qDebug() << "gnssGPSQuality: " << g->gnssGPSQuality;
    qDebug() << "gnssLatitude: " << g->gnssLatitude;
    qDebug() << "gnssLongitude: " << g->gnssLongitude;
    qDebug() << "gnssAltitude: " << g->gnssAltitude;
    qDebug() << "gnssLatStdDev: " << g->gnssLatStdDev;
    qDebug() << "gnssLongStddev: " << g->gnssLongStddev;
    qDebug() << "gnssAltStdDev: " << g->gnssAltStdDev;
    qDebug() << "LatLongCovariance: " << g->LatLongCovariance;
    qDebug() << "geoidalSep: " << g->geoidalSep;

}


void gpsBinaryReader::printBinary(uint32_t data)
{
    // Use this to print out any 32 bits as binary:
    QString message_index;
    QString message_data;
    unsigned char bit = 0;

    for(int i=0; i < 32; i++)
    {
        bit = (data >> i) & 1;
        message_index.append(QString("[%1]").arg(i, 2, 10, QChar('0')));
        message_data.append(QString("[%1]").arg(bit, 2, 10, QChar(' ')));
    }

    qDebug() << "Number as unsigned int: " << data;
    qDebug() << "Number as hex: " << QString("0x%1").arg(data, 8, 16, QChar('0'));
    qDebug() << message_index;
    qDebug() << message_data;

}

void gpsBinaryReader::printBinary(QByteArray dataArray)
{
    // Use this to print out any data array as binary:

    QString s;
    QString binStr;
    unsigned char byte;
    unsigned char bit = 0;
    for(int b=0; b < dataArray.size(); b++)
    {
        byte = dataArray.at(b);
        for(int p=0; p < 8; p++)
        {
            bit = (byte >> p) & 1;
            binStr.append(QString("%1").arg(bit, 1, 10, QChar(' ')));
        }

        s = QString("Byte: %1: 0x%2 = 0b%3").arg(b,3,10,QChar('0')).arg(byte, 2, 16, QChar('0')).arg(binStr);
        qDebug() << s;
        binStr.clear();
    }
}
