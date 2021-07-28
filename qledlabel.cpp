#include "qledlabel.h"
#include <QDebug>

static const int SIZE = 16;
static const QString greenSS = QString("color: white;border-radius: %1;background-color: qlineargradient(spread:pad, x1:0.145, y1:0.16, x2:1, y2:1, stop:0 rgba(20, 252, 7, 255), stop:1 rgba(25, 134, 5, 255));").arg(SIZE / 2);
//static const QString redSS = QString("color: white;border-radius: %1;background-color: qlineargradient(spread:pad, x1:0.145, y1:0.16, x2:0.92, y2:0.988636, stop:0 rgba(255, 12, 12, 255), stop:0.869347 rgba(103, 0, 0, 255));").arg(SIZE / 2);
//static const QString orangeSS = QString("color: white;border-radius: %1;background-color: qlineargradient(spread:pad, x1:0.232, y1:0.272, x2:0.98, y2:0.959773, stop:0 rgba(255, 113, 4, 255), stop:1 rgba(91, 41, 7, 255))").arg(SIZE / 2);
static const QString blueSS = QString("color: white;border-radius: %1;background-color: qlineargradient(spread:pad, x1:0.04, y1:0.0565909, x2:0.799, y2:0.795, stop:0 rgba(203, 220, 255, 255), stop:0.41206 rgba(0, 115, 255, 255), stop:1 rgba(0, 49, 109, 255));").arg(SIZE / 2);


// Rounded squares:
static const QString orangeSS = QString("height: 16px;\
                                      width: 16px;\
                                      background-color: #FA6900;\
                                      border-radius: 2px;\
                                      position: relative;\
                                      position: absolute;\
                                      top: 0;\
                                      bottom: 0;\
                                      left: 0;\
                                      right: 0;\
                                      content: \"\274c\";\
                                      font-size: 16px; \
                                      color: #FFF;\
                                      line-height: 16px;\
                                      text-align: center;\
                                  ");


static const QString redSS = QString("height: 16px;\
                                    width: 16px;\
                                    background-color: #FF0000;\
                                    border-radius: 2px;\
                                    position: relative;\
                                    position: absolute;\
                                    top: 0;\
                                    bottom: 0;\
                                    left: 0;\
                                    right: 0;\
                                    content: \"\274c\";\
                                    font-size: 16px; \
                                    color: #FFF;\
                                    line-height: 16px;\
                                    text-align: center;\
                                ");

QLedLabel::QLedLabel(QWidget* parent) :
    QLabel(parent)
{
    //Set to ok by default
    this->setText("");
    setState(StateOkBlue);
    setFixedSize(SIZE, SIZE);
}

void QLedLabel::setState(State state)
{
    switch (state) {
    case StateOk:
        setStyleSheet(greenSS);
        this->setText("");
        break;
    case StateWarning:
        setStyleSheet(orangeSS);
        this->setText("❌");
        break;
    case StateError:
        setStyleSheet(redSS);
        this->setText("❌");
        break;
    case StateOkBlue:
    default:
        setStyleSheet(blueSS);
        this->setText("");
        break;
    }
}

void QLedLabel::setState(bool state)
{
    setState(state ? StateOk : StateError);
}
