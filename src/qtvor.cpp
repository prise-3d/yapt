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

#include "spherical_voronoi.h"

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

class Scene3D : public QOpenGLWidget, protected QOpenGLFunctions, public SphericalVoronoiIntegratorObserver {
public:
    Scene3D(QWidget *parent = nullptr) : QOpenGLWidget(parent) {

        QSurfaceFormat format;
        format.setAlphaBufferSize(8);
        format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
        format.setDepthBufferSize(24);

        setFormat(format);
        setAttribute(Qt::WA_OpaquePaintEvent);

        setWindowTitle("Voronoi nesting sampling");
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
        // glLightfv(GL_LIGHT0, GL_SPECULAR, lightColor);

        GLfloat lightColor2[] = { .3f, .3f, .3f, 1.0f };
        glLightfv(GL_LIGHT0, GL_AMBIENT, lightColor2);

        // Pour que les objets simples (lignes, triangles) gardent leur couleur
        // sans qu'on ait à définir un matériau complexe pour eux :
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
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

        normal = unit_vector(normal);

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
            // glColor3f(1.0f, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(1.0f, 0.0f, 0.0f);
            // // Axe Y (Vert)
            // glColor3f(0.0f, 1.0f, 0.0f); glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 1.0f, 0.0f);
            // // Axe Z (Bleu)
            // glColor3f(0.0f, 0.0f, 1.0f); glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 1.0f);

        for (size_t i = 0 ; i < contributions.size() ; ++i) {
            const Vec3 contribution = contributions[i];
            const double weight = weights[i];
            Vec3 direction = 1.2*directions[i];

            glColor3f(
                static_cast<float>(contribution.x()),
                static_cast<float>(contribution.y()),
                static_cast<float>(contribution.z())
            );
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(
                static_cast<float>(direction.x()),
                static_cast<float>(direction.y()),
                static_cast<float>(direction.z())
            );
        }
        glColor3f(
                1.f,
                1.f,
                1.f
            );
        glVertex3f(0.0f, 0.0f, 0.0f);
        glVertex3f(
            static_cast<float>(1.5*normal.x()),
            static_cast<float>(1.5*normal.y()),
            static_cast<float>(1.5*normal.z())
        );

        glEnd();

        // début dessin d'un quad

        // À placer dans paintGL()
        QVector3D n(
            static_cast<float>(normal.x()),
            static_cast<float>(normal.y()),
            static_cast<float>(normal.z())
        );
        n.normalize();                 // Important : toujours normaliser

        // 1. Calcul de la rotation pour passer de l'axe Z (0,0,1) à votre vecteur n
        // On utilise QQuaternion qui fait les maths complexes pour nous.
        QQuaternion rotation = QQuaternion::rotationTo(QVector3D(0.0f, 0.0f, 1.0f), n);

        float angle;
        QVector3D axis;
        rotation.getAxisAndAngle(&axis, &angle);

        // 2. Application de la transformation
        glPushMatrix(); // Sauvegarde la matrice actuelle

        // On tourne la scène de l'angle calculé
        glRotatef(angle, axis.x(), axis.y(), axis.z());

        // 3. On dessine un Quad plat "par défaut" sur le plan Z=0
        // Grâce à la rotation, il apparaîtra perpendiculaire à n.
        float size = 5.0f;

        // Optionnel : Désactiver le "Culling" pour voir le plan des deux côtés
        glDisable(GL_CULL_FACE);

        // glColor4f(0.5f, 0.8f, 0.2f, 0.6f); // Vert semi-transparent
        // glBegin(GL_QUADS);
        // glVertex3f(-size, -size, 0.0f);
        // glVertex3f( size, -size, 0.0f);
        // glVertex3f( size,  size, 0.0f);
        // glVertex3f(-size,  size, 0.0f);
        // glEnd();

        glEnable(GL_CULL_FACE); // Bonne pratique : réactiver après

        glPopMatrix(); // Restaure la matrice

        drawPolygons();

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


        // drawSphere(1.0f, 50, 50); // Augmenter un peu les segments pour un éclairage lisse

        glPopMatrix();

        // On réactive le comportement standard pour les prochaines frames/objets
        glEnable(GL_COLOR_MATERIAL);
    }

    void drawPolygons() {
        GLfloat mat_no_specular[] = { 0.0f, 0.0f, 0.0f, 1.0f };
        GLfloat mat_no_shininess[] = { 0.0f }; // Pas de brillance

        // 2. On applique ces paramètres au matériau
        glMaterialfv(GL_FRONT, GL_SPECULAR, mat_no_specular);
        glMaterialfv(GL_FRONT, GL_SHININESS, mat_no_shininess);

        // 3. On s'assure que la couleur de base (Diffuse) suit toujours glColor()
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
        size_t i=0;
        for (const auto& face: faces) {
            const auto contribution = contributions[i++];
            glColor4f(static_cast<float>(contribution.x()), static_cast<float>(contribution.y()), static_cast<float>(contribution.z()), 1.f);

            for (const auto& triangle: face) {
                QVector3D p0(
                    static_cast<float>(triangle[0].x()),
                    static_cast<float>(triangle[0].y()),
                    static_cast<float>(triangle[0].z())
                );
                const QVector3D p1(static_cast<float>(triangle[1].x()), static_cast<float>(triangle[1].y()), static_cast<float>(triangle[1].x()));
                const QVector3D p2(static_cast<float>(triangle[2].x()), static_cast<float>(triangle[2].y()), static_cast<float>(triangle[2].z()));

                const QVector3D u = p1 - p0;
                const QVector3D v = p2 - p0;

                // La normale est le produit vectoriel de u et v
                QVector3D n = QVector3D::crossProduct(u, v);
                n.normalize();
                // n = -n;

                glNormal3f(n.x(), n.y(), n.z());


                glBegin(GL_POLYGON);
                for (const auto& pt : triangle) {
                    glVertex3f(static_cast<float>(pt.x()), static_cast<float>(pt.y()), static_cast<float>(pt.z()));
                }
                glEnd();
            }
        }
    }

