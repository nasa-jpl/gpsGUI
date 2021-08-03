#include "gpsbinaryfilereader.h"

gpsBinaryFileReader::gpsBinaryFileReader()
{
    filenameSet = false;
    readFirstLine = false;
    fileOpen = false;
    maximumMessageSize = 512; // 391 is max seen
    messageDelayMicroSeconds = 1E6 / 200.0;
    rawData = (char*)malloc(maximumMessageSize); // bytes to hold a message read

}

gpsBinaryFileReader::~gpsBinaryFileReader()
{
    // close file
    closeFile();
    free(rawData);
}

void gpsBinaryFileReader::beginWork()
{
    // entry point from outside.
    keepGoing = true;
    startProcessFile();
}

void gpsBinaryFileReader::stopWork()
{
    qDebug() << "Stopping work";
    keepGoing = false;
}

void gpsBinaryFileReader::startProcessFile()
{
    bool found = false;
    bool ok = true;
    volatile long offset = 0;
    (void)offset;



    openFile();
    if(!fileOpen)
        return;


    //fseek(binFilePtr, 0L, SEEK_SET); // go to beginning of file



    //offset = ftell(binFilePtr); // starting point


    while(ok && keepGoing)
    {

        //qDebug() << "About to try find message.";
        found = findMessage(); // Block while looking through the file
                               // advance file two bytes in the process
        //qDebug() << "finished the findMessage.";
        if(!found)
        {
            // We read the entire file, didn't find anything
            emit haveErrorMessage(QString("Error: read entire file [%1], no (further) GPS data found.").arg(filename));
            return;
        }

        // rewind two bytes (not needed anymore:
        //        offset = ftell(my_file);
        //        fseek(binFilePtr, offset-2, SEEK_CUR);
        // Read the size of the next message from the message header:
        messageSizeBytes = readV5header(); // read the next 27-2 bytes
        //emit haveStatusMessage(QString("Read header for messageCount %1. Telegram size is %2 bytes.").arg(messagesRead).arg(messageSizeBytes)  );

        ok = readRestOfMessage(); // read message bytes, copy into rawData, and set binMessage to point into rawData.

        reader.insertData(QByteArray(binMessage)); // make a copy, just to be safe
        m = reader.getMessage();
        if(m.validDecode)
        {
            emit haveGPSMessage(m);
        } else {
            emit haveErrorMessage(QString("Invalid decode. Message number %1 with counter value %2, error message: [%3], message checksum: %4, calculated message checksum: %5, byte array length: %6, message telegram claimed size: %7")\
                                  .arg(messagesRead).arg(m.counter)\
                                  .arg(m.lastDecodeErrorMessage)\
                                  .arg(m.claimedMessageSum)\
                                  .arg(m.calculatedChecksum)\
                                  .arg(binMessage.length())\
                                  .arg(messageSizeBytes));
            //debugReader.printMessage(m);
            emit haveGPSMessage(m);
        }
        messagesRead++;
        // TODO: Automatically replay at the same speed as the file,
        // using the change in microseconds between messages.
        // First file sets the offset, then we count and wait between.
        usleep(messageDelayMicroSeconds);
    }


    closeFile();
}

bool gpsBinaryFileReader::findMessage()
{
    bool lookingForMessageStart = true;
    size_t nread = 0;
    long offset = 0;
    (void)offset; // keep this variable around for debug.
    volatile uint64_t readCount = 0;
    offset = ftell(binFilePtr);
    //emit haveStatusMessage(QString("findMessage: starting read at offset value: %1").arg(offset));


    // TODO: Add file size checking so as to not read beyond the file
    // TODO: Account for a situation where we read and are off by one byte.
    while(lookingForMessageStart)
    {
        nread = fread(rawData, sizeof(char), 2, binFilePtr);
        offset = ftell(binFilePtr);
        if(nread != 2)
        {
            emit haveErrorMessage(QString("findMessage: Tried to read %1 bytes but read %2 instead. Read %3 times.").arg(2).arg(nread).arg(readCount));
            return false;
        }
        if(strncmp(rawData, "IX", 2) == 0)
        {
            lookingForMessageStart = false;
            //emit haveStatusMessage(QString("findMessage: ***FOUND*** IX at readcount %1, with offset %2.").arg(readCount).arg(offset));
        } else {
            // Scoot back one byte
            //emit haveErrorMessage(QString("findMessage: Found bytes %1 and %2 with attempt %3 at offset %4.").arg(rawData[0]).arg(rawData[1]).arg(readCount).arg(offset));
            fseek(binFilePtr, -1, SEEK_CUR);
        }
        readCount++;
    }
    return !lookingForMessageStart;
}

uint16_t gpsBinaryFileReader::readV5header()
{
    size_t nread = 0; // for use as we read things
    nread = fread(rawData+2, sizeof(char), headerV5sizeBytes-2, binFilePtr);
    uint16_t offset = 0;
    if(nread != (size_t)(headerV5sizeBytes-2))
    {
        emit haveErrorMessage(QString("readV5header: Tried to read %1 bytes but read %2 instead.").arg(headerV5sizeBytes).arg(nread));
        return false;
    }
    // Position in file for total telegram size data (a 2-byte "word"):
    offset = 17;
//    char telegramB0 = rawData[offset];
//    char telegramB1 = rawData[offset+1];

    return (unsigned char)rawData[offset+1] | ((unsigned char)rawData[offset+0] << 8);

}

bool gpsBinaryFileReader::readRestOfMessage()
{
    size_t nread = 0; // for use as we read things
    // The +4 is from the checksum size, which isn't included in the messageSizeBytes per the ICD.
    // +2: move past the I X bytes
    // +header: move past the header which has been read
    // -2: we didn't read the entire header actually, so scoot in two

    // Note: the checksum will fail if we transfer too many bytes in

    size_t nBytesToRead =  messageSizeBytes - headerV5sizeBytes; // the +100 is a debug method to make sure we read enough
    nread = fread(rawData+2+headerV5sizeBytes-2, sizeof(char), nBytesToRead, binFilePtr);
    if(nread != size_t(nBytesToRead))
    {
        emit haveErrorMessage(QString("readRestOfMessage: Tried to read %1 bytes but read %2 instead.").arg(messageSizeBytes+4).arg(nread));
        return false;
    }

    // Note: the checksum will fail if we transfer too many bytes in
    binMessage.setRawData(rawData, messageSizeBytes);

    return true;

}

void gpsBinaryFileReader::setFilename(QString filename)
{
    if(!filename.isEmpty())
    {
        this->filename = filename;
        filenameSet = true;
    }
}

void gpsBinaryFileReader::openFile()
{
    if(filenameSet && (!fileOpen) && (binFilePtr==NULL))
    {
        binFilePtr = fopen(filename.toStdString().c_str() ,"rb");
        lastFileError = ferror(binFilePtr);
        if((binFilePtr == NULL) || lastFileError)
        {
            emit haveFileReadError(lastFileError);
            emit haveErrorMessage(QString("Error opening binary gps file [%1] for reading: %2").arg(filename).arg(lastFileError));
            fileOpen = false;
        } else {
            fileOpen = true;
        }
    }
}

void gpsBinaryFileReader::closeFile()
{
    if(fileOpen && (binFilePtr != NULL))
    {
        int rtnVal = fclose(binFilePtr);
        lastFileError = ferror(binFilePtr);
        if((rtnVal) || lastFileError)
        {
            emit haveFileReadError(lastFileError);
            emit haveErrorMessage(QString("Error opening binary gps file [%1] for reading: %2").arg(filename).arg(lastFileError));
        }

        fileOpen = false;
    }
}
