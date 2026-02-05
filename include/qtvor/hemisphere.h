#ifndef SOLIDSPHEREVIEW_H
#define SOLIDSPHEREVIEW_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <vector>
#include <cmath>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Voronoi_diagram_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_traits_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_policies_2.h>
#include <CGAL/Delaunay_triangulation_on_sphere_2.h>
#include <CGAL/Projection_on_sphere_traits_3.h>

// typedefs for defining the adaptor
typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Delaunay_triangulation_2 <K> Delaunay;
typedef CGAL::Delaunay_triangulation_adaptation_traits_2 <Delaunay> AT;
typedef CGAL::Delaunay_triangulation_caching_degeneracy_removal_policy_2 <Delaunay> AP;
typedef CGAL::Voronoi_diagram_2 <Delaunay, AT, AP> Voronoi;
// typedef for the result type of the point location
typedef AT::Point_2 Point;
typedef Voronoi::Face_handle Face_handle;
typedef Voronoi::Ccb_halfedge_circulator Ccb_halfedge_circulator;
typedef CGAL::Polygon_2<K> Polygon;

// typedefs for Voronoi diagrams on the surface of a sphere
typedef CGAL::Projection_on_sphere_traits_3<K> Traits;
typedef CGAL::Delaunay_triangulation_on_sphere_2<Traits> SDT;
typedef K::Point_3 Point_3;
typedef K::Vector_3 Vector_3;

