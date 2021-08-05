#ifndef MAPVIEW_H
#define MAPVIEW_H

#include <QWidget>
#include <QtWidgets>
#include <QObject>
#include <QtQuickWidgets/QQuickWidget>
#include <QtQuick/QQuickItem>

namespace Ui {
class mapView;
}

class mapView : public QWidget
{
    Q_OBJECT

public:
    explicit mapView(QWidget *parent = nullptr);
    ~mapView();

public slots:
    void handleMapUpdatePosition(double lat, double lng);

private:
    Ui::mapView *ui;
};

#endif // MAPVIEW_H
