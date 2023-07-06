# gpsGUI
Graphical User Interface and Binary Reading Library for the Atlans A7 GNSS INS

The gpsGUI program provides both a graphical application to show live and "playback" data, as well as a set of libraries which can be used by other programs to make connections to an Atlans A7 GPS IMU as well as to playback binary recordings.

# Build Requirements:

gpsGUI requires the following libraries and programs:

1. QtCore (tested with version 5.12.0, others are probably fine)
2. QtNetwork
3. QtQml
4. QtGui
5. QtQuick
6. QtWidgets
7. QtQuickWidgets
8. QtPrintSupport
9. QtSvg
10. qmake
11. gcc

# Build Directions

1. Clone the repository. It will create a folder called "gpsGUI". Do not cd to that directory, we will build from a separate folder in the parent directory. 
2. mkdir build
3. cd build
4. qmake ../gpsGUI/gpsgui.pro
5. make

# Usage

Under the Setup tab, you can either supply hostname and port number and press Connect, or, you can select a Binary Log file for replay by pressing "Select..." and then pressing "Replay GPS Log". 

The replay speed may be increased by using the "Playback speedup factor" adjustment. Speeds higher than 3 may cause the GUI to become slugish. Widget redraws may be toggled on the Main page. Future versions will address this automatically. 

# Export statement: 
"Copyright 2021, by the California Institute of Technology. ALL RIGHTS RESERVED. United States Government Sponsorship acknowledged. Any commercial use must be negotiated with the Office of Technology Transfer at the California Institute of Technology.

This software may be subject to U.S. export control laws. By accepting this software, the user agrees to comply with all applicable U.S. export laws and regulations. User has the responsibility to obtain export licenses, or other export authority as may be required before exporting such information to foreign countries or providing access to foreign persons."
