#ifndef QLEDLABEL_H
#define QLEDLABEL_H

#include <QLabel>
#include <QMutex>

class QLedLabel : public QLabel
{
    Q_OBJECT

    QMutex ledLocker;
public:
    explicit QLedLabel(QWidget* parent = 0);

    enum State {
        StateOk,
        StateOkBlue,
        StateWarning,
        StateError,
        StateUnknown,
        StateInitial
    };

    void setSizeCustom(int fontSize);

private:
    int SIZE = 16;
    QFont f;
    QString greenSS;
    QString blueSS;
    QString orangeSS;
    QString redSS;
    State lastState = StateOkBlue;
    void setupSS(int dotSize);

signals:

public slots:
    void setState(State state);
    void setState(bool state);

};

#endif // QLEDLABEL_H
