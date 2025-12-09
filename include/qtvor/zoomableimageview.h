/*
* This file is part of the YAPT distribution (https://github.com/prise-3d/yapt).
 * Copyright (c) 2025 PrISE-3D.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * --- ADDITIONAL PERMISSION UNDER GNU GPL VERSION 3 SECTION 7 ---
 *
 * If you modify this Program, or any covered work, by linking or
 * combining it with the Intel Math Kernel Library (MKL) (or a modified
 * version of that library), containing parts covered by the terms of
 * the Intel Simplified Software License, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 */

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