class SolidSphereView : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    SolidSphereView(QWidget *parent = nullptr) : QOpenGLWidget(parent) {}
    ~SolidSphereView() {
        makeCurrent();
        m_vboSphere.destroy();
        m_vaoSphere.destroy();
        m_vboAxes.destroy();
        m_vaoAxes.destroy();
        delete m_program;
        doneCurrent();
    }

    void updateVoronoi() {
        m_cpuVoronoiData.clear(); // On vide le tampon membre
        Traits traits(Point_3(0, 0, 0), 1.0);
        SDT dt(traits);

        double t[] = {1.,0.5,0.2,0.22,0.6,2.5,4.2,5.};
        double p[] = {1.3,1.,0.1,.22,.4,1,.5,.11};
        for (int i = 0 ; i < 8 ;++i) {
            const double tt = t[i];
            const double pp = p[i];
            dt.insert(Point_3(cos(tt)*cos(pp), sin(tt)*cos(pp), sin(pp)));
        }

        std::vector<float> voronoiData;

        // Rayon légèrement augmenté pour éviter le "Z-fighting" (clignotement)
        // L'arête doit flotter un tout petit peu au-dessus de la sphère bleue.
        float R = 1.002f;

        // On parcourt toutes les arêtes finies de la triangulation
        for (auto e = dt.finite_edges_begin(); e != dt.finite_edges_end(); ++e) {
            // Une arête est définie par une face et un index
            SDT::Face_handle f1 = e->first;
            int idx = e->second;
            SDT::Face_handle f2 = f1->neighbor(idx);

            // Les sommets de l'arête Voronoi sont les duaux (circumcenters) des faces
            Point_3 p1_cgal = dt.dual(f1);
            Point_3 p2_cgal = dt.dual(f2);

            // Conversion CGAL -> QVector3D (ou structure simple)
            QVector3D p1(CGAL::to_double(p1_cgal.x()), CGAL::to_double(p1_cgal.y()), CGAL::to_double(p1_cgal.z()));
            QVector3D p2(CGAL::to_double(p2_cgal.x()), CGAL::to_double(p2_cgal.y()), CGAL::to_double(p2_cgal.z()));

            // Normalisation pour être sûr (CGAL dual est sur la sphère, mais précaution)
            p1.normalize(); p2.normalize();

            // --- Tessellation de l'arc (SLERP) ---
            // On découpe l'arc en N segments pour qu'il épouse la sphère
            int segments = 20; // Plus ce nombre est grand, plus la courbe est lisse

            // Calcul de l'angle (omega) entre les deux vecteurs pour le Slerp
            float dot = QVector3D::dotProduct(p1, p2);
            // Clamp pour éviter les erreurs numériques acos
            if (dot > 1.0f) dot = 1.0f; else if (dot < -1.0f) dot = -1.0f;
            float omega = std::acos(dot);
            float sinOmega = std::sin(omega);

            QVector3D prevP = p1 * R;

            for (int i = 1; i <= segments; ++i) {
                float t = (float)i / segments;

                QVector3D currP;
                if (std::abs(sinOmega) < 1e-5) {
                    // Points très proches ou opposés, interpolation linéaire simple
                    currP = (1.0f - t) * p1 + t * p2;
                } else {
                    // Formule Slerp standard
                    float w1 = std::sin((1.0f - t) * omega) / sinOmega;
                    float w2 = std::sin(t * omega) / sinOmega;
                    currP = w1 * p1 + w2 * p2;
                }
                currP = currP.normalized() * R; // Projection stricte sur le rayon R

                // Ajout du segment (LIGNE)
                voronoiData.push_back(prevP.x()); voronoiData.push_back(prevP.y()); voronoiData.push_back(prevP.z());
                voronoiData.push_back(currP.x()); voronoiData.push_back(currP.y()); voronoiData.push_back(currP.z());

                prevP = currP;
            }
        }

        // makeCurrent(); // Important si appelé depuis l'extérieur
        //
        // // Création/Mise à jour buffers Voronoi
        // if(!m_vaoVoronoi.isCreated()) m_vaoVoronoi.create();
        // m_vaoVoronoi.bind();
        //
        // if(!m_vboVoronoi.isCreated()) m_vboVoronoi.create();
        // m_vboVoronoi.bind();
        //
        // m_vboVoronoi.allocate(voronoiData.data(), voronoiData.size() * sizeof(float));
        // m_vertexCountVoronoi = voronoiData.size() / 3;
        //
        // // On utilise le même shader, donc location 0 est la position
        // m_program->enableAttributeArray(0);
        // m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 0);
        //
        // m_vboVoronoi.release();
        // m_vaoVoronoi.release();
        //
        // doneCurrent();

        m_voronoiDataDirty = true;
        update(); // Demande de repeindre
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();

        // 1. ACTIVATION DU DEPTH TEST (Crucial pour les faces cachées)
        glEnable(GL_DEPTH_TEST);

        // Optionnel : Cull Face (ne dessine pas l'intérieur de la sphère pour optimiser)
        glEnable(GL_CULL_FACE);

        m_program = new QOpenGLShaderProgram();

        // --- Vertex Shader ---
        // On passe la position (loc 0) et la normale (loc 1)
        m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
            "#version 330 core\n"
            "layout(location = 0) in vec3 aPos;\n"
            "layout(location = 1) in vec3 aNormal;\n"
            "uniform mat4 mvp;\n"
            "uniform mat4 modelView;\n" // Pour calculer l'éclairage dans l'espace vue
            "uniform bool uLightingEnabled;\n" // Switch pour activer/désactiver lumière
            "out vec3 vNormal;\n"
            "out vec3 vFragPos;\n"
            "void main() {\n"
            "   gl_Position = mvp * vec4(aPos, 1.0);\n"
            "   if(uLightingEnabled) {\n"
            "       // Transforme la normale par la matrice de vue (rotation seule)\n"
            "       vNormal = mat3(modelView) * aNormal;\n"
            "       vFragPos = vec3(modelView * vec4(aPos, 1.0));\n"
            "   }\n"
            "}");

        // --- Fragment Shader ---
        m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
            "#version 330 core\n"
            "in vec3 vNormal;\n"
            "in vec3 vFragPos;\n"
            "uniform vec4 uColor;\n"
            "uniform bool uLightingEnabled;\n"
            "out vec4 FragColor;\n"
            "void main() {\n"
            "   if(uLightingEnabled) {\n"
            "       // --- Eclairage Simple (Lambert) ---\n"
            "       // Lumière fixée venant d'en haut à droite\n"
            "       vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));\n"
            "       vec3 norm = normalize(vNormal);\n"
            "       // Calcul diffus\n"
            "       float diff = max(dot(norm, lightDir), 0.0);\n"
            "       // Ambiance (0.3) + Diffus\n"
            "       vec3 result = (0.3 + diff) * uColor.rgb;\n"
            "       FragColor = vec4(result, uColor.a);\n"
            "   } else {\n"
            "       // Couleur unie (pour les axes)\n"
            "       FragColor = uColor;\n"
            "   }\n"
            "}");
        m_program->link();

        // --- 2. Génération Sphère (TRIANGLES) ---
        // Format des données : 3 floats (Pos) + 3 floats (Normal)
        std::vector<float> data;
        int rings = 30; int slices = 30; float radius = 1.0f;

        for (int i = 0; i < rings; ++i) {
            float phi1 = 3.14159f * (float)i / rings;
            float phi2 = 3.14159f * (float)(i+1) / rings;

            for (int j = 0; j < slices; ++j) {
                float theta1 = 2.0f * 3.14159f * (float)j / slices;
                float theta2 = 2.0f * 3.14159f * (float)(j+1) / slices;

                // Fonction locale pour ajouter un sommet (Pos + Normale)
                // Pour une sphère centrée en 0, la normale est égale à la position normalisée
                auto pushVert = [&](float phi, float theta) {
                    float x = std::sin(phi) * std::cos(theta);
                    float y = std::sin(phi) * std::sin(theta);
                    float z = std::cos(phi);
                    // Position (Rayon * dir)
                    data.push_back(x * radius); data.push_back(y * radius); data.push_back(z * radius);
                    // Normale (dir)
                    data.push_back(x); data.push_back(y); data.push_back(z);
                };

                // Création de 2 triangles pour former un rectangle (Quad)
                // Triangle 1
                pushVert(phi1, theta1);
                pushVert(phi2, theta1);
                pushVert(phi2, theta2);
                // Triangle 2
                pushVert(phi1, theta1);
                pushVert(phi2, theta2);
                pushVert(phi1, theta2);
            }
        }
        m_sphereVertexCount = data.size() / 6; // 6 floats per vertex

        m_vaoSphere.create(); m_vaoSphere.bind();
        m_vboSphere.create(); m_vboSphere.bind();
        m_vboSphere.allocate(data.data(), data.size() * sizeof(float));

        // Attribut 0 : Position (offset 0)
        m_program->enableAttributeArray(0);
        m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 6 * sizeof(float));
        // Attribut 1 : Normale (offset 3 floats)
        m_program->enableAttributeArray(1);
        m_program->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));
        m_vaoSphere.release();

        // --- 3. Axes (idem précédent, mais sans normales) ---
        float l = 2.0f;
        std::vector<float> axes = {
             0.0f,0.0f,0.0f, l,0.0f,0.0f, // X
             0.0f,0.0f,0.0f, 0.0f,l,0.0f, // Y
             0.0f,0.0f,0.0f, 0.0f,0.0f,l  // Z
        };
        m_vaoAxes.create(); m_vaoAxes.bind();
        m_vboAxes.create(); m_vboAxes.bind();
        m_vboAxes.allocate(axes.data(), axes.size() * sizeof(float));
        m_program->enableAttributeArray(0);
        m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 0);
        // Pas de normales pour les axes -> on désactivera l'attribut 1 au dessin
        m_vaoAxes.release();

        m_vaoVoronoi.create();
        m_vboVoronoi.create();
    }

    void paintGL() override {
        // Nettoyage Couleur ET Profondeur
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Important !


        // AJOUT : Chargement différé des données Voronoi (Lazy Loading)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Si on a de nouvelles données calculées par CGAL
        if (m_voronoiDataDirty) {
            // NOTE : On ne fait plus de .create() ici ! Ils sont déjà créés.

            m_vaoVoronoi.bind();
            m_vboVoronoi.bind();

            // On remplit le buffer existant avec les nouvelles données
            if (!m_cpuVoronoiData.empty()) {
                m_vboVoronoi.allocate(m_cpuVoronoiData.data(), m_cpuVoronoiData.size() * sizeof(float));

                // On refait le lien avec le shader (nécessaire après un nouveau allocate)
                m_program->bind();
                m_program->enableAttributeArray(0);
                m_program->setAttributeBuffer(0, GL_FLOAT, 0, 3, 0);
                m_program->release();

                m_vertexCountVoronoi = m_cpuVoronoiData.size() / 3;
            }

            m_vboVoronoi.release();
            m_vaoVoronoi.release();

            m_voronoiDataDirty = false;
        }

        m_program->bind();

        QMatrix4x4 proj;
        proj.perspective(45.0f, float(width())/height(), 0.1f, 100.0f);
        QMatrix4x4 view;
        view.translate(0.0f, 0.0f, -6.0f);
        view.rotate(m_xRot, 1.0f, 0.0f, 0.0f);
        view.rotate(m_zRot, 0.0f, 0.0f, 1.0f);

        m_program->setUniformValue("mvp", proj * view);
        m_program->setUniformValue("modelView", view); // Utile pour l'éclairage

        // --- Dessin de la Sphère (LIT) ---
        m_vaoSphere.bind();
        m_program->setUniformValue("uLightingEnabled", true);
        m_program->setUniformValue("uColor", QVector4D(0.2f, 0.6f, 1.0f, 1.0f)); // Bleu clair
        glDrawArrays(GL_TRIANGLES, 0, m_sphereVertexCount);
        m_vaoSphere.release();

        // --- Dessin des Axes (UNLIT / FLAT) ---
        m_vaoAxes.bind();
        // On désactive l'attribut Normale pour éviter de lire des données incohérentes
        m_program->disableAttributeArray(1);
        m_program->setUniformValue("uLightingEnabled", false);

        glLineWidth(2.0f);
        // X (Rouge)
        m_program->setUniformValue("uColor", QVector4D(1,0,0,1)); glDrawArrays(GL_LINES, 0, 2);
        // Y (Vert)
        m_program->setUniformValue("uColor", QVector4D(0,1,0,1)); glDrawArrays(GL_LINES, 2, 2);
        // Z (Bleu)
        m_program->setUniformValue("uColor", QVector4D(0,0,1,1)); glDrawArrays(GL_LINES, 4, 2);

        // On réactive pour le prochain tour (bonne pratique)
        m_program->enableAttributeArray(1);
        m_vaoAxes.release();

        m_program->release();
    }

    void resizeGL(int w, int h) override {}

    void mousePressEvent(QMouseEvent *e) override { m_lastPos = e->pos(); }
    void mouseMoveEvent(QMouseEvent *e) override {
        if(e->buttons() & Qt::LeftButton) {
            m_xRot += (e->position().y() - m_lastPos.y()) * 0.5f;
            m_zRot += (e->position().x() - m_lastPos.x()) * 0.5f;
            update();
        }
        m_lastPos = e->pos();
    }

private:
    QOpenGLShaderProgram *m_program = nullptr;
    QOpenGLVertexArrayObject m_vaoSphere, m_vaoAxes;
    QOpenGLBuffer m_vboSphere, m_vboAxes;
    int m_sphereVertexCount = 0;
    float m_xRot = -30.0f, m_zRot = 45.0f;
    QPoint m_lastPos;

    QOpenGLVertexArrayObject m_vaoVoronoi;
    QOpenGLBuffer m_vboVoronoi;
    int m_vertexCountVoronoi = 0;

    std::vector<float> m_cpuVoronoiData;
    bool m_voronoiDataDirty = false;
};

#endif