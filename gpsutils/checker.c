#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <fcntl.h>
#include <string.h>

#define dword uint32_t
#define word uint16_t
#define byte unsigned char

typedef struct {
    size_t offsetBytes;
    bool valid;
    bool readEntireFile;
    size_t pos;
    // Header:
    char headerIX[2];
    char protVers;
    dword navDataBlockBitmask;
    dword extendedNavDataBlockBitmask;
    dword externDataBitMask;
    word navDataSize;
    word totalTelegramSize;
    dword navDataValidityTime;
    dword counter;
    bool validHeader;

    int validityHour;
    int validityMinute;
    float validitySecond;
    char valString[32];

    bool haveHeadingRollPitch;
    float heading;
    float roll;
    float pitch;

    bool havePosition;
    double latitude;
    double longitude;
    char altitudeReference;
    float altitude;

    bool haveSystemDate;
    char day;
    char month;
    word year;
    char sysDateStr[13];


    // INS Sensor Status (bit 14):
    bool haveINSSensorStatus;
    dword insSensorStatus1;
    dword insSensorStatus2;

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

    // Course and speed over ground (bit 24):
    float courseOverGround;
    float speedOverGround;
    bool haveCourseSpeedGroundData;

    // Temperatures (bit 25):
    float meanTempFOG;
    float meanTempACC;
    float meanTempSensor;
    bool haveTempData;

    // External Data Bitmasks:

    // UTC (bit 0):
    dword UTCdataValidityTime;
    unsigned char UTCSource;
    int UTC_hour;
    int UTC_minute;
    float UTC_second;
    char UTCString[32];
    bool haveUTC;

    bool checksumPassed;
    dword checksum;

} telegram_t;

bool checkChecksum(char*b, dword claim, word totalSize) {
    dword sum = 0;
    for(int n=0; n < totalSize-4; n++) {
        sum = sum+(unsigned char)b[n];
    }
    //printf("claim: %u, sum: %u\n", claim, sum);
    return(sum==claim);
}
word getWord(char* buffer, size_t startPos) {
    // return (unsigned char)d.at(startPos+1) | ((unsigned char)d.at(startPos+0) << 8);
    return  (unsigned char)buffer[startPos+1] | ((unsigned char)buffer[startPos] << 8);
}

dword getDWord(char* buffer, size_t startPos) {
    // return (unsigned char)d.at(startPos+3) |  ((unsigned char)d.at(startPos+2) << 8) |   ((unsigned char)d.at(startPos+1) << 16) |   ((unsigned char)d.at(startPos+0) << 24);
    return (unsigned char)buffer[startPos+3] | ((unsigned char)buffer[startPos+2] << 8) | ((unsigned char)buffer[startPos+1] << 16) | ((unsigned char)buffer[startPos+0] << 24);
}

float getFloat(char* d, size_t startPos)
{
    // IEEE 32-Bit Float
    // dataPos +=4;
    float f=0.0;
    unsigned char bytes[] = { (unsigned char)d[startPos+3],   (unsigned char)d[startPos+2],
                             (unsigned char)d[startPos+1], (unsigned char)d[startPos] };

    memcpy(&f, bytes, sizeof(f));
    return f;
}

double getDouble(char* a, size_t startPos)
{
    // IEEE 64-Bit Float
    //dataPos +=8;
    double d=0.0;

    unsigned char bytes[] = {  (unsigned char)a[startPos+7], (unsigned char)a[startPos+6],
                             (unsigned char)a[startPos+5], (unsigned char)a[startPos+4],
                             (unsigned char)a[startPos+3], (unsigned char)a[startPos+2],
                             (unsigned char)a[startPos+1], (unsigned char)a[startPos] };

    memcpy(&d, bytes, sizeof(d));
    return d;
}


unsigned char getBit(uint32_t num, size_t position) {
    return((num & ( 1 << position )) >> position);
}

