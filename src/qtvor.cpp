//
// Created by franck on 13/12/24.
//


#include "parser.h"
#include "qtvor/zoomableimageview.h"

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
