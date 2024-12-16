//
// Created by franck on 13/12/24.
//


#include "parser.h"

#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QPen>
#include <QBrush>
#include <QGraphicsView>
#include <QWheelEvent>



inline QColor toQColor(const double r, const double g, const double b) {
    static const Interval intensity(0.000, 0.999);
    const double rr = linearToGamma(r);
    const double gg = linearToGamma(g);
    const double bb = linearToGamma(b);

    const int ir = static_cast<int>(256 * intensity.clamp(rr));
    const int ig = static_cast<int>(256 * intensity.clamp(gg));
    const int ib = static_cast<int>(256 * intensity.clamp(bb));

    return {ir, ig, ib};
}

inline QColor toQColor(const Color &color) {
    return toQColor(color.x(), color.y(), color.z());
}

class ZoomableGraphicsView : public QGraphicsView {
public:
    explicit ZoomableGraphicsView(QWidget *parent = nullptr)
        : QGraphicsView(parent)
          , scaleFactor(1.15) {
    }

protected:
    void wheelEvent(QWheelEvent *event) override {
        if (event->angleDelta().y() > 0) {
            scale(scaleFactor, scaleFactor);
        } else {
            scale(1.0 / scaleFactor, 1.0 / scaleFactor);
        }
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        QGraphicsView::mouseReleaseEvent(event);
        if (event->button() == Qt::LeftButton) {
            viewport()->setCursor(Qt::ArrowCursor);
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        // viewport()->setCursor(Qt::ArrowCursor);
        QGraphicsView::mouseMoveEvent(event);
    }

    double scaleFactor;
};

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

void displayVoronoi(Scene yaptScene, int x, int y) {
    QGraphicsScene *qScene = new QGraphicsScene();

    std::cout << "rendering pixel @" << x << ", " << y << std::endl;

    auto ag = yaptScene.camera->renderPixel(*yaptScene.content, *yaptScene.lights, y, x);

    auto aggregator = std::dynamic_pointer_cast<VoronoiAggregator>(ag);

    if (aggregator == nullptr) {
        std::cout << "not a VoronoiAggregator" << std::endl;
        return;
    }

    auto delaunay = aggregator->delaunay;
    auto voronoi = aggregator->voronoi;

    for (auto vertex = delaunay.finite_vertices_begin(); vertex != delaunay.finite_vertices_end(); ++vertex) {
        constexpr qreal ellipse_diameter = .003;
        Point &site = vertex->point();
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

        QPen voronoiPen(Qt::blue);
        voronoiPen.setWidthF(.0015);

        auto *cellItem = new VoronoiCellItem(
            qPolygon, QBrush(toQColor(aggregator->contributions[aggregator->pointToIndex[site]])),
            QPointF(site.x(), site.y()),
            ellipse_diameter);
        cellItem->setPen(voronoiPen);
        qScene->addItem(cellItem);
    }

    QPen pointPen(Qt::red);
    pointPen.setWidthF(.0025);

    qScene->addRect(-.5, -.5, (1.), (1.), pointPen);

    QGraphicsView *popupView = new ZoomableGraphicsView();
    popupView->setScene(qScene);
    popupView->setRenderHint(QPainter::Antialiasing);
    popupView->setWindowTitle("Voronoi");
    popupView->resize(800, 800);
    popupView->setAttribute(Qt::WA_DeleteOnClose);
    popupView->fitInView(QRectF(-0.55, -0.55, 1.1, 1.1), Qt::KeepAspectRatio);
    popupView->setDragMode(QGraphicsView::ScrollHandDrag);
    popupView->viewport()->setCursor(Qt::ArrowCursor);
    popupView->show();
}

class ZoomableImageView : public ZoomableGraphicsView {
public:
    explicit ZoomableImageView(Scene yaptScene, QWidget *parent = nullptr): ZoomableGraphicsView(parent),
                                                                            yaptScene(yaptScene) {
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && (event->modifiers() & Qt::ControlModifier)) {
            const QPointF scenePos = mapToScene(event->pos());

            QList<QGraphicsItem *> itemsAtClick = scene()->items(scenePos);
            for (QGraphicsItem *item: itemsAtClick) {
                auto pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(item);
                if (pixmapItem) {
                    const QPointF localPos = pixmapItem->mapFromScene(scenePos);
                    const int x = static_cast<int>(localPos.x());
                    const int y = static_cast<int>(localPos.y());

                    if (x >= 0 && y >= 0 && x < pixmapItem->pixmap().width() && y < pixmapItem->pixmap().height()) {
                        displayVoronoi(yaptScene, x, y);
                    }

                    return;
                }
            }
        }

        ZoomableGraphicsView::mousePressEvent(event);
    }

    Scene yaptScene;
};

QImage convertToQImage(const ImageData &imageData) {
    QImage image(static_cast<int>(imageData.width), static_cast<int>(imageData.height), QImage::Format_RGB32);

    const int width = imageData.width;
    const int height = imageData.height;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            size_t index = 3 * (y * imageData.width + x);
            auto color = toQColor(
                imageData.data[index],
                imageData.data[index + 1],
                imageData.data[index + 2]
            );

            image.setPixel(x, y, color.rgb());
        }
    }
    return image;
}


typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Delaunay_triangulation_2<K> Delaunay_triangulation;
typedef K::Point_2 Point;
typedef K::Segment_2 Segment;

int main(int argc, char **argv) {
    Parser parser;
    Scene yaptScene;
    if (!parser.parseScene(argc, argv, yaptScene)) return 0;

    yaptScene.camera->render(*yaptScene.content, *yaptScene.lights);

    parser.exportImage(argc, argv, yaptScene);

    QApplication app(argc, argv);

    ImageData imageData = *yaptScene.camera->data();

    QImage image = convertToQImage(imageData);

    QGraphicsScene scene;
    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    scene.addItem(item);

    ZoomableImageView view(yaptScene, nullptr);
    view.setDragMode(QGraphicsView::ScrollHandDrag);
    view.viewport()->setCursor(Qt::ArrowCursor);
    view.setScene(&scene);
    view.setRenderHint(QPainter::Antialiasing);
    view.setWindowTitle("Image Display");
    view.resize(800, 800);
    view.fitInView(scene.sceneRect(), Qt::KeepAspectRatio);
    view.show();

    return app.exec();
}
