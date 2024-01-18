#ifndef PLOTCOLORS_H
#define PLOTCOLORS_H

#include <QColor>
#include <QVector>

#define maxPlotLines (16)

struct plotColors_t {
    QColor grid = QColor(Qt::white).darker();
    QColor axis = QColor(Qt::white);
    QColor text = QColor(Qt::white);
    QColor legendBkg = QColor("#85000000");
    QColor background = QColor(Qt::black);

    QColor lines[maxPlotLines] = { QColor(Qt::green),
                        QColor(Qt::red),
                        QColor(Qt::blue).lighter(),
                        QColor(Qt::yellow),
                        QColor(Qt::magenta),
                        QColor(Qt::white) };
    QVector <QColor> linesVec;
};


inline QVector <QColor> makeColors(int num)
{
    // Returns a vector of colors, size num, which
    // have good separation.

    int red,green,blue;
    QColor color;
    QVector <QColor> rainbow;
    if(num < 1)
    {
        // Generally this happens when a plot is not setup
        // or has been set with zero members.
        qCritical() << "Cannot make plots of size <= 0";
        return rainbow;
    }
    rainbow.resize(num);

    int angle;

    for(int i=0; i<num; i++)
    {
        angle = (i*360)/num;
        if (angle<60) {red = 255; green = round(angle*4.25-0.01); blue = 0;} else
            if (angle<120) {red = round((120-angle)*4.25-0.01); green = 255; blue = 0;} else
                if (angle<180) {red = 0, green = 255; blue = round((angle-120)*4.25-0.01);} else
                    if (angle<240) {red = 0, green = round((240-angle)*4.25-0.01); blue = 255;} else
                        if (angle<300) {red = round((angle-240)*4.25-0.01), green = 0; blue = 255;} else
                        {red = 255, green = 0; blue = round((360-angle)*4.25-0.01);}
        rainbow[i] = QColor(red, green, blue, 255);
    }
    return rainbow;
}



#endif // PLOTCOLORS_H
