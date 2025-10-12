//
// Created by franck on 17/12/24.
//

#ifndef VORONOICELLITEM_H
#define VORONOICELLITEM_H

#include "qtvor/utils.h"
#include <qgraphicsitem.h>
#include <QGraphicsView>
#include <QVBoxLayout>
#include <QLabel>
#include <QPen>
#include <QTableWidget>
#include <QHeaderView>
#include <QClipboard>
#include <QMouseEvent>
#include <QWidget>

class VoronoiCellItem : public QGraphicsPolygonItem {
public:
    VoronoiCellItem(const QPolygonF &polygon, const QBrush &brush, const QPointF &site,
                    const qreal ellipseDiameter, const int rowIndex, QTableWidget *table,
                    QGraphicsItem *parent = nullptr)
        : QGraphicsPolygonItem(polygon, parent),
          sitePoint(site),
          ellipseDiameter(ellipseDiameter),
          rowIndex(rowIndex),
          tableWidget(table) {
        setBrush(brush);
        setAcceptHoverEvents(true);
        setCursor(Qt::PointingHandCursor);

        const qreal ellipseRadius = ellipseDiameter / 2.0;
        ellipseItem = new QGraphicsEllipseItem(site.x() - ellipseRadius,
                                               site.y() - ellipseRadius,
                                               ellipseDiameter,
                                               ellipseDiameter,
                                               this);
        ellipseItem->setBrush(QBrush(Qt::white));
        QPen sitePen(Qt::black);
        sitePen.setWidthF(0.001);
        ellipseItem->setPen(sitePen);
        ellipseItem->setVisible(false);
    }

    void showSite(bool show) {
        if (ellipseItem) {
            ellipseItem->setVisible(show);
        }
    }

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override {
        QGraphicsPolygonItem::hoverEnterEvent(event);

        // Show the site point
        if (ellipseItem) {
            ellipseItem->setVisible(true);
        }

        // Highlight the corresponding row in the table
        if (tableWidget && rowIndex >= 0 && rowIndex < tableWidget->rowCount()) {
            tableWidget->selectRow(rowIndex);
            tableWidget->scrollToItem(tableWidget->item(rowIndex, 0));
        }

        auto views = scene()->views();
        if (!views.isEmpty()) {
            if (QGraphicsView *view = views.first()) {
                view->window()->setWindowTitle(QString("Voronoi Cell - site = (%1, %2)").arg(sitePoint.x()).arg(sitePoint.y()));
                view->window()->update();
            }
        }
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override {
        QGraphicsPolygonItem::hoverLeaveEvent(event);

        // Hide the site point
        if (ellipseItem) {
            ellipseItem->setVisible(false);
        }

        // Deselect the table row
        if (tableWidget) {
            tableWidget->clearSelection();
        }
    }

private:
    QPointF sitePoint;
    qreal ellipseDiameter;
    QGraphicsEllipseItem *ellipseItem;
    int rowIndex;
    QTableWidget *tableWidget;
};

class CustomTableWidget : public QTableWidget {

public:
    CustomTableWidget(int rows, int columns, QWidget* parent = nullptr)
        : QTableWidget(rows, columns, parent), lastHoveredRow(-1) {
        setSelectionMode(QAbstractItemView::ContiguousSelection);
        setMouseTracking(true);
        viewport()->setMouseTracking(true);
        connect(this, &QTableWidget::cellEntered, this, &CustomTableWidget::onCellEntered);
    }

    ~CustomTableWidget() override = default;

    void setCellItems(const std::vector<VoronoiCellItem*>& cells) {
        cellItems = cells;
    }

protected:
    void keyPressEvent(QKeyEvent* event) override {
        if (event->matches(QKeySequence::Copy)) {
            copySelectionToClipboard();
        } else {
            QTableWidget::keyPressEvent(event);
        }
    }

    void leaveEvent(QEvent* event) override {
        QTableWidget::leaveEvent(event);
        // Hide site when mouse leaves the table
        if (lastHoveredRow >= 0 && lastHoveredRow < cellItems.size()) {
            cellItems[lastHoveredRow]->showSite(false);
        }
        lastHoveredRow = -1;
    }

private:
    void onCellEntered(int row, int column) {
        if (row != lastHoveredRow) {
            // Hide the previous row's site
            if (lastHoveredRow >= 0 && lastHoveredRow < cellItems.size()) {
                cellItems[lastHoveredRow]->showSite(false);
            }

            // Show the current row's site
            if (row >= 0 && row < cellItems.size()) {
                cellItems[row]->showSite(true);
            }

            lastHoveredRow = row;
        }
    }
    void copySelectionToClipboard() {
        QModelIndexList indexes = selectionModel()->selectedIndexes();

        if (indexes.isEmpty()) {
            return;
        }

        // Sort the selected indexes by row and column
        std::sort(indexes.begin(), indexes.end());

        QString copiedText;
        int previousRow = indexes.first().row();

        for (const QModelIndex& index : indexes) {
            if (index.row() != previousRow) {
                copiedText += '\n'; // New line for a new row
                previousRow = index.row();
            } else if (!copiedText.isEmpty()) {
                copiedText += '\t'; // Tab for a new column
            }

            copiedText += model()->data(index).toString();
        }

        // Copy to clipboard
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(copiedText);
    }

