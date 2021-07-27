#ifndef GPSBINARYREADER_H
#define GPSBINARYREADER_H

#include <QByteArray>
#include <QDebug>

enum messageKinds {
    msgCM_input,
    msgAN_outputAnswer,
    msgIX_outputNav,
    msgUnknown
};

enum gpsQualityKinds {
    gpsQualityInvalid = 0,
    gpsQualityNatural_10m = 1,
    gpsQualityDifferential_3m = 2,
    gpsQualityMilitary_10m = 3,
    gpsQualityRTK_0p1m = 4,
    gpsQualityFloatRTK_0p3m = 5,
    gpsQualityOther = 6
};

struct gnssInfo {
    bool populated = false;
    long gnssDataValidityTime;
    unsigned char gnssIdentification; // 0 = GNSS1, 2 = Manual GNSS
    unsigned char gnssQuality;
    gpsQualityKinds gnssGPSQuality;
    double gnssLatitude; // -90 to 90
    double gnssLongitude; // 0-360
    float gnssAltitude;
    float gnssLatStdDev;
    float gnssLongStddev;
    float gnssAltStdDev;
    float LatLongCovariance;
    float geoidalSep;
};

// To match the exact wording of the ICD:
#define word uint16_t
#define dword uint32_t

struct gpsMessage {

    bool validDecode;
    uint32_t numberDropped = 0;
    char lastDecodeErrorMessage[64];

    // Header:
    messageKinds mType;
    char protoVers;
    dword navDataBlockBitmask = 0;
    dword extendedNavDataBlockBitmask = 0;
    dword externDataBitMask = 0;

    word navigationDataSize = 0;
    word totalTelegramSize = 0;
    dword navDataValidityTime = 0;
    dword counter = 0;

    dword claimedMessageSum = 0;

    //////////////////////
    //
    //  Begin Navigation Data
    //


    // Altitude and Heading data Block (bit 0):
    float heading;
    float roll;
    float pitch;
    bool haveAltitudeHeading;

    // Altitude and Heading standard deviation data block (bit 1):
    float headingStardardDeviation;
    float rollStandardDeviation;
    float pitchStandardDeviation;
    bool haveAltitudeHeadingStdDev;

    // Mystery data, bit 2: RealTimeHeaveSurgeSway

    float rt_heave_withoutBdL; /*! Meters - positive UP in horizontal vehicle frame */
    float rt_heave_atBdL;      /*! Meters - positive UP in horizontal vehicle frame */
    float rt_surge_atBdL; /*! Meters - positive FORWARD in horizontal vehicle frame */
    float rt_sway_atBdL;  /*! Meters - positive PORT SIDE in horizontal vehicle frame */
    bool haveRealTimeHeaveSurgeSwayData;

    // SmartHeave data, bit 3:
    dword smartHeaveValidityTime_100us;
    float smartHeave_m;
    bool haveSmartHeaveData;

    // Heading/Roll/Pitch rate data block (bit 4):
    float headingRotationRate;
    float rollRotationRate;
    float pitchRotationRate;
    bool haveHeadingRollPitchRate;

    // Body rotation rate data block in vessel frame (bit 5):
    float rotationRateXV1;
    float rotationRateXV2;
    float rotationRateXV3;
    bool haveBodyRotationRate;

    // Accelerations data block in vessel frame (bit 6):
    float accelXV1;
    float accelXV2;
    float accelXV3;
    bool haveAccel;

    // Position data (bit 7):
    double latitude;
    double longitude;
    unsigned char altitudeReference;
    float altitude;
    bool havePosition;

    // Position standard deviation data block (bit 8):
    float northStdDev;
    float eastStdDev;
    float neCorrelation;
    float altitudStdDev;
    bool havePositionStdDev;

    // Speed data block in geographic frame (bit 9):
    float northVelocity;
    float eastVelocity;
    float upVelocity;
    bool haveSpeedData;

    // Speed standard deviation data block in geographic frame (bit 10):
    float northVelocityStdDev;
    float eastVelocityStdDev;
    float upVelocityStdDev;
    bool haveSpeedStdDev;

    // (ocean) Current data block in geographic frame (bit 11):
    float northCurrent;
    float eastCurrent;
    bool haveCurrentData;