    // Fonction utilitaire pour dessiner une sphère "à la main"
    void drawSphere(float radius, int latSegments, int lonSegments) {
        const auto longitude = static_cast<float>(lonSegments);
        for (int i = 0; i < latSegments; ++i) {
            const float lat0 = M_PI * (-0.5f + static_cast<float>(i) / longitude);
            const float z0  = sinf(lat0);
            const float zr0 = cosf(lat0);

            const float lat1 = M_PI * (-0.5f + static_cast<float>(i + 1) / longitude);
            const float z1  = sinf(lat1);
            const float zr1 = cosf(lat1);

            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= lonSegments; ++j) {
                const float lng = 2.f * static_cast<float>(M_PI) * static_cast<float>(j - 1) / longitude;
                const float x = cosf(lng);
                const float y = sinf(lng);

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
        const int dx = event->pos().x() - m_lastPos.x();
        const int dy = event->pos().y() - m_lastPos.y();

        if (event->buttons() & Qt::LeftButton) {
            m_xRot += static_cast<float>(dy); // Rotation axe X
            m_yRot += static_cast<float>(dx); // Rotation axe Y
            update();     // Demande à Qt de redessiner
        }
        m_lastPos = event->pos();
    }

    // Quand on utilise la molette (Zoom)
    void wheelEvent(QWheelEvent *event) override {
        // Ajuste la distance en fonction du scroll
        const float delta = static_cast<float>(event->angleDelta().y()) / 120.0f;
        m_dist -= delta * 0.5f;

        if (m_dist < 1.0f) m_dist = 1.0f; // Empêcher de traverser l'objet

        update();
    }

    void on_computation_complete(const Color &total_contribution, const Vec3 &new_normal, const double total_area, const std::vector<Vec3> new_directions, const std::vector<Color> new_contributions, const std::vector<double> new_weights, const SDT& delaunay, const std::vector<std::vector<std::vector<Point_3>>>& new_faces) override {
        directions = new_directions;
        contributions = new_contributions;
        weights = new_weights;
        normal = new_normal;
        dt = delaunay;
        faces = new_faces;
    }

private:
    float m_xRot = 0.0f;
    float m_yRot = 0.0f;
    float m_dist = 5.0f; // Distance de caméra (Zoom) initial
    QPoint m_lastPos;
    std::vector<Vec3> directions;
    std::vector<Vec3> contributions;
    std::vector<double> weights;
    Vec3 normal;
    SDT dt;
    std::vector<std::vector<std::vector<Point_3>>> faces;
};

int go_for_it(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Scene3D window;
    window.show();
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
    qputenv("QT_QPA_PLATFORM", "xcb"); // we try our best to bypass wayland
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

    const auto sampling_strategy = camera->samplingStrategy;

    std::vector<std::shared_ptr<SphericalVoronoiIntegratorObserver>> observers;

    QApplication app(argc, argv);
    // Scene3D window;
    auto window = std::make_shared<Scene3D>();
    observers.push_back(window);
    camera->scattering_strategy =
        std::make_shared<ObservableCVorNestedRayEvaluator>(
            sampling_strategy,
            make_shared<SimpleRayEvaluator>(sampling_strategy),
            config.nested_sample_size,
            observers
        );
    camera->render(scene);
    window->show();
    return app.exec();

    // return go_for_it(argc, argv);
    // return test(argc, argv);

    // RenderFactory::init();
    // CommandLineParser commandLineParser;
    //
    // RenderConfig config = commandLineParser.parse(argc, argv);
    // if (config.help) {
    //     display_help(config);
    //     std::exit(0);
    // }
    //
    // const auto content = RenderFactory::createContent(config);
    // const auto camera = content->camera;
    // const auto scene = content->scene;
    //
    // OutputManager manager(config);
    //
    // manager.start_timer();
    // camera->render(scene);
    // manager.stop_timer();
    //
    // QApplication app(argc, argv);
    //
    //
    //
    // const ImageData imageData = *camera->data();
    // const QImage image = convertToQImage(imageData);
    //
    // QGraphicsScene qgScene;
    // auto *item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    // qgScene.addItem(item);
    //
    //
    //
    // ZoomableImageView view(*content, nullptr);
    // view.setDragMode(QGraphicsView::ScrollHandDrag);
    // view.viewport()->setCursor(Qt::ArrowCursor);
    // view.setScene(&qgScene);
    // view.setRenderHint(QPainter::Antialiasing);
    // view.setWindowTitle("QtVor");
    // view.resize(800, 800);
    // view.fitInView(qgScene.sceneRect(), Qt::KeepAspectRatio);
    // view.show();
    //
    // return app.exec();
}