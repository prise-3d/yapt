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

#include "parser.h"
#include "qtvor/zoomableimageview.h"

inline std::string to_string(const Point& point) {
    return "(" + std::to_string(point.x()) + ", " + std::to_string(point.y()) + ")";
}

int main(int argc, char **argv) {
    Parser parser;
    Scene yaptScene;
    if (!parser.parseScene(argc, argv, yaptScene)) return 0;
    yaptScene.camera->render(*yaptScene.content, *yaptScene.lights);

    QApplication app(argc, argv);

    const ImageData imageData = *yaptScene.camera->data();
    const QImage image = convertToQImage(imageData);

    QGraphicsScene scene;
    auto *item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    scene.addItem(item);

    ZoomableImageView view(yaptScene, nullptr);
    view.setDragMode(QGraphicsView::ScrollHandDrag);
    view.viewport()->setCursor(Qt::ArrowCursor);
    view.setScene(&scene);
    view.setRenderHint(QPainter::Antialiasing);
    view.setWindowTitle("QtVor");
    view.resize(800, 800);
    view.fitInView(scene.sceneRect(), Qt::KeepAspectRatio);
    view.show();

    return app.exec();
}