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

#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QPolygon>
#include <QPen>
#include <QBrush>

#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>

class Scene3D : public QOpenGLWidget, protected QOpenGLFunctions {
public:
    Scene3D(QWidget *parent = nullptr) : QOpenGLWidget(parent) {
        setWindowTitle("Qt6 OpenGL : Sphère Transparente & Orbite");
        resize(800, 600);
    }

protected:
    // --- 1. Initialisation OpenGL ---
    void initializeGL() override {
        initializeOpenGLFunctions();
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

        // --- NOUVEAU : ACTIVATION DE LA LUMIÈRE ---
        glEnable(GL_LIGHTING); // Active le calcul d'éclairage global
        glEnable(GL_LIGHT0);   // Allume la "lampoule n°0"

        // Position de la lumière (fixe par rapport au monde)
        // x=1, y=1, z=1, w=0 (lumière directionnelle venant de la diagonale)
        GLfloat lightPos[] = { 1.0f, 1.0f, 1.0f, 0.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

        // Lumière blanche
        GLfloat lightColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
        glLightfv(GL_LIGHT0, GL_SPECULAR, lightColor);

        // Pour que les objets simples (lignes, triangles) gardent leur couleur
        // sans qu'on ait à définir un matériau complexe pour eux :
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    }

    // --- 2. Gestion du redimensionnement ---
    void resizeGL(int w, int h) override {
        glViewport(0, 0, w, h);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        // Perspective : FOV 45°, Ratio, Near 0.1, Far 100.0
        float aspect = float(w) / float(h ? h : 1);
        float zNear = 0.1f;
        float zFar = 100.0f;
        float fov = 45.0f;

        // Calcul manuel de la matrice de perspective (équivalent à gluPerspective)
        float ymax = zNear * tanf(fov * M_PI / 360.0f);
        float xmax = ymax * aspect;
        glFrustum(-xmax, xmax, -ymax, ymax, zNear, zFar);

        glMatrixMode(GL_MODELVIEW);
    }

    // --- 3. Dessin de la scène ---
    void paintGL() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        // -- GESTION CAMÉRA (ZOOM & ORBITE) --
        // 1. On recule la caméra (Zoom)
        glTranslatef(0.0f, 0.0f, -m_dist);
        // 2. On pivote la scène (Orbite)
        glRotatef(m_xRot, 1.0f, 0.0f, 0.0f);
        glRotatef(m_yRot, 0.0f, 1.0f, 0.0f);

        // -- A. DESSINER DES SEGMENTS (Les Axes XYZ) --
        glLineWidth(2.0f);
        glBegin(GL_LINES);
            // Axe X (Rouge)
            glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(1.0f, 0.0f, 0.0f);
            // Axe Y (Vert)
            glColor3f(0.0f, 1.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 1.0f, 0.0f);
            // Axe Z (Bleu)
            glColor3f(0.0f, 0.0f, 1.0f); glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 1.0f);
        glEnd();

        // // -- B. DESSINER UN POLYGONE OPAQUE --
        // // Un triangle jaune décalé sur la gauche
        // glBegin(GL_TRIANGLES);
        //     glColor3f(1.0f, 1.0f, 0.0f); // Jaune
        //     glVertex3f(-1.5f, 0.0f, 0.0f);
        //     glVertex3f(-2.5f, 0.0f, 0.0f);
        //     glVertex3f(-2.0f, 1.5f, 0.0f);
        // glEnd();

        // -- C. DESSINER UNE SPHÈRE SEMI-TRANSPARENTE --
        // Note: Pour une transparence correcte, il faut idéalement dessiner
        // les objets transparents EN DERNIER.