void printMessageData(telegram_t m) {
    // Prints out some data from the telegram.
    printf("C: %u, ", m.counter);
    printf("navValTime: %s, ", m.valString);
    printf("navValTime: %u, ", m.navDataValidityTime);
    if(m.haveSystemDate) {
        printf("%s, ", m.sysDateStr);
    }
    if(m.havePosition) {
        printf("Lat: %f, Long: %f, alt: %05.2f (ft)\n",
               m.latitude, m.longitude, m.altitude*3.28084);
    }
    if(m.haveHeadingRollPitch) {
        printf("Heading: %3.2f, Roll: %3.3f, Pitch: %3.3f, ", m.heading,
               m.roll,
               m.pitch);
    }
    if(m.haveCourseSpeedGroundData) {
        printf("course: %05.2f (deg), gndSpeed: %05.2f (knots),\n",
               m.courseOverGround, m.speedOverGround*1.94384);
    }
    if(m.haveTempData) {
        printf("Board temp: %f (c)", m.meanTempFOG);
    }
    if(m.haveUTC) {
        // Once per second, decimals always zero
        printf("UTC Time: %s", m.UTCString);
    }
    printf("\n");
}

// this function from http://tungchingkai.blogspot.com/2011/07/binary-strstr.html
char *(bstrchr) (const char *s, int c, size_t l) {
    /* find first occurrence of c in char s[] for length l*/
    const char ch = c;
    /* handle special case */
    if (l == 0)
        return (NULL);

    for (; *s != ch; ++s, --l)
        if (l == 0)
            return (NULL);
    return ((char*)s);
}

char *(bstrstr)(const char *s1, size_t l1, const char *s2, size_t l2) {
    /* find first occurrence of s2[] in s1[] for length l1*/
    const char *ss1 = s1;
    const char *ss2 = s2;
    /* handle special case */
    if (l1 == 0)
        return (NULL);
    if (l2 == 0)
        return ((char *)s1);

    /* match prefix */
    for (; (s1 = bstrchr(s1, *s2, ss1-s1+l1)) != NULL && ss1-s1+l1!=0; ++s1) {

        /* match rest of prefix */
        const char *sc1, *sc2;
        for (sc1 = s1, sc2 = s2; ;)
            if (++sc2 >= ss2+l2)
                return ((char *)s1);
            else if (*++sc1 != *sc2)
                break;
    }
    return (NULL);
}

void test(const char *s1, size_t l1) {
    // Print the bytes in hex format from s1 up tp length l1
    for(size_t i=0; i < l1; i++) {
        printf("primary[%zu] = %x\n", i, s1[i]);
    }
    return;
}


telegram_t readHeader(char* mbuf, size_t maxlen) {

    telegram_t m;
    m.valid = false;
    m.validHeader = false;
    m.readEntireFile = false;
    m.counter = 0;
    size_t pos=0;
    m.pos = 0;
    m.offsetBytes = 0;
    bool foundGarbage = false;

    if(mbuf) {
        size_t i=0;
        while( !((mbuf[i]=='I') && (mbuf[i+1]=='X')) ) {
            i++;
            //printf("Read %zu bytes and have not found IX header start yet.\n", i);
            foundGarbage = true;
            if(i > maxlen-2) {
                fprintf(stderr, "Error, exceeded buffer length (%zu) and did not discover header.\n", maxlen);
                m.valid = false;
                m.readEntireFile = true;
                return m;
            }
        }
        m.offsetBytes += i; // save for later
        m.valid = true;
        m.validHeader = true;
        pos = i+2;
        // word = uint16_t, 2 bytes
        // dword = uint32_t, 4 bytes
        //printf("Header found at %zu bytes\n", i);
        m.protVers = mbuf[pos]; pos++;
        //printf("Protocol version: %d\n", m.protVers);
        if(m.protVers != 5){
            m.valid = false;
            m.validHeader = false;
            return m;
        }
        m.navDataBlockBitmask = getDWord(mbuf, pos); pos+=4;
        m.extendedNavDataBlockBitmask = getDWord(mbuf, pos); pos+=4;
        m.externDataBitMask = getDWord(mbuf, pos); pos+=4;
        m.navDataSize = getWord(mbuf, pos); pos+=2;
        //printf("Claimed navigation data size: %hu\n", m.navDataSize);
        m.totalTelegramSize = getWord(mbuf, pos); pos+=2; // mbuf[i+2+1+4+4+4+2+2];
        //printf("Claimed total telegram size: %hu\n", m.totalTelegramSize);
        m.navDataValidityTime = getDWord(mbuf, pos); pos+=4;
        m.checksum = getDWord(mbuf, m.totalTelegramSize-0-4);
        m.checksumPassed = checkChecksum(mbuf, m.checksum, m.totalTelegramSize);

        uint64_t t = m.navDataValidityTime;
        m.validityHour = t / ((float)1E4)/60.0/60.0;
        m.validityMinute = ( t / ((float)1E4)/60.0 ) - (m.validityHour*60) ;
        m.validitySecond = ( t / ((float)1E4) ) - (m.validityHour*60.0*60.0) - (m.validityMinute*60.0);


        m.counter = getDWord(mbuf, pos); pos+=4;
        //        printf("Counter: %u, Nav Validity Time: %02d:%0d:%02d\n",
        //               m.counter,
        //               m.validityHour, m.validityMinute, m.validitySecond);

        sprintf(m.valString, "%02d:%02d:%02.3f",
                m.validityHour, m.validityMinute, m.validitySecond);

        m.pos = pos; // save the spot
    }

    if(foundGarbage) {
        fprintf(stderr, "Note: Found %zu bytes garbage before header.\n", m.offsetBytes);
    }
    return m;
}

