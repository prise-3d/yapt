//
// Created by franck on 17/12/24.
//

#ifndef VORONOICELLITEM_H
#define VORONOICELLITEM_H

#include "qtvor/utils.h"
#include <qgraphicsitem.h>
#include <QGraphicsView>
#include <QPen>

class VoronoiCellItem : public QGraphicsPolygonItem {
public:
    VoronoiCellItem(const QPolygonF &polygon, const QBrush &brush, const QPointF &site,
                    const qreal ellipseDiameter, QGraphicsItem *parent = nullptr)
        : QGraphicsPolygonItem(polygon, parent),
          originalBrush(brush),
          sitePoint(site),
          ellipseDiameter(ellipseDiameter) {
        setBrush(brush);
        setAcceptHoverEvents(true);

        const qreal ellipseRadius = ellipseDiameter / 2.0;
        ellipseItem = new QGraphicsEllipseItem(site.x() - ellipseRadius,
                                               site.y() - ellipseRadius,
                                               ellipseDiameter,
                                               ellipseDiameter,
                                               this);
        ellipseItem->setBrush(QBrush(Qt::red));
        ellipseItem->setPen(QPen(Qt::NoPen));
    }

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override {
        setBrush(QBrush(Qt::yellow));
        QGraphicsPolygonItem::hoverEnterEvent(event);

        auto views = scene()->views();
        if (!views.isEmpty()) {
            if (QGraphicsView *view = views.first()) {
                view->setWindowTitle(QString("Voronoi Cell - site = (%1, %2)").arg(sitePoint.x()).arg(sitePoint.y()));
            }
        }
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override {
        setBrush(originalBrush);
        QGraphicsPolygonItem::hoverLeaveEvent(event);
    }

private:
    QBrush originalBrush;
    QPointF sitePoint;
    qreal ellipseDiameter;
    QGraphicsEllipseItem *ellipseItem;
};

inline void displayVoronoi(Scene yaptScene, int x, int y) {
    auto *qScene = new QGraphicsScene();

    auto ag = yaptScene.camera->renderPixel(*yaptScene.content, *yaptScene.lights, y, x);

    auto aggregator = std::dynamic_pointer_cast<VoronoiAggregator>(ag);

    if (aggregator == nullptr) {
        return;
    }

    auto delaunay = aggregator->delaunay;
    auto voronoi = aggregator->voronoi;

    QPen voronoiPen(Qt::blue);
    voronoiPen.setWidthF(.0015);

    QPen pointPen(Qt::red);
    pointPen.setWidthF(.0025);

    for (auto vertex = delaunay.finite_vertices_begin(); vertex != delaunay.finite_vertices_end(); ++vertex) {
        constexpr qreal ellipse_diameter = .003;
        Point &site = vertex->point();
        if (site.x() < -.5 || site.x() >= .5 || site.y() < -.5 || site.y() >= .5) {
            auto ellipseItem = new QGraphicsEllipseItem(site.x() - ellipse_diameter / 2,
                                       site.y() - ellipse_diameter / 2,
                                       ellipse_diameter,
                                       ellipse_diameter);

            ellipseItem->setBrush(QBrush(Qt::darkRed));
            ellipseItem->setPen(QPen(Qt::NoPen));
            qScene->addItem(ellipseItem);

            continue;
        }

        Face_handle face = voronoi.dual(vertex);

        Ccb_halfedge_circulator halfEdge = face->ccb(), done(halfEdge);

        std::vector<QPointF> polygon;

        do {
            Point p = halfEdge->source()->point();
            polygon.emplace_back(p.x(), p.y());
            ++halfEdge;
        } while (halfEdge != done);

        auto qPolygon = QPolygonF(QVector<QPointF>(polygon.begin(), polygon.end()));

        auto *cellItem = new VoronoiCellItem(
            qPolygon, QBrush(toQColor(aggregator->contributions[aggregator->pointToIndex[site]])),
            QPointF(site.x(), site.y()),
            ellipse_diameter);
        cellItem->setPen(voronoiPen);
        qScene->addItem(cellItem);
    }

    qScene->addRect(-.5, -.5, (1.), (1.), pointPen);

    QGraphicsView *popupView = new ZoomableGraphicsView();
    popupView->setScene(qScene);
    popupView->setRenderHint(QPainter::Antialiasing);
    popupView->setWindowTitle(QString("Voronoi (%1; %2)").arg(x).arg(y));
    popupView->resize(800, 800);
    popupView->setAttribute(Qt::WA_DeleteOnClose);
    popupView->fitInView(QRectF(-0.55, -0.55, 1.1, 1.1), Qt::KeepAspectRatio);
    popupView->setDragMode(QGraphicsView::ScrollHandDrag);
    popupView->viewport()->setCursor(Qt::ArrowCursor);
    popupView->show();
}
#endif //VORONOICELLITEM_H