        // On sauvegarde la matrice actuelle pour bouger la sphère sans bouger le reste
        // On désactive GL_COLOR_MATERIAL pour que la sphère utilise
        // précisément les paramètres de matériau qu'on va définir, et non glColor.
        glDisable(GL_COLOR_MATERIAL);

        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.0f);

        // 1. COULEUR DE BASE (DIFFUSE) + TRANSPARENCE
        // R=0.7, G=0.8, B=0.9 (Bleuté glacé)
        // Alpha = 0.4 (40% opaque -> effet un peu laiteux du dépoli)
        GLfloat mat_diffuse[] = { 0.7f, 0.8f, 0.9f, 0.4f };
        glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);

        // 2. REFLET (SPECULAR)
        // Le verre reflète beaucoup la lumière (blanc pur)
        GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);

        // 3. RUGOSITÉ (SHININESS)
        // C'est ici que se joue l'effet "dépoli".
        // - 128.0 (Max) = Miroir parfait / Verre clair (point lumineux minuscule)
        // - 10.0 - 30.0 = Plastique
        // - 50.0 - 70.0 = Verre dépoli (point lumineux étalé)
        GLfloat mat_shininess[] = { 80.0f };
        glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

        drawSphere(1.0f, 50, 50); // Augmenter un peu les segments pour un éclairage lisse

        glPopMatrix();

        // On réactive le comportement standard pour les prochaines frames/objets
        glEnable(GL_COLOR_MATERIAL);
    }

    // Fonction utilitaire pour dessiner une sphère "à la main"
    void drawSphere(float radius, int latSegments, int lonSegments) {
        for (int i = 0; i < latSegments; ++i) {
            float lat0 = M_PI * (-0.5f + (float)(i) / latSegments);
            float z0  = sinf(lat0);
            float zr0 = cosf(lat0);

            float lat1 = M_PI * (-0.5f + (float)(i + 1) / latSegments);
            float z1  = sinf(lat1);
            float zr1 = cosf(lat1);

            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= lonSegments; ++j) {
                float lng = 2 * M_PI * (float)(j - 1) / lonSegments;
                float x = cosf(lng);
                float y = sinf(lng);

                // Normales (pour la lumière, optionnel ici mais bonne pratique)
                glNormal3f(x * zr0, y * zr0, z0);
                glVertex3f(radius * x * zr0, radius * y * zr0, radius * z0);

                glNormal3f(x * zr1, y * zr1, z1);
                glVertex3f(radius * x * zr1, radius * y * zr1, radius * z1);
            }
            glEnd();
        }
    }

    // --- 4. Gestion de la Souris (Interactions) ---

    // Quand on clique
    void mousePressEvent(QMouseEvent *event) override {
        m_lastPos = event->pos();
    }

    // Quand on bouge la souris en cliquant (Orbite)
    void mouseMoveEvent(QMouseEvent *event) override {
        int dx = event->pos().x() - m_lastPos.x();
        int dy = event->pos().y() - m_lastPos.y();

        if (event->buttons() & Qt::LeftButton) {
            m_xRot += dy; // Rotation axe X
            m_yRot += dx; // Rotation axe Y
            update();     // Demande à Qt de redessiner
        }
        m_lastPos = event->pos();
    }

    // Quand on utilise la molette (Zoom)
    void wheelEvent(QWheelEvent *event) override {
        // Ajuste la distance en fonction du scroll
        float delta = event->angleDelta().y() / 120.0f;
        m_dist -= delta * 0.5f;

        if (m_dist < 1.0f) m_dist = 1.0f; // Empêcher de traverser l'objet

        update();
    }

private:
    float m_xRot = 0.0f;
    float m_yRot = 0.0f;
    float m_dist = 5.0f; // Distance de caméra (Zoom) initial
    QPoint m_lastPos;
};

int go_for_it(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Scene3D fenetre;
    fenetre.show();
    return app.exec();
}

// class MyWidget : public QWidget {
// public:
//     MyWidget(QWidget *parent = nullptr) : QWidget(parent) {
//         resize(400, 400);
//         setWindowTitle("Qt6 example");
//     }
//
// protected:
//     // called automatically when the widget needs to be redrawn
//     void paintEvent(QPaintEvent *event) override {
//         Q_UNUSED(event);
//
//         QPainter painter(this);
//
//         painter.setRenderHint(QPainter::Antialiasing);
//
//         // points
//         QPen point_pen(Qt::red);
//         point_pen.setWidth(5);
//         painter.setPen(point_pen);
//
//         painter.drawPoint(50, 50);
//         painter.drawPoint(100, 50);
//         painter.drawPoint(150, 50);
//
//         // line
//         QPen line_pen(Qt::blue);
//         line_pen.setWidth(3);
//         line_pen.setStyle(Qt::DashLine);
//         painter.setPen(line_pen);
//
//         painter.drawLine(50, 100, 350, 100);
//
//         // Polygon
//         QBrush brush(Qt::green);
//         brush.setStyle(Qt::SolidPattern);
//
//         painter.setPen(QPen(Qt::black, 2, Qt::SolidLine));
//         painter.setBrush(brush);
//
//         QPolygon polygon;
//         polygon << QPoint(50, 200)
//                 << QPoint(150, 350)
//                 << QPoint(250, 200);
//
//         painter.drawPolygon(polygon);
//     }
// };
//
// int go_for_it(int argc, char *argv[]) {
//     QApplication app(argc, argv);
//
//     MyWidget window;
//     window.show();
//
//     return app.exec();
// }

int main(int argc, char **argv) {
    return go_for_it(argc, argv);
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