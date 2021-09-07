#include "gpsbinarylogger.h"

gpsBinaryLogger::gpsBinaryLogger()
{

    idealBufferSize = 100; // messages to buffer before writing to file
    messageCount = 0; // lifetime message counter
    // rollover is after 2924712086 years at 200 Hz.
    setupBuffer();
    loggingAllowed = false;
    fileIsOpen = false;
    amWritingFile = false;
}

gpsBinaryLogger::~gpsBinaryLogger()
{
    // Write out the rest of the buffer:
    writeBufferToFile();
    // Close file (and wait for any file i/o buffering to complete):
    closeFileWriting();

    // done
}

void gpsBinaryLogger::setPrimaryLogStatus(bool isPrimaryLog)
{
    this->isPrimaryLog = isPrimaryLog;
    if(isPrimaryLog)
    {
        logRankingStr = "Primary Log: ";
    } else {
        logRankingStr = "Secondary Log: ";
    }
}

void gpsBinaryLogger::startLogging()
{
    // Entry point for primary logging
    // Assumes filename has been set.
    openFileWriting();
    if(lastFileIOError==0)
    {
        loggingAllowed = true;
    } else {
        emit haveStatusMessage(logRankingStr + QString("Error, logging not allowed. lastFileIOError: %1").arg(lastFileIOError));
    }
}

void gpsBinaryLogger::stopLogging()
{
    if(!closingLogOut)
    {
        closingLogOut = true;
        loggingAllowed = false;
        usleep(100000); // wait long enough that whatever is in queue finished

        //emit haveStatusMessage(logRankingStr + QString("About to stop logging."));
        //emit haveStatusMessage(logRankingStr + QString("Finishing buffer to file."));
        //writeBufferToFile(); // finish writing if needed
        //emit haveStatusMessage(logRankingStr + QString("Wrote buffer. Closing file."));
        closeFileWriting(); // close file
        //emit haveStatusMessage(logRankingStr + QString("Closed file."));
        //emit haveStatusMessage(logRankingStr + QString("Finished with stopLogging function."));
    } else {
        emit haveStatusMessage(logRankingStr + QString("stopLogging(): Logging already stopped."));
    }
}

void gpsBinaryLogger::beginLogToFilenameNow(QString filename)
{
    // This function will set the filename, stop any current logging,
    // and begin logging to the new filename.

    if(fileIsOpen)
    {
        this->stopLogging(); // be careful
    }
    closingLogOut = false;
    this->setFilename(filename);
    this->startLogging();
}

void gpsBinaryLogger::setupBuffer()
{
    std::lock_guard<std::mutex>lock(bufferSetupMutex);
    buffer.reserve(idealBufferSize);
}

void gpsBinaryLogger::openFileWriting()
{
    if(!fileIsOpen)
    {
        if(filename.isEmpty())
        {
            emit haveStatusMessage(logRankingStr + QString("Error, filename string empty, cannot write GPS data to file!!"));
            loggingAllowed = false;
            // do things
            return;
        }
        if(fileWritePtr != NULL)
        {
            emit haveStatusMessage(logRankingStr + QString("Error, file pointer was already open!!"));
            return;
        }
        fileWritePtr = fopen(filename.toStdString().c_str() ,"ab"); // Append mode, for safety.
        if(fileWritePtr == NULL)
        {
            emit haveStatusMessage(logRankingStr + QString("Error, filename pointer is NULL, cannot write GPS data to file [%1]!!").arg(filename));
            // do things
            return;
        }
        emit haveStatusMessage(logRankingStr + QString("Opened gps binary log file [%1] successfully for binary append write.").arg(filename));
        fileIsOpen = true;
    } else {
        emit haveStatusMessage(logRankingStr + "Error, file was already open! Expect bad things!");
    }
}

void gpsBinaryLogger::closeFileWriting()
{
    if(fileIsOpen)
    {
        std::lock_guard<std::mutex>lockfile(fileCloseMutex);
        if(fileWritePtr != NULL)
        {
            int rtnValue = fclose(fileWritePtr);
            fileIsOpen = false;
            //lastFileIOError = ferror(fileWritePtr); // May cause crash after file is closed, do not use
            if(rtnValue != 0)
            {
                emit haveStatusMessage(logRankingStr + QString("Warning, gps binary logger closed file with status %1 and error %2.").arg(rtnValue).arg(lastFileIOError));
            } else {
                // If the fclose function returns zero, then there is not a file error,
                // and, any reported error at this point is not meaningful.
                // Therefore, we clear the errors:
                //clearerr(fileWritePtr); // may cause crash, do not use.
                lastFileIOError = 0;
            }
            fileWritePtr = NULL;
            //emit haveStatusMessage(logRankingStr + "Closed file.");
        } else {
            // class destructor calls here too, maybe disable warning.
            emit haveStatusMessage(logRankingStr + QString("Closing GPS binary log file: Warning, file pointer was already NULL!!"));
        }
    } else {
        emit haveStatusMessage(logRankingStr + "Error: asked to close file, but file is already closed!");
    }
}

