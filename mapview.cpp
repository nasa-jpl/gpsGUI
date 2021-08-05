#include "mapview.h"
#include "ui_mapview.h"

mapView::mapView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mapView)
{
    ui->setupUi(this);
}

mapView::~mapView()
{
    delete ui;
}


void mapView::handleMapUpdatePosition(double lat, double lng){
    QQuickItem *item = ui->mapQuickWidget->rootObject();
    QObject *object;
    object = item->findChild<QObject *>("mapItem");
    QVariant posx = QVariant(lat);
    QVariant posy = QVariant(lng);

    if (object != NULL) {
      QMetaObject::invokeMethod(object, "recenter", Q_ARG(QVariant, posx),
                                Q_ARG(QVariant, posy));
    }
}
