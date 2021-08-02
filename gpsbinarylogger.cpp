#include "gpsbinarylogger.h"

gpsBinaryLogger::gpsBinaryLogger()
{

    idealBufferSize = 100; // messages to buffer before writing to file
    messageCount = 0; // lifetime message counter
    // rollover is after 2924712086 years at 200 Hz.
    setupBuffer();
    loggingAllowed = false;
    fileIsOpen = false;
}

gpsBinaryLogger::~gpsBinaryLogger()
{
    // Write out the rest of the buffer:
    writeBufferToFile();
    // Close file (and wait for any file i/o buffering to complete):
    closeFileWriting();

    // done
}

void gpsBinaryLogger::startLogging()
{
    openFileWriting();
    if(lastFileIOError==0)
    {
        loggingAllowed = true;
    }
}

void gpsBinaryLogger::stopLogging()
{
    writeBufferToFile(); // finish writing if needed
    closeFileWriting(); // close file
    loggingAllowed = false;

}

void gpsBinaryLogger::setupBuffer()
{
    std::lock_guard<std::mutex>lock(bufferMutex);
    buffer.reserve(idealBufferSize);
}

void gpsBinaryLogger::openFileWriting()
{
    if(!fileIsOpen)
    {
        if(filename.isEmpty())
        {
            emit haveStatusMessage(QString("Error, filename string empty, cannot write GPS data to file!!"));
            loggingAllowed = false;
            // do things
            return;
        }
        if(fileWritePtr != NULL)
        {
            emit haveStatusMessage(QString("Error, file pointer was already open!!"));
            return;
        }
        fileWritePtr = fopen(filename.toStdString().c_str() ,"wb");
        if(fileWritePtr == NULL)
        {
            emit haveStatusMessage(QString("Error, filename pointer is NULL, cannot write GPS data to file [%1]!!").arg(filename));
            // do things
            return;
        }
        fileIsOpen = true;
    }
}

void gpsBinaryLogger::closeFileWriting()
{
    if(fileIsOpen)
    {
        std::lock_guard<std::mutex>lockfile(fileMutex);
        if(fileWritePtr != NULL)
        {
            int rtnValue = fclose(fileWritePtr);
            lastFileIOError = ferror(fileWritePtr);
            if(rtnValue != 0)
            {
                emit haveStatusMessage(QString("Warning, gps binary logger closed file with status %1 and error %2.").arg(rtnValue).arg(lastFileIOError));
            }
        } else {
            // class destructor calls here too, maybe disable warning.
            emit haveStatusMessage(QString("Warning, file pointer was already closed!!"));
        }
        fileIsOpen = false;
    }
}

void gpsBinaryLogger::writeBufferToFile()
{
    if(!fileIsOpen)
        return;
    if(loggingAllowed==false)
    {
        emit haveStatusMessage(QString("Error, gps binary logging is disabled, cannot write data to file!!"));
        return;
    }
    if(fileWritePtr == NULL)
    {
        emit haveStatusMessage(QString("Error, filename pointer is NULL, cannot write GPS data to file!!"));
        // do things
        loggingAllowed = false; // must re-setup the logging, the pointer will not become valid automatically.
        return;
    }

    // Each byte array may be of different sizes, thus, we must check each one:
    std::lock_guard<std::mutex>lockfile(fileMutex);
    std::lock_guard<std::mutex>lockbuffer(bufferMutex);

    amWritingFile = true;
    uint16_t bufSize = buffer.size();
    QByteArray temp;
    size_t size = 0;
    size_t nWritten = 0;

    // The following "magicId" can be enabled to aid in debugging and searching the binary file:
    //QByteArray magicId;
    //magicId.setRawData("\xCA\xFE\x00\x11\x22\x33\x44\x55\x66\x77\x88\x99\xaa\xbb\xcc\xdd\xee\xff", 18);

    for(uint16_t i=0; i < bufSize; i++)
    {
        temp = buffer.at(i);
        // TESTING ONLY:
        //temp.append(magicId);
        size = temp.size();
        if(size != 0)
        {
            nWritten = fwrite(temp.constData(), size,1,fileWritePtr);
            if(nWritten != 1)
            {
                lastFileIOError = ferror(fileWritePtr);
                emit haveStatusMessage(QString("Error, complete GPS byte array not written to file [%1]!! Error number: %2").arg(filename).arg(lastFileIOError));
                emit haveFileIOError(lastFileIOError);
                // Clear buffer? Self destruct?
                // Maybe keep the buffer in case the operator fixes the issue, such as a full disk or bad filename
                amWritingFile = false;
                break;
            }
        }
    }
    lastFileIOError = ferror(fileWritePtr);
    if(lastFileIOError)
    {
        emit haveStatusMessage(QString("Error writing binary GPS data to file [%1]!! Error number: %2").arg(filename).arg(lastFileIOError) );
        emit haveFileIOError(lastFileIOError);
    }

    buffer.clear(); // TODO: Only do this if no errors?

    amWritingFile = false;
}



void gpsBinaryLogger::insertData(QByteArray raw)
{
    if(raw.isEmpty())
        return;
    std::unique_lock<std::mutex>buffLock(bufferMutex);

    messageCount++;
    buffer.push_back(raw);

    int count = buffer.size();

    buffLock.unlock();
    if(count >= idealBufferSize)
        writeBufferToFile();
}

uint16_t gpsBinaryLogger::getBufferSize()
{
    std::lock_guard<std::mutex>lock(bufferMutex);
    return buffer.size();
}


void gpsBinaryLogger::setFilename(QString filename)
{
    // This can be set multiple times, however,
    // the file will be opened to the current value of this variable.
    if(!filename.isEmpty())
    {
        this->filename = filename;
        filenameSet = true;
    }
}

void gpsBinaryLogger::getFilenameSetStatus()
{
    emit haveFilename(filenameSet);
}

void gpsBinaryLogger::getLifetimeMessageCount()
{
    emit haveLifetimeMessageCount(messageCount);
}

void gpsBinaryLogger::debugThis()
{
    emit haveStatusMessage(QString("Debug called in gpsBinaryLogger."));
    emit haveStatusMessage(QString("Current filename: %1").arg(filename));
    emit haveStatusMessage(QString("Current buffer message count: %1").arg(getBufferSize()));
    emit haveFileIOError(lastFileIOError);
    emit haveFilename(filenameSet);
    emit haveLifetimeMessageCount(messageCount);
}