void gpsBinaryLogger::writeBufferToFile()
{

    if(!fileIsOpen)
    {
        emit haveStatusMessage(logRankingStr + "Error, file was not open, could not write buffer.");
        return;
    }

    // Logging being allowed or not should not preclude finishing dumping the buffer to a file:
    //    if(loggingAllowed==false)
    //    {
    //        emit haveStatusMessage(logRankingStr + QString("Error, gps binary logging is disabled, cannot write data to file!!"));
    //        return;
    //    }

    if(fileWritePtr == NULL)
    {
        emit haveStatusMessage(logRankingStr + QString("Error, filename pointer is NULL, cannot write GPS data to file!!"));
        // do things
        loggingAllowed = false; // must re-setup the logging, the pointer will not become valid automatically.
        return;
    }

    // This is a simplistic lock.
    bool usedWait = false;
    while(amWritingFile)
    {
        usleep(200);
        usedWait = true;
    }

    if(usedWait)
    {
        // This has never actually happened, but I'd like to catch it if it does.
        emit haveStatusMessage(logRankingStr + "Warning: Used waiting function when writing to log file.");
    }

    std::lock_guard<std::mutex>lockfile(fileWriteMutex);
    std::lock_guard<std::mutex>lockbuffer(bufferReadAndClearMutex);



    uint16_t bufSize = buffer.size();
    if(bufSize)
    {
        amWritingFile = true;
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
            //temp = QByteArray("\xC0\xFF\xEE\xBA\xBE\x00\x00\x44\x44\x88\x88\xDD\xDD", 13);
            size = temp.size();
            if(size != 0)
            {
                if(fileWritePtr != NULL)
                {
                    nWritten = fwrite(temp.constData(), size,1,fileWritePtr);
                    nWritten = 1;
                    if(nWritten != 1)
                    {
                        lastFileIOError = ferror(fileWritePtr);
                        emit haveStatusMessage(QString(logRankingStr + "Error, complete GPS byte array not written to file [%1]!! Error number: %2").arg(filename).arg(lastFileIOError));
                        emit haveFileIOError(lastFileIOError);
                        // Clear buffer? Self destruct?
                        // Maybe keep the buffer in case the operator fixes the issue, such as a full disk or bad filename
                        //amWritingFile = false;
                        break;
                    }
                } else {
                    emit haveStatusMessage(logRankingStr + QString("Error, cannot write to NULL pointer."));
                }
            }
        }
        lastFileIOError = ferror(fileWritePtr);
        if(lastFileIOError)
        {
            emit haveStatusMessage(logRankingStr + QString("Error writing binary GPS data to file [%1]!! Error number: %2").arg(filename).arg(lastFileIOError) );
            emit haveFileIOError(lastFileIOError);
        }

        buffer.clear(); // TODO: Only do this if no errors?

        amWritingFile = false;
    }

}



void gpsBinaryLogger::insertData(QByteArray raw)
{
    if(closingLogOut)
    {
        // This happens all the time, and also happens after the log is closed out
        // when we are not logging scene data.
        //emit haveStatusMessage(logRankingStr + "Warning, GPS data received while closing log.");
        return;
    }
    if(raw.isEmpty())
    {
        emit haveStatusMessage(logRankingStr + "Warning, empty data passed to gps logger");
        return;
    }
    if(loggingAllowed && fileIsOpen)
    {
        bool waited=false;
        while(amWritingFile)
        {
            usleep(200);
            waited=true;
        }
        if(waited)
            emit haveStatusMessage(logRankingStr + "Warning, waited on file to write log.");
        std::unique_lock<std::mutex>buffLock(bufferInsertMutex);

        messageCount++;
        buffer.push_back(raw);

        int count = buffer.size();

        buffLock.unlock();
        if(count >= idealBufferSize)
        {
            if(!closingLogOut)
                writeBufferToFile();
        }
    }
}

uint16_t gpsBinaryLogger::getBufferSize()
{
    std::lock_guard<std::mutex>lock(bufferSizeMutex);
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
    } else {
        emit haveStatusMessage(logRankingStr + "Error, filename set was empty.");
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
    emit haveStatusMessage(QString(logRankingStr + "Debug called in gpsBinaryLogger."));
    emit haveStatusMessage(QString(logRankingStr + "Current filename: %1").arg(filename));
    emit haveStatusMessage(QString(logRankingStr + "Current buffer message count: %1").arg(getBufferSize()));
    emit haveFileIOError(lastFileIOError);
    emit haveFilename(filenameSet);
    emit haveLifetimeMessageCount(messageCount);
}