    std::vector<VoronoiCellItem*> cellItems;
    int lastHoveredRow;
};

inline void displayVoronoi(Scene yaptScene, int x, int y) {
    auto ag = yaptScene.camera->renderPixel(*yaptScene.content, *yaptScene.lights, y, x);

    auto aggregator = std::dynamic_pointer_cast<VoronoiAggregator>(ag);

    if (aggregator == nullptr) {
        return;
    }

    auto delaunay = aggregator->delaunay;
    auto voronoi = aggregator->voronoi;

    // Create the table first so we can pass it to VoronoiCellItem
    size_t size = aggregator->samples.size();
    auto *table = new CustomTableWidget(size, 7);
    table->setHorizontalHeaderLabels({"Color", "Site x", "Site y", "R",
                                  "G", "B", "Area"});

    // Populate the table
    size_t row = 0;
    for (auto vertex = delaunay.vertices_begin(); vertex != delaunay.vertices_end(); ++vertex) {
        Point p = vertex->point();

        Color contribution = aggregator->contributions[row];

        // Create a colored widget for the first column
        QWidget *colorWidget = new QWidget();
        QColor cellColor = toQColor(contribution);
        colorWidget->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
                                    .arg(cellColor.red())
                                    .arg(cellColor.green())
                                    .arg(cellColor.blue()));
        table->setCellWidget(row, 0, colorWidget);

        table->setItem(row, 1, new QTableWidgetItem(QString::number(p.x(), 'f', 5)));
        table->setItem(row, 2, new QTableWidgetItem(QString::number(p.y(), 'f', 5)));
        table->setItem(row, 3, new QTableWidgetItem(QString::number(contribution.x(), 'f', 5)));
        table->setItem(row, 4, new QTableWidgetItem(QString::number(contribution.y(), 'f', 5)));
        table->setItem(row, 5, new QTableWidgetItem(QString::number(contribution.z(), 'f', 5)));
        table->setItem(row, 6, new QTableWidgetItem(QString::number(aggregator->weights[row], 'f', 5)));
        ++row;
    }

    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // Set fixed width for the Color column
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    table->setColumnWidth(0, 60);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Now create the graphics scene with cells linked to table rows
    auto *qScene = new QGraphicsScene();
    std::vector<VoronoiCellItem*> cellItems;

    QPen voronoiPen(Qt::blue);
    voronoiPen.setWidthF(.0002);

    QPen pointPen(Qt::red);
    pointPen.setWidthF(.0025);

    size_t idx = 0;
    for (auto vertex = delaunay.vertices_begin(); vertex != delaunay.vertices_end(); ++vertex) {
        constexpr qreal ellipse_diameter = .01;
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
            qPolygon, QBrush(toQColor(aggregator->contributions[idx])),
            QPointF(site.x(), site.y()),
            ellipse_diameter, idx, table);
        cellItem->setPen(voronoiPen);
        qScene->addItem(cellItem);
        cellItems.push_back(cellItem);
        ++idx;
    }

    // Connect the cell items to the table for hover highlighting
    table->setCellItems(cellItems);

    qScene->addRect(-.5, -.5, (1.), (1.), pointPen);

    QGraphicsView *popupView = new ZoomableGraphicsView();
    popupView->setScene(qScene);
    popupView->setRenderHint(QPainter::Antialiasing);
    popupView->fitInView(QRectF(-0.55, -0.55, 1.1, 1.1), Qt::KeepAspectRatio);

    popupView->setDragMode(QGraphicsView::ScrollHandDrag);
    popupView->viewport()->setCursor(Qt::ArrowCursor);

    QWidget *textWidget = new QWidget();
    QVBoxLayout *textLayout = new QVBoxLayout(textWidget);

    textLayout->addWidget(new QLabel(QString("Pixel: (%1, %2)").arg(x).arg(y)));
    textLayout->addWidget(table);


    textLayout->addStretch();

    QWidget *popupWidget = new QWidget();
    QHBoxLayout *mainLayout = new QHBoxLayout(popupWidget);
    mainLayout->addWidget(popupView);
    mainLayout->addWidget(textWidget);

    popupWidget->setWindowTitle(QString("Voronoi (%1; %2)").arg(x).arg(y));
    popupWidget->resize(1300, 800);
    popupWidget->setAttribute(Qt::WA_DeleteOnClose);

    popupWidget->show();
}
#endif //VORONOICELLITEM_H
