#ifndef STARTUPOPTIONS_H
#define STARTUPOPTIONS_H

#include <QString>

struct startupOptions_t {
    bool automaticMode = false;
    bool logWithInterval = false;
    bool debugMode = false;
    bool headless = false;

    bool handleZupt = false;

    bool haveDataStorageLocation = false;
    QString dataStorageLocation;

    bool haveGPSIPAddress = false;
    QString gpsIP;

    bool haveGPSPort = false;
    int gpsPort=8112;

    bool haveLoggingInterval = false;
    int loggingIntervalMinutes = 60;

};





#endif // STARTUPOPTIONS_H