    // (ocean) Current standard deviation data block in gepgraphic frame (bit 12):
    float northCurrentStdDev;
    float eastCurrentStdDev;
    bool haveCurrentStdDev;

    // System date data block (bit 13):
    unsigned char systemDay;
    unsigned char systemMonth;
    word systemYear;
    bool haveSystemDateData;

    // INS Sensor Status (bit 14):
    dword insSensorStatus1;
    dword insSensorStatus2;
    bool haveINSSensorStatus;

    // INS Algorithm Status (bit 15):
    dword algorithmStatus1;
    dword algorithmStatus2;
    dword algorithmStatus3;
    dword algorithmStatus4;
    bool haveINSAlgorithmStatus;

    // INS System Status (bit 16):
    dword systemStatus1;
    dword systemStatus2;
    dword systemStatus3;
    bool haveINSSystemStatus;

    // INS User Status (bit 17):
    dword INSuserStatus;
    bool haveINSUserStatus;

    // Bits 18, 19, and 20 have never been received

    // Heave Surge Sway data (bit 21):
    float realtime_heave_speed;
    float surge_speed;
    float sway_speed;
    bool haveHeaveSurgeSwaySpeedData;

    //    From the OEM software, these are the next ones ("reserved" in the spec):
    //    boost::optional<AHRSAlgorithmStatus> ahrsAlgorithmStatus;
    //    boost::optional<AHRSSystemStatus> ahrsSystemStatus;
    //    boost::optional<AHRSUserStatus> ahrsUserStatus;

    // Speed data block in vessel frame (bit 22):
    float vesselXV1Velocity;
    float vesselXV2Velocity;
    float vesselXV3Velocity;
    bool haveSpeedVesselData;

    // Acceleration data block in geographic frame (bit 23):
    float geographicNorthAccel;
    float geographicEastAccel;
    float geographicVertAccel;
    bool haveAccelGeographicData;

    // Course and speed over ground (bit 24):
    float courseOverGround;
    float speedOverGround;
    bool haveCourseSpeedGroundData;

    // Temperatures (bit 25):
    float meanTempFOG;
    float meanTempACC;
    float meanTempSensor;
    bool haveTempData;

    // Attitude quaternion (bit 26):
    float attitudeQCq0;
    float attitudeQCq1;
    float attitudeQCq2;
    float attitudeQCq3;
    bool haveAttitudeQuaternionData;

    // Attitude quaternion standard deviation (bit 27):
    float attitudeQE1;
    float attitudeQE2;
    float attitudeQE3;
    bool haveAttitudeQEData;

    // Raw acceleration in vessel frame (bit 28):
    float vesselAccelXV1;
    float vesselAccelXV2;
    float vesselAccelXV3;
    bool haveVesselAccel;

    // Acceleration standard deviation in vessel frame (bit 29):
    float vesselAccelXV1StdDev;
    float vesselAccelXV2StdDev;
    float vesselAccelXV3StdDev;
    bool haveVesselAccelStdDev;

    // Rotation rate standard deviation in vessel frame (bit 30):
    float vesselRotationRateXV1StdDev;
    float vesselRotationRateXV2StdDev;
    float vesselRotationRateXV3StdDev;
    bool haveVesselRotationRateStdDev;

    //
    //  End Navigation Data
    //
    //////////////////////

    //////////////////////
    //
    //  Begin Extended Navigation Data
    //

    // Rotation accelerations in vessel frame (bit 0):
    float rawRotationAccelXV1;
    float rawRotationAccelXV2;
    float rawRotationAccelXV3;
    bool haveExtendedRawRotationAccelData = false;

    // Rotation acceleration standard deviation in vessel frame (bit 1):
    float rawRotationAccelStdDevXV1;
    float rawRotationAccelStdDevXV2;
    float rawRotationAccelStdDevXV3;
    bool haveExtendedRawRotationAccelStdDevData = false;

    // Raw rotation rate in vessel frame (bit 2):
    float rawRotationRateXV1;
    float rawRotationRateXV2;
    float rawRotationRateXV3;
    bool haveExtendedRawRotationRateData = false;

    //
    //  End Extended Navigation Data
    //
    //////////////////////

    //////////////////////
    //
    //  Begin External sensors data blocks
    //

