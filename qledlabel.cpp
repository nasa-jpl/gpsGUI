#include "qledlabel.h"
#include <QDebug>

static const int SIZE = 16;

// Reference for QSS (CSS):
// https://doc.qt.io/qt-5/stylesheet-reference.html#list-of-properties



QLedLabel::QLedLabel(QWidget* parent) :
    QLabel(parent)
{
    this->f = this->font();
    this->setText("");
    setupSS(SIZE);
    lastState = QLedLabel::StateUnknown;
    setState(StateOkBlue);
    setFixedSize(SIZE, SIZE);
    this->setAlignment(Qt::AlignCenter);
}

void QLedLabel::setupSS(int dotSize)
{
    greenSS = QString("color: black;text-align: center;border-radius: %1;background-color: qlineargradient(spread:pad, x1:0.145, y1:0.16, x2:1, y2:1, stop:0 rgba(20, 252, 7, 255), stop:1 rgba(25, 134, 5, 255));").arg(dotSize / 2);
    //static const QString redSS = QString("color: white;border-radius: %1;background-color: qlineargradient(spread:pad, x1:0.145, y1:0.16, x2:0.92, y2:0.988636, stop:0 rgba(255, 12, 12, 255), stop:0.869347 rgba(103, 0, 0, 255));").arg(dotSize / 2);
    //static const QString orangeSS = QString("color: white;border-radius: %1;background-color: qlineargradient(spread:pad, x1:0.232, y1:0.272, x2:0.98, y2:0.959773, stop:0 rgba(255, 113, 4, 255), stop:1 rgba(91, 41, 7, 255))").arg(dotSize / 2);
    blueSS = QString("color: black;text-align: center;border-radius: %1;background-color: qlineargradient(spread:pad, x1:0.04, y1:0.0565909, x2:0.799, y2:0.795, stop:0 rgba(203, 220, 255, 255), stop:0.41206 rgba(0, 115, 255, 255), stop:1 rgba(0, 49, 109, 255));").arg(dotSize / 2);


    // Rounded squares:
    orangeSS = QString("height: %1px;\
                                          width: %2px;\
                                          background-color: #FA6900;\
                                          border-radius: 2px;\
                                          position: relative;\
                                          position: absolute;\
                                          top: 0;\
                                          bottom: 0;\
                                          left: 0;\
                                          right: 0;\
                                          font-size: %3px; \
                                          color: #FFF;\
                                          line-height: %4px;\
                                          text-align: center;\
                                          color: black;\
                                      ").arg(dotSize/2).arg(dotSize/2).arg(dotSize).arg(dotSize/2);


    redSS = QString("height: %1px;\
                                        width: %2px;\
                                        background-color: #FF0000;\
                                        border-radius: 2px;\
                                        position: relative;\
                                        position: absolute;\
                                        top: 0;\
                                        bottom: 0;\
                                        left: 0;\
                                        right: 0;\
                                        font-size: %3px; \
                                        color: #FFF;\
                                        line-height: %4px;\
                                        text-align: center;\
                                        color: black;\
                                    ").arg(dotSize/2).arg(dotSize/2).arg(dotSize).arg(dotSize/2);
}

void QLedLabel::setSizeCustom(int s)
{
    //f.setPixelSize(s);
    f.setPointSize(s);
    this->setFont(f);
    setupSS(s);
    setFixedSize(s, s);
    setState(lastState);
}

void QLedLabel::setState(State state)
{
    if(state==lastState)
        return;

    ledLocker.lock();
    switch (state) {
    case StateOk:
        setStyleSheet(greenSS);
        this->setText("✔");
        this->setToolTip("OK");
        break;
    case StateWarning:
        setStyleSheet(orangeSS);
        this->setText("!!");
        this->setToolTip("WARNING");
        break;
    case StateError:
        setStyleSheet(redSS);
        this->setText("❌");
        this->setToolTip("ERROR");
        break;
    case StateUnknown:
        setStyleSheet(orangeSS);
        this->setText("??");
        this->setToolTip("UNKNOWN");
        break;
    case StateOkBlue:
    default:
        setStyleSheet(blueSS);
        this->setText("");
        this->setToolTip("DEFAULT");
        break;
    }
    lastState = state;
    ledLocker.unlock();
}

void QLedLabel::setState(bool state)
{
    setState(state ? StateOk : StateError);
}