telegram_t parseNavData(char* mbuf, size_t startPos, telegram_t m) {
    // Read message and print some data about it
    // such as UTC, lat/long, counter

    // startPos should be the position in mbuf where the telegram starts
    // m.pos should be the position where the header was finished by readHeader(...).
    // m.offsetBytes is any needed offset that readHeader(...) found was needed.
    // Set offsetBytes to zero if it has already been accounted for by the calling function.

    // This is a work in progress and not complete.

    // Name: Bits:  Bytes:
    // Word   16    2
    // DWord  32    4
    // Float  32    4
    // Double 64    8

    /*

    if(!m.validHeader)
    {
        return;
        //m = readHeader(mbuf, (size_t)400); // dangerous assumption...
    }

    */


    size_t origPos = m.pos;
    m.pos += startPos;
    m.pos += m.offsetBytes;

    if(getBit(m.navDataBlockBitmask, 0)) {
        // We have heading, roll, pitch
        //heading
        m.heading = getFloat(mbuf, m.pos);
        m.pos += 4;
        //roll
        m.roll = getFloat(mbuf, m.pos);
        m.pos += 4;
        //pitch
        m.pitch = getFloat(mbuf, m.pos);
        m.pos += 4;
        m.haveHeadingRollPitch = true;
        //        printf("Heading: %3.2f, Roll: %3.3f, Pitch: %3.3f\n", m.heading,
        //               m.roll,
        //               m.pitch);
    }
    if(getBit(m.navDataBlockBitmask, 1)) {
        // heading std dev
        m.pos +=4;
        // roll std dev
        m.pos +=4;
        // pitch std dev
        m.pos +=4;
    }
    if(getBit(m.navDataBlockBitmask, 2))
    {
        //processRealTimeHeaveSurgeSwayData();
        // We would have gotten 4 floats
        m.pos +=4;
        m.pos +=4;
        m.pos +=4;
        m.pos +=4;
    }
    if(getBit(m.navDataBlockBitmask, 3))
    {
        //processSmartHeaveData();
        // We would have gotten two DWords.
        m.pos +=4;
        m.pos +=4;
    }
    if(getBit(m.navDataBlockBitmask, 4)) {
        // Heading/roll/pitch rate data block
        m.pos +=4;
        m.pos +=4;
        m.pos +=4;
    }
    if(getBit(m.navDataBlockBitmask, 5)) {
        // Body rotation rates in vessel frame
        m.pos +=4;
        m.pos +=4;
        m.pos +=4;
    }
    if(getBit(m.navDataBlockBitmask, 6)) {
        // Acceleration data blocks
        m.pos +=4;
        m.pos +=4;
        m.pos +=4;
    }
    if(getBit(m.navDataBlockBitmask, 7)) {
        // We have Position
        m.havePosition = true;
        m.latitude = getDouble(mbuf, m.pos); m.pos +=8;
        m.longitude = getDouble(mbuf, m.pos); m.pos +=8;
        m.altitudeReference = mbuf[m.pos]; m.pos++;
        m.altitude = getFloat(mbuf, m.pos); m.pos +=4;
        //printf("Lat: %f, Long: %f, altitude: %5.2f(ft)\n", m.latitude, m.longitude, m.altitude*3.28084);
    }
    if(getBit(m.navDataBlockBitmask, 8)) {
        // Position Std. Dev
        m.pos +=4;
        m.pos +=4;
        m.pos +=4;
        m.pos +=4;
    }
    if(getBit(m.navDataBlockBitmask, 9)) {
        // Speed data block in geographic frame
        m.pos +=4;
        m.pos +=4;
        m.pos +=4;
    }
    if(getBit(m.navDataBlockBitmask, 10)) {
        // Speed std dev
        m.pos +=4;
        m.pos +=4;
        m.pos +=4;
    }
    if(getBit(m.navDataBlockBitmask, 11)) {
        // Current Data Block in geographic frame
        m.pos +=4;
        m.pos +=4;
    }
    if(getBit(m.navDataBlockBitmask, 12)) {
        // Current standard deviation data block
        m.pos +=4;
        m.pos +=4;
    }
    if(getBit(m.navDataBlockBitmask, 13)) {
        // We have System Date
        m.haveSystemDate = true;
        m.day = mbuf[m.pos]; m.pos++;
        m.month = mbuf[m.pos]; m.pos++;
        m.year = getWord(mbuf, m.pos); m.pos +=2;
        sprintf(m.sysDateStr, "%d-%02d-%02d",
                m.year, m.month, m.day);
        //printf("System time: YYYY-MM-DD: %d-%02d-%02d\n", m.year, m.month, m.day);
    }
    if(getBit(m.navDataBlockBitmask, 14 )) {
        //INS Sensor Status
        m.insSensorStatus1 = getDWord(mbuf, m.pos); m.pos+=4;
        m.insSensorStatus2 = getDWord(mbuf, m.pos); m.pos+=4;
        m.haveINSSensorStatus = true;
    }
    if(getBit(m.navDataBlockBitmask, 15))
    {
        // Have INS Algorithm Status
        m.algorithmStatus1 = getDWord(mbuf, m.pos); m.pos+=4;
        m.algorithmStatus2 = getDWord(mbuf, m.pos); m.pos+=4;
        m.algorithmStatus3 = getDWord(mbuf, m.pos); m.pos+=4;
        m.algorithmStatus4 = getDWord(mbuf, m.pos); m.pos+=4;
        m.haveINSAlgorithmStatus = true;
    }
    if(getBit(m.navDataBlockBitmask, 16))
    {
        // Have INS System Status
        m.systemStatus1 = getDWord(mbuf, m.pos); m.pos+=4;
        m.systemStatus2 = getDWord(mbuf, m.pos); m.pos+=4;
        m.systemStatus3 = getDWord(mbuf, m.pos); m.pos+=4;
        m.haveINSSystemStatus = true;
    }
    if(getBit(m.navDataBlockBitmask, 17))
    {
        // Have INS User Status
        m.pos+=4;
    }
    // reserved: 18, 19, 20
    if(getBit(m.navDataBlockBitmask, 21))
    {
        // Heave Surge Sway Speed
        // Three floats
        m.pos+=4;
        m.pos+=4;
        m.pos+=4;
    }
    if(getBit(m.navDataBlockBitmask, 22))
    {
        // Vessel speed data";
        // Three floats
        m.pos+=4;
        m.pos+=4;
        m.pos+=4;
    }
    if(getBit(m.navDataBlockBitmask, 23))
    {
        // Have accel geographic frame
        // three floats
        m.pos+=4;
        m.pos+=4;
        m.pos+=4;
    }
    if(getBit(m.navDataBlockBitmask, 24))
    {
        // Have course and speed over ground
        m.courseOverGround = getFloat(mbuf, m.pos); m.pos+=4;
        m.speedOverGround = getFloat(mbuf, m.pos); m.pos+=4;
        m.haveCourseSpeedGroundData = true;
    }
    if(getBit(m.navDataBlockBitmask, 25))
    {
        // Have Temperatures (of system)
        m.meanTempFOG = getFloat(mbuf, m.pos); m.pos+=4;
        m.meanTempACC = getFloat(mbuf, m.pos); m.pos+=4;
        m.meanTempSensor = getFloat(mbuf, m.pos); m.pos+=4;
        m.haveTempData = true;
        // printf("FOG Temp: %f, ACC Temp: %f, Sens Temp: %f\n",
        //       m.meanTempFOG,
        //       m.meanTempACC,
        //       m.meanTempSensor);
    }
    if(getBit(m.navDataBlockBitmask, 26))
    {
        //qDebug() << "Have attitude quaternion";
        //processAttitudeQuaternion();
        // Four floats:
        m.pos+=4; m.pos+=4; m.pos+=4; m.pos+=4;
    }
    if(getBit(m.navDataBlockBitmask, 27))
    {
        //qDebug() << "Have attitude quaternion std dev";
        //processAttitudeQuaternationStdDev();
        // Three floats:
        m.pos+=4; m.pos+=4; m.pos+=4;
    }
    if(getBit(m.navDataBlockBitmask, 28))
    {
        //qDebug() << "Have raw accel in vessel frame";
        //processVesselAccel();
        // Three floats
        m.pos+=4; m.pos+=4; m.pos+=4;
    }
    if(getBit(m.navDataBlockBitmask, 29))
    {
        //qDebug() << "Have acceleration std dev in vessel frame";
        //processVesselAccelStdDev();
        m.pos+=4; m.pos+=4; m.pos+=4;
    }
    if(getBit(m.navDataBlockBitmask, 30))
    {
        //qDebug() << "Have vessel rotation rate std dev";
        //processVesselRotationRateStdDev();
        m.pos+=4; m.pos+=4; m.pos+=4;
    }

    // Extended Navigational Data Block
    if( (m.extendedNavDataBlockBitmask != 0x00000007) && ( m.extendedNavDataBlockBitmask!= 0x00000000) ) {
        printf("Warning, unusual data\n");
    }

    if(getBit(m.extendedNavDataBlockBitmask, 0))
    {
        // processExtendedRotationAccelData();
        m.pos+=4; m.pos+=4; m.pos+=4;
    }
    if(getBit(m.extendedNavDataBlockBitmask, 1))
    {
        //processExtendedRotationAccelStdDevData();
        m.pos+=4; m.pos+=4; m.pos+=4;
    }
    if(getBit(m.extendedNavDataBlockBitmask, 2))
    {
        //processExtendedRawRotationRateData();
        m.pos+=4; m.pos+=4; m.pos+=4;
    }

    // exteRNAL sensor data blocks.
    if(getBit(m.externDataBitMask, 0))
    {
        // UTC
        m.UTCdataValidityTime = getDWord(mbuf, m.pos); m.pos+=4;
        m.UTCSource = (unsigned char)mbuf[m.pos]; m.pos++;
        // Make a string to help later:
        uint64_t t = m.UTCdataValidityTime;
        m.UTC_hour = t / ((float)1E4)/60.0/60.0;
        m.UTC_minute = ( t / ((float)1E4)/60.0 ) - (m.UTC_hour*60) ;
        m.UTC_second = ( t / ((float)1E4) ) - (m.UTC_hour*60.0*60.0) - (m.UTC_minute*60.0);
        sprintf(m.UTCString, "%02d:%02d:%02.3f",
                m.UTC_hour, m.UTC_minute, m.UTC_second);
        m.haveUTC = true;
    } else {
        m.haveUTC = false;
    }
    if(getBit(m.externDataBitMask, 1))
    {
        // GNSS 1 status
        //        processGNSS(1);
        //        m.haveGNSSInfo1 = true;
    }
    if(getBit(m.externDataBitMask, 2))
    {
        // GNSS 2 status
        // processGNSS(2);
        // m.haveGNSSInfo2 = true;
    }
    if(getBit(m.externDataBitMask, 3))
    {
        // GNSS Manual status "3"
        // processGNSS(3);
        // m.haveGNSSInfo3 = true;
    }


    return m;
}


