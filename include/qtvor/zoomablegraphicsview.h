//
// Created by franck on 17/12/24.
//

#include <QGraphicsView>
#include <QWheelEvent>

#ifndef ZOOMABLEGRAPHICSVIEW_H
#define ZOOMABLEGRAPHICSVIEW_H
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
        QGraphicsView::mouseMoveEvent(event);
    }

    double scaleFactor;
};
#endif //ZOOMABLEGRAPHICSVIEW_H
