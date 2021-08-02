#include "gpsbinarylogger.h"

gpsBinaryLogger::gpsBinaryLogger()
{

    bufferSize = 100; // messages to buffer before writing to file
    messageCount = 0; // lifetime message counter
                      // rollover is after 2924712086 years at 200 Hz.
}

gpsBinaryLogger::~gpsBinaryLogger()
{
    // Write out the rest of the buffer:

    // Close file:

    // done
}

void gpsBinaryLogger::setupBuffer()
{
    std::lock_guard<std::mutex>lock(bufferMutex);
    buffer.reserve(bufferSize);
//    QByteArray empty;
//    for(int i=0; i < bufferSize; i++)
//    {
//        buffer.push_back(empty);
//    }
}

void gpsBinaryLogger::openFileWriting()
{
    if(filename.isEmpty())
    {
        emit haveStatusMessage(QString("Error, filename string empty, cannot write GPS data to file!!"));
        // do things
        return;
    }
    fileWritePtr = fopen(filename.toStdString().c_str() ,"wb");
    if(fileWritePtr == NULL)
    {
        emit haveStatusMessage(QString("Error, filename pointer is NULL, cannot write GPS data to file!!"));
        // do things
        return;
    }


}

void gpsBinaryLogger::writeBufferToFile()
{
    if(fileWritePtr == NULL)
    {
        emit haveStatusMessage(QString("Error, filename pointer is NULL, cannot write GPS data to file!!"));
        // do things
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

    for(uint16_t i=0; i < bufSize; i++)
    {
        temp = buffer.at(i);
        size = temp.size() - 1; // Remove null-termination
        if(size != 0)
        {
            nWritten = fwrite(temp.constData(), size,1,fileWritePtr);
            if(nWritten != 1)
            {
                lastFileIOError = ferror(fileWritePtr);
                emit haveStatusMessage(QString("Error, complete GPS byte array not written to file!! Error number: %1").arg(lastFileIOError));
                emit haveFileIOError(lastFileIOError);
                amWritingFile = false;
                break;
            }
        }
    }
    lastFileIOError = ferror(fileWritePtr);
    if(lastFileIOError)
    {
        emit haveStatusMessage(QString("Error writing binary GPS data to file!! Error number: %1").arg(lastFileIOError) );
        emit haveFileIOError(lastFileIOError);
    }
    amWritingFile = false;
}



void gpsBinaryLogger::insertData(QByteArray raw)
{
    std::lock_guard<std::mutex>lock(bufferMutex);
    if(raw.isEmpty())
        return;
    messageCount++;
    buffer.push_back(raw);
}

uint16_t gpsBinaryLogger::getBufferSize()
{
    std::lock_guard<std::mutex>lock(bufferMutex);
    return buffer.size();
}


void gpsBinaryLogger::setFilename(QString filename)
{
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
    emit haveFileIOError(lastFileIOError);
    emit haveFilename(filenameSet);
    emit haveLifetimeMessageCount(messageCount);
}