bool checkFile(char* buf, size_t length) {
    // Check a loaded file of specified length.
    telegram_t m;
    dword counter = 0;
    dword priorCounter = 0;
    size_t pos = 0;
    size_t skips = 0;
    size_t gaps = 0;
    size_t duplicates = 0;
    size_t invalidChecksums = 0;
    size_t invalidMessages = 0;
    size_t totalValidMessagesRead = 0;
    size_t sequenceErrors = 0;
    size_t reboots = 0;

    bool printAllMessages = false; // Lots of text

    dword priorNavDataValidityTime = 0;
    dword deltaNDVT = 0;

    bool keepReading = true;
    bool isPerfect = true;
    while(keepReading) {
        if(pos >= length) {
            goto summary;
            //return isPerfect;
        }
        m = readHeader(buf+pos, length);
        if(m.checksumPassed) {
            if(m.valid) {
                if( (totalValidMessagesRead==0) || (printAllMessages)) {
                    // First time, print info:
                    printf("--- First message: \n");
                    m = parseNavData(buf, pos, m);
                    printMessageData(m);
                    printf("--- end message --\n");
                    priorNavDataValidityTime = m.navDataValidityTime;
                } else {
                    deltaNDVT = m.navDataValidityTime - priorNavDataValidityTime;
                    //printf("%u,", deltaNDVT);
                    if( (deltaNDVT != 50) && (deltaNDVT !=51)) {
                        printf("\n---DeltaNDVT != 50 && != 51 ---\n");
                        printf("Prior: %u, Current: %u, deltaNDVT: %u\n",
                               priorNavDataValidityTime,
                               m.navDataValidityTime,
                               deltaNDVT);
                        printf("Message: \n");
                        printMessageData(m);
                    }
                    priorNavDataValidityTime = m.navDataValidityTime;
                }
                //parseNavData(buf, pos, m);
                totalValidMessagesRead++;
                counter = m.counter;
                pos = pos + m.totalTelegramSize + m.offsetBytes;
                if( (priorCounter+1 == counter) || (priorCounter == 0) ) {
                    // We're good, keep going.
                } else if (priorCounter == counter) {
                    // duplicate
                    duplicates++;
                    fprintf(stderr, "DUP Error, counter: %u, \t" \
                            "priorCounter: %u, \t"\
                            "duplicates: %zu\n",
                            counter, priorCounter,\
                            duplicates);
                    isPerfect = false;
                } else if (counter < priorCounter) {
                    if(priorCounter - counter < 100) {
                        // out of sequence: (but the error is less than half a second of data)
                        fprintf(stderr, "SEQUENCE error, c: %u, prior counter: %u\n", counter, priorCounter);
                        sequenceErrors++;
                    } else {
                        // Likely Atlans reboot as the new data is much "newer" than it ought to be
                        fprintf(stderr, "REBOOT at counter:  %u\n", counter);
                        reboots++;
                    }
                } else {
                    skips++;
                    gaps += (counter - priorCounter - 1);
                    fprintf(stderr, "GAP Error, counter: %u, \t" \
                            "priorCounter: %u, \t" \
                            "total skips: %zu, \t" \
                            "total missing messages: %zu\n",\
                            counter, priorCounter,\
                            skips, gaps);
                    isPerfect = false;
                }
                priorCounter = counter;
            } else {
                if(m.readEntireFile) {
                    fprintf(stderr, "File finished with errors.\n");
                    isPerfect = false;
                    goto summary;
                } else {
                    fprintf(stderr, "Message marked as invalid.\n");
                    isPerfect = false;
                    invalidMessages++;
                }
            }
        } else {
            fprintf(stderr, "Invalid checksum at counter %u\n", m.counter);
            invalidChecksums++;
            // Note, we may miss a telegram due to bad
            // info from the corrupted telegram.
            pos = pos + m.totalTelegramSize + m.offsetBytes;
        }
    }

summary:
    printf("---- Summary of file: ----\n");
    printf( "Last counter: %u,\n"\
           "total skips: %zu,\n" \
           "total missing messages: %zu,\n" \
           "total duplicate messages: %zu\n" \
           "total reboots: %zu\n" \
           "total out of sequence: %zu\n" \
           "total valid messages read: %zu\n",\
           counter, skips, gaps, duplicates, reboots, sequenceErrors, totalValidMessagesRead);
    printf( "total invalud messages: %zu\n", invalidMessages);
    printf( "total bad checksums: %zu\n", invalidChecksums);
    return isPerfect;
}

