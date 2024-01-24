#include "gpsgui.h"

#include <QApplication>

#include "startupoptions.h"

startupOptions_t startupOptions;

bool parseArguments(QStringList args);

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setOrganizationDomain("jpl.nasa.gov");
    a.setOrganizationName("GPSGUI");
    a.setApplicationDisplayName("GPSGUI");
    a.setApplicationName("GPSGUI");

    QStringList arguments = a.arguments();

    if(parseArguments(arguments)) {
        GpsGui w(startupOptions);
        w.show();
        return a.exec();
    } else {
        return -1;
    }


}



bool parseArguments(QStringList args)
{
    int length = args.length();

    if(length == 1)
    {
        // program name is args.at(0)
        return true;
    }

    int position = 0;
    int matches = 0;
    for ( const auto& i : args  )
    {

        QString loweri = i.toLower();

        if(loweri == QString("--datastoragelocation"))
        {
            if( position+1 <= length-1 )
            {
                //printf("Accepting datastoragelocation option.\n");
                startupOptions.haveDataStorageLocation = true;
                startupOptions.dataStorageLocation = args.at(position+1);
                matches += 2;
            } else {
                printf("Got --datastoragelocation without a location specified!\n");
                goto help;
            }
        }

        if(loweri == QString("--gpsip"))
        {
            if( position+1 <= length-1 )
            {
                startupOptions.haveGPSIPAddress = true;
                startupOptions.gpsIP = args.at(position+1);
                matches += 2;
            } else {
                printf("Got --gpsip without an address specified!\n");
                goto help;
            }
        }

        if(loweri == QString("--gpsport"))
        {
            if( position+1 <= length-1 )
            {
                startupOptions.haveGPSPort = true;
                startupOptions.gpsPort = QString("%1").arg(args.at(position+1)).toInt();
                matches += 2;
            } else {
                printf("Got --gpsport without a port specified!\n");
                goto help;
            }
        }


        if( (loweri == QString("--fullautomode")) ||
            (loweri == QString("--fullauto")) ||
            (loweri == QString("-a")) ||
            (loweri == QString("--auto")) ||
            (loweri == QString("--full-auto")))
        {
            startupOptions.automaticMode = true;
            matches++;
        }

        if(loweri == QString("--headless")) {
            startupOptions.headless = true;
            matches++;
        }

        if(loweri == QString("--zupt")) {
            startupOptions.handleZupt = true;
            matches++;
        }

        if(loweri == QString("--logwithinterval")) {
            startupOptions.logWithInterval = true;
            matches++;
        }

        if(loweri == QString("--logginginterval"))
        {
            if( position+1 <= length-1 )
            {
                startupOptions.haveLoggingInterval = true;
                startupOptions.loggingIntervalMinutes = QString("%1").arg(args.at(position+1)).toInt();
                matches += 2;
            } else {
                printf("Got --logginginterval without an interval specified!\n");
                goto help;
            }
        }


        if( (loweri == QString("--debug")) || (loweri == QString("-d")) )
        {
            startupOptions.debugMode = true;
            matches++;
        }

        if( (loweri == QString("-h")) || (loweri == QString("--help")) )
        {
            matches++;
            goto help;
        }
        position++;
    }

    if( (length >= 1) && (matches >= 1) )
    {
        return true;
    } else {
        printf("Could not understand all supplied options.\n");
        goto help;
    }

    help:
    printf("Usage options:\n");
    printf("    --datastoragelocation /path/to/data/storage/location\n");
    printf("    --fullautomode\n");
    printf("    --gpsip 10.2.3.4\n");
    printf("    --gpsport 8112\n");
    printf("    --zupt\n");
    printf("    --logwithinterval\n");
    printf("    --logginginterval 60\n");
    printf("    --headless\n");
    printf("    -platform offscreen\n");
    printf("    --debug\n");
    printf("    --help\n");
    return false;
}
