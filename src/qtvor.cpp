//
// Created by franck on 13/12/24.
//


#include "parser.h"
#include "qtvor/zoomableimageview.h"

inline std::string to_string(Point& point) {
    return "(" + std::to_string(point.x()) + ", " + std::to_string(point.y()) + ")";
}

int test(int argc, char *argv[]) {
    auto delaunay = make_shared<Delaunay>();

    const Point points[] = {
        Point(0.2, 0.2), //A
        Point(0, 1.2),   //B
        Point(1, 0),     //C
        Point(-1, 0),    //D
        Point(0, -1),    //E
        Point(-1, -1),   //F
        Point(1.1, -1.1),//G
        Point(1, 1),     //H
        Point(-1.2, 1.2),//I
    };

    for (int i = 0; i < std::size(points); i++) {
        delaunay->insert(points[i]);
    }

    auto voronoi = make_shared<Voronoi>(*delaunay);

    std::cout << "voronoi" << std::endl;
    for (auto f = voronoi->unbounded_faces_begin(); f != voronoi->unbounded_faces_end(); ++f) {
        auto p = f->dual();
        auto point = p->point();
        std::cout << to_string(point) << std::endl;
    }
    std::cout << std::endl;

    for (auto v = delaunay->all_vertices_begin() ; v!=delaunay->all_vertices_end(); ++v) {
        if (delaunay->is_infinite(v)) {
            std::cout << to_string(v->point()) << " is infinite " << std::endl;
        }
    }
    return 0;
}


int main(int argc, char **argv) {
    // test(argc, argv);
    Parser parser;
    Scene yaptScene;
    if (!parser.parseScene(argc, argv, yaptScene)) return 0;
    yaptScene.camera->render(*yaptScene.content, *yaptScene.lights);

    QApplication app(argc, argv);

    ImageData imageData = *yaptScene.camera->data();
    QImage image = convertToQImage(imageData);

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