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
        StateUnknown
    };


signals:

public slots:
    void setState(State state);
    void setState(bool state);
};

#endif // QLEDLABEL_H
