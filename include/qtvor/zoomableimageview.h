//
// Created by franck on 17/12/24.
//

#ifndef ZOOMABLEIMAGEVIEW_H
#define ZOOMABLEIMAGEVIEW_H
#include <QApplication>
#include <qgraphicsitem.h>

#include "scene.h"
#include "zoomablegraphicsview.h"
#include "voronoicellitem.h"

class ZoomableImageView : public ZoomableGraphicsView {
public:
    explicit ZoomableImageView(Scene yaptScene, QWidget *parent = nullptr): ZoomableGraphicsView(parent),
                                                                            yaptScene(yaptScene) {
        setMouseTracking(true);
        viewport()->installEventFilter(this);
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

    bool eventFilter(QObject *obj, QEvent *event) override {
        if (obj == viewport() && event->type() == QEvent::MouseMove) {
            auto mouseEvent = dynamic_cast<QMouseEvent*>(event);
            QPointF scenePos = mapToScene(mouseEvent->pos());
            updateWindowTitleWithCoordinates(scenePos);
        }
        return ZoomableGraphicsView::eventFilter(obj, event);
    }

    void updateWindowTitleWithCoordinates(const QPointF &scenePos) {
        if (!scene()) return;

        QList<QGraphicsItem *> itemsAtClick = scene()->items(scenePos);
        for (QGraphicsItem *item: itemsAtClick) {
            auto pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(item);
            if (pixmapItem) {
                QPointF localPos = pixmapItem->mapFromScene(scenePos);
                int x = static_cast<int>(localPos.x());
                int y = static_cast<int>(localPos.y());
                if (x >= 0 && y >= 0 && x < pixmapItem->pixmap().width() && y < pixmapItem->pixmap().height()) {
                    if (!QApplication::topLevelWidgets().isEmpty()) {
                        this->window()->setWindowTitle(QString("QtVor - Pixel: (%1, %2)").arg(x).arg(y));
                        this->window()->update();
                    }
                }
            }
        }
    }

    Scene yaptScene;
};

inline QImage convertToQImage(const ImageData &imageData) {
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
#endif //ZOOMABLEIMAGEVIEW_H
