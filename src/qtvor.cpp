//
// Created by franck on 13/12/24.
//


#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>

#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QPen>
#include <QBrush>

#include "sampler.h"
#include "utils.h"

#include <QGraphicsView>
#include <QWheelEvent>

#include "aggregators.h"

class VoronoiCellItem : public QGraphicsPolygonItem {
public:
  VoronoiCellItem(const QPolygonF& polygon, const QBrush& brush, const QPointF& site,
                  const qreal ellipseDiameter, QGraphicsItem* parent = nullptr)
      : QGraphicsPolygonItem(polygon, parent),
        originalBrush(brush),
        sitePoint(site),
        ellipseDiameter(ellipseDiameter)
  {
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
  void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override {
    setBrush(QBrush(Qt::yellow));
    QGraphicsPolygonItem::hoverEnterEvent(event);
  }

  void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override {
    setBrush(originalBrush);
    QGraphicsPolygonItem::hoverLeaveEvent(event);
  }

private:
  QBrush originalBrush;
  QPointF sitePoint;
  qreal ellipseDiameter;
  QGraphicsEllipseItem* ellipseItem;
};

class ZoomableGraphicsView : public QGraphicsView {
public:
  explicit ZoomableGraphicsView(QWidget* parent = nullptr)
      : QGraphicsView(parent)
      , scaleFactor(1.15) {}
protected:
  void wheelEvent(QWheelEvent* event) override {
    if (event->angleDelta().y() > 0) {
      scale(scaleFactor, scaleFactor);
    } else {
      scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
  }
private:
  double scaleFactor;
};

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Delaunay_triangulation_2<K> Delaunay_triangulation;
typedef K::Point_2 Point;
typedef K::Segment_2 Segment;

int main(int argc, char** argv) {

  SkewedPPPSamplerFactory f(1000, .999);
  // ClippedStratifiedSamplerFactory f(64);
  auto sampler = f.create(0, 0);

  sampler->begin();

  randomSeed(1000);

  QApplication app(argc, argv);

  std::vector<Point> points;

  while (sampler->hasNext()) {
    auto [x, y, dx, dy] = sampler->get();
    points.emplace_back(dx, dy);
  }

  Delaunay_triangulation delaunay;
  delaunay.insert(points.begin(), points.end());

  QGraphicsScene scene;
  QPen pointPen(Qt::red);
  pointPen.setWidthF(.0025);

  QPen voronoiPen(Qt::blue);
  voronoiPen.setWidthF(.0015);

  Voronoi voronoi(delaunay);

  for (auto vertex = delaunay.finite_vertices_begin() ; vertex != delaunay.finite_vertices_end() ; ++vertex) {
    constexpr qreal ellipse_diameter = .003;
    Point site = vertex->point();
    if (site.x() < -.5 || site.x() >= .5 || site.y() < -.5 || site.y() >= .5) continue;

    Face_handle face = voronoi.dual(vertex);

    Ccb_halfedge_circulator halfEdge = face->ccb(), done(halfEdge);

    std::vector<QPointF> polygon;

    do {
      Point p = halfEdge->source()->point();
      polygon.emplace_back(p.x(), p.y());
      ++halfEdge;
    } while (halfEdge != done);

    auto qPolygon = QPolygonF(QVector<QPointF>(polygon.begin(), polygon.end()));

    QColor color = QColor::fromRgb(100, 150, 200, 128);

    auto* cellItem = new VoronoiCellItem(
      qPolygon, QBrush(color),
      QPointF(site.x(), site.y()),
      ellipse_diameter);
    cellItem->setPen(voronoiPen);
    scene.addItem(cellItem);
  }

  scene.addRect(-.5, -.5, (1.), (1.), pointPen);

  ZoomableGraphicsView view;
  view.setDragMode(QGraphicsView::ScrollHandDrag);
  view.setScene(&scene);
  view.setRenderHint(QPainter::Antialiasing);
  view.setWindowTitle("Voronoi Diagram");
  view.resize(600, 600);
  QRectF initialRect(-0.6, -0.6, 1.2, 1.2);
  view.fitInView(initialRect, Qt::KeepAspectRatio);
  view.setDragMode(QGraphicsView::ScrollHandDrag);
  view.show();

  return app.exec();
}