char * loadFile(char* filename, size_t *length) {
    // Loads a file into memory. Returns pointer to memery.
    // Calling function is responsible to free the memory.
    unsigned long primarySize = 0;
    char *binBuffer = NULL;

    printf("Opening binary file %s\n", filename);
    FILE *fdPrimary = fopen(filename, "rb");
    if(fdPrimary==NULL) {
        fprintf(stderr, "Error, file is null.\n");
        return NULL;
    }

    fseek(fdPrimary, 0, SEEK_END);
    primarySize = ftell(fdPrimary);
    fseek(fdPrimary, 0, SEEK_SET);

    binBuffer = (char*)calloc(1, primarySize+1);

    if(!binBuffer) {
        fprintf(stderr, "Error, could not allocate for buffer. We might be out of memory.\n");
        fclose(fdPrimary);
        return NULL;
    } else {
        fread(binBuffer, primarySize, 1, fdPrimary);
        printf("Read %lu bytes from binary file.\n", primarySize);
    }

    *length = primarySize;

    fclose(fdPrimary);
    return binBuffer;
}

int main(int argc, char* argv[]) {

    bool hasSecondaryFile = false;
    if(argc < 3) {
        hasSecondaryFile = false;
        //fprintf(stderr, "Error, requires two arguments, primaryLog and secondaryLog\n");
        //fprintf(stderr, "Secondary log may use wildcards, ie, AV3*_gps\n");
        //return -1;
    } else {
        hasSecondaryFile = true;
    }

    bool checkEverySecondaryFile = true;

    unsigned long primarySize = 0;
    unsigned long secondarySize = 0;

    char* primaryBuffer = NULL;
    char* secondaryBuffer = NULL;

    primaryBuffer = loadFile(argv[1], &primarySize);
    printf("Primary file loaded, checking file...\n");
    bool primaryFilePerfect = checkFile(primaryBuffer, primarySize);
    if(primaryFilePerfect) {
        printf("Primary File: PASS\n");
    } else {
        fprintf(stderr, "Primary file: FAIL\n");
    }

    if(!hasSecondaryFile)
    {
        if(primaryFilePerfect)
            return 0;
        else
            return -1;
    }
    printf("\n\n");

    char* match = NULL;
    int numMatches = 0;
    int numNotMatch = 0;
    int numFiles = 0;
    int passedSecondaryFiles = 0;
    int failedSecondaryFiles = 0;

    for(int f=0; f < argc-2; f++) {

        secondaryBuffer = loadFile(argv[2+f], &secondarySize);
        if(secondaryBuffer == NULL) {
            printf("Skipping null file %s\n", argv[2+f]);
        } else {
            printf("Searching for file %s within primary log...\n", argv[2+f]);
            match = bstrstr(primaryBuffer, primarySize,  secondaryBuffer, secondarySize);
            printf("Done searching. Results:\n");
            if(match==NULL) {
                printf("Match returned NULL, secondary file is not inside primary buffer.\n");
                numNotMatch++;
            } else {
                printf("Match FOUND, secondary file is inside primary file.\n");
                printf("Location is %zu bytes within the primary file.\n", (size_t)(match-primaryBuffer));
                if(checkEverySecondaryFile)
                {
                    printf("Checking secondary file...\n");
                    bool passed = checkFile(secondaryBuffer, secondarySize);
                    if(passed)
                    {
                        printf("Passed.\n\n");
                        passedSecondaryFiles++;
                    } else {
                        printf("Failed.\n\n");
                        failedSecondaryFiles++;
                    }
                }
                numMatches++;
            }

            //printf("Primary buffer:   %p,\nSecondary buffer: %p,\nmatch:            %p\n", primaryBuffer, secondaryBuffer, match);
            numFiles++;
            free(secondaryBuffer);
        }
    }
    free(primaryBuffer);
    printf("\n---- ---- ----\n");
    if(primaryFilePerfect)
        printf("Primary log PASSED all checks.\n");
    else
        printf("Primary log FAILED some checks, see above for details.\n\n");
    printf("Summary for primary log [%s] to secondary log(s) match detection:\n", argv[1]);
    printf("Matches:       %d\n", numMatches);
    printf("Unmatched:     %d\n", numNotMatch);
    printf("2nd files:     %d (total)\n", numFiles);
    if(checkEverySecondaryFile) {
        printf("Summary of secondary log checks:\n");
        printf("Passed files:  %d\n", passedSecondaryFiles);
        printf("Failed files:  %d\n", failedSecondaryFiles);
    }

    // Return -1 if last file did not match. Useful generally when used without wildcards.
    if(match==NULL)
        return -1;
    return 0;
}
