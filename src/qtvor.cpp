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


#include "qtvor/hemisphere.h"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

int test(int argc, char *argv[])
{
    QApplication a(argc, argv);

    SolidSphereView window;
    window.resize(800, 600);
    window.setWindowTitle("Qt6 C++ - Hémisphère (Z > 0)");
    window.updateVoronoi();
    window.show();

    return a.exec();
}

inline std::string to_string(const Point& point) {
    return "(" + std::to_string(point.x()) + ", " + std::to_string(point.y()) + ")";
}

int main(int argc, char **argv) {

    // return test(argc, argv);

    RenderFactory::init();
    CommandLineParser commandLineParser;

    RenderConfig config = commandLineParser.parse(argc, argv);
    if (config.help) {
        display_help(config);
        std::exit(0);
    }

    const auto content = RenderFactory::createContent(config);
    const auto camera = content->camera;
    const auto scene = content->scene;

    OutputManager manager(config);

    manager.start_timer();
    camera->render(scene);
    manager.stop_timer();

    QApplication app(argc, argv);



    const ImageData imageData = *camera->data();
    const QImage image = convertToQImage(imageData);

    QGraphicsScene qgScene;
    auto *item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    qgScene.addItem(item);



    ZoomableImageView view(*content, nullptr);
    view.setDragMode(QGraphicsView::ScrollHandDrag);
    view.viewport()->setCursor(Qt::ArrowCursor);
    view.setScene(&qgScene);
    view.setRenderHint(QPainter::Antialiasing);
    view.setWindowTitle("QtVor");
    view.resize(800, 800);
    view.fitInView(qgScene.sceneRect(), Qt::KeepAspectRatio);
    view.show();

    return app.exec();
}