    // UTC data block (bit 0):
    dword UTCdataValidityTime;
    unsigned char UTCSource;
    bool haveUTC;

    // GNSS and Manual GNSS data blocks (bits 1, 2, and 3):
    // may contain data for external (extra) GNSS inputs 2 and 3

    bool haveGNSSInfo1;
    bool haveGNSSInfo2;
    bool haveGNSSInfo3;

    gnssInfo gnss[3];

    // DMI:
    long dmiDataValidityTime;
    long dmiPulseCount;

    // Event Marker A to C data blocks (bits 18, 19, 20):
    // Read as many as three times?
    long eventDataValidityTime;
    unsigned char eventIdentification;
    long eventCount;

    // VTG1 and VTG2 data blocks (bits 25, 26):
    long vtgDataValidityTime_25;
    long vtgID_25;
    float vtgTrueCourse_25;
    float vtgMagneticCourse_25;
    float vtgSpeedOverGround_25;

    long vtgDataValidityTime_26;
    long vtgID_26;
    float vtgTrueCourse_26;
    float vtgMagneticCourse_26;
    float vtgSpeedOverGround_26;

    // LogBook data blocks (bit 27):
    long logBookDataValidityTime;
    dword logBookIdentifier;
    char logBookCustomText[32] = {'0'};

};

Q_DECLARE_METATYPE(gpsMessage)

class gpsBinaryReader
{
private:
    void initialize();

    QByteArray rawData;
    uint16_t dataPos;
    uint32_t oldCounter = 0;
    bool firstRun;
    bool decodeInvalid;
    uint32_t messageSum;

    gpsMessage m;

    // Message Decoders:
    void processData();
    void detMsgType();
    void processNavOutHeaderV2();
    void processNavOutHeaderV3();
    void processNavOutHeaderV5();
    void processAltitudeHeadingData();
    void processAltitudeHeadingStdDevData();

    void processRealTimeHeaveSurgeSwayData();
    void processSmartHeaveData();

    void processHeadingRollPitchRateData();
    void processBodyRotationRate();
    void processAccelerationData();
    void processPositionData();
    void processPositionStdDevData();
    void processSpeedData();
    void processSpeedStdDevData();
    void processCurrentData();
    void processCurrentStdDevData();
    void processSystemDateData();

    void processINSSensorStatus();
    void processINSAlgorithmStatus();
    void processINSSystemStatus();
    void processINSUserStatus();

    void processHeaveSurgeSwaySpeed();

    void processVesselVelocity();
    void processAccelGeographic();
    void processCourseSpeedGround();

    void processTemperature();

    void processAttitudeQuaternion();
    void processAttitudeQuaternationStdDev();

    void processVesselAccel();
    void processVesselAccelStdDev();
    void processVesselRotationRateStdDev();

    // Extemded Navigation Data Blocks:
    void processExtendedRotationAccelData();
    void processExtendedRotationAccelStdDevData();
    void processExtendedRawRotationRateData();

    // External Data Blocks:
    void processUTC();
    void processGNSS(int num);

    void getMessageSum();
    bool checkMessageSum();

    // Helper functions:
    word makeWord(QByteArray, uint16_t startPos); // unsigned 16 bit int
    short makeShort(QByteArray, uint16_t startPos); // signed 16 bit int
    dword makeDWord(QByteArray, uint16_t startPos); // unsigned 32 bit int
    long makeLong(QByteArray, uint16_t startPos); // signed 32 bit int
    float makeFloat(QByteArray, uint16_t startPos); // IEEE 32 bit float
    double makeDouble(QByteArray, uint16_t startPos); // IEEE 64 bit float

    void copyQStringToCharArray(char *array, QString s);

    unsigned char getBit(dword d, unsigned char bit);

public:
    gpsBinaryReader();
    gpsBinaryReader(QByteArray rawData);
    void insertData(QByteArray rawData);
    gpsMessage getMessage();

    messageKinds getMessageType();
    uint32_t getCounter();
    unsigned char getVersion();
    float getHeading();
    float getRoll();
    float getPitch();

    // Utility Functions:
    QString debugString();
    void debugThis();
    void printMessage(gpsMessage g);
    void printMessage();
    void printGNSSInfo(int num);
    void printBinary(dword data);
    void printBinary(QByteArray data);
};

#endif // GPSBINARYREADER_H
