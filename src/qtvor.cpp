#include "parser.h"
#include "qtvor/zoomableimageview.h"
#include "qtvor/hemisphere.h"
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include "spherical_voronoi.h"

#include <QApplication>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>
#include <mutex>
#include <memory>
#include <vector>

// ==========================================
// 1. STRUCTURES DE DONNÉES
// ==========================================

// Structure pour stocker les données brutes prêtes pour le GPU
// Cela évite de recalculer la géométrie dans le thread d'affichage
struct GpuData {
    std::vector<float> linesData;    // x,y,z, r,g,b (pour les rayons)
    std::vector<float> polysData;    // x,y,z, r,g,b, nx,ny,nz (pour les faces)
    int linesCount = 0;
    int polysCount = 0;
};

struct RenderData {
    std::vector<Vec3> directions;
    std::vector<Vec3> contributions;
    std::vector<double> weights;
    Vec3 normal;
    SDT dt;
    std::vector<std::vector<std::vector<Point_3>>> faces;
};

// ==========================================
// 2. SHADERS (GLSL)
// ==========================================

static const char *vertexShaderSource = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aColor;
    layout(location = 2) in vec3 aNormal;

    out vec3 vColor;
    out vec3 vNormal;
    out vec3 vFragPos;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform bool useLighting; // Pour désactiver la lumière sur les lignes

    void main() {
        vColor = aColor;

        // Calcul de la position dans le monde pour la lumière
        vFragPos = vec3(model * vec4(aPos, 1.0));

        // Calcul de la normale transformée (si éclairage activé)
        if (useLighting) {
            // Matrice normale pour corriger les déformations non-uniformes
            vNormal = mat3(transpose(inverse(model))) * aNormal;
        } else {
            vNormal = vec3(0.0);
        }

        gl_Position = projection * view * vec4(vFragPos, 1.0);
    }
)";

static const char *fragmentShaderSource = R"(
    #version 330 core
    in vec3 vColor;
    in vec3 vNormal;
    in vec3 vFragPos;

    out vec4 FragColor;

    uniform bool useLighting;
    uniform vec3 lightPos;
    uniform vec3 viewPos;

    void main() {
        if (!useLighting) {
            // Rendu simple (lignes), pas d'ombre
            FragColor = vec4(vColor, 1.0);
        } else {
            // Rendu Phong (Faces Voronoi)

            // 1. Ambient
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);

            // 2. Diffuse
            vec3 norm = normalize(vNormal);
            vec3 lightDir = normalize(lightPos - vFragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * vec3(1.0, 1.0, 1.0); // Lumière blanche

            // 3. Specular (Brillance type verre/plastique)
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - vFragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);

            vec3 result = (ambient + diffuse + specular) * vColor;
            FragColor = vec4(result, 0.8); // Alpha 0.8 pour légère transparence
        }
    }
)";

// ==========================================
// 3. CLASSE SCENE 3D (MODERN OPENGL)
// ==========================================

class Scene3D : public QOpenGLWidget, protected QOpenGLFunctions, public SphericalVoronoiIntegratorObserver {

public:
    explicit Scene3D(QWidget *parent = nullptr) : QOpenGLWidget(parent) {
        // Configuration indispensable pour le context Core Profile
        QSurfaceFormat format;
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        format.setVersion(3, 3);
        format.setProfile(QSurfaceFormat::CompatibilityProfile);
        format.setSamples(0);
        format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
        setFormat(format);

        setWindowTitle("Voronoi Modern OpenGL");
        resize(800, 600);
    }

    ~Scene3D() {
        makeCurrent();
        m_vaoLines.destroy();
        m_vboLines.destroy();
        m_vaoPolys.destroy();
        m_vboPolys.destroy();
        doneCurrent();
    }

    // --- INTERFACE OBSERVER ---
    void on_computation_complete(const Color &total_contribution, const Vec3 &new_normal, const double total_area, const std::vector<Vec3> new_directions, const std::vector<Color> new_contributions, const std::vector<double> new_weights, const SDT& delaunay, const std::vector<std::vector<std::vector<Point_3>>>& new_faces) override {

        // 1. Conversion des données CGAL en données brutes (float)
        // On fait ça ici (Worker Thread) pour ne pas bloquer l'interface
        auto gpuData = std::make_shared<GpuData>();
        prepareGeometry(new_directions, new_contributions, new_normal, new_faces, *gpuData);

        // 2. Stockage thread-safe
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_nextData = gpuData;
            m_dataDirty = true;
        }

        // 3. Signaler à l'interface de se mettre à jour
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    }

protected:
    void initializeGL() override {
        initializeOpenGLFunctions();
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_MULTISAMPLE); // Si activé dans le format

        // --- COMPILATION SHADERS ---
        if (!m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource))
            qWarning() << "Vertex Shader Error:" << m_program.log();
        if (!m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource))
            qWarning() << "Fragment Shader Error:" << m_program.log();
        if (!m_program.link())
            qWarning() << "Link Error:" << m_program.log();

        // --- CONFIGURATION LIGNES (VAO/VBO) ---
        m_vaoLines.create();
        m_vaoLines.bind();
        m_vboLines.create();
        m_vboLines.bind();
        m_vboLines.setUsagePattern(QOpenGLBuffer::DynamicDraw);

        // Attribut 0 : Position (x, y, z)
        m_program.enableAttributeArray(0);
        m_program.setAttributeBuffer(0, GL_FLOAT, 0, 3, 6 * sizeof(float));
        // Attribut 1 : Couleur (r, g, b)
        m_program.enableAttributeArray(1);
        m_program.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));

        m_vaoLines.release();

        // --- CONFIGURATION POLYGONES (VAO/VBO) ---
        m_vaoPolys.create();
        m_vaoPolys.bind();
        m_vboPolys.create();
        m_vboPolys.bind();
        m_vboPolys.setUsagePattern(QOpenGLBuffer::DynamicDraw);

        // Stride = 9 floats (3 pos + 3 color + 3 normal)
        int stride = 9 * sizeof(float);

        // Attribut 0 : Position
        m_program.enableAttributeArray(0);
        m_program.setAttributeBuffer(0, GL_FLOAT, 0, 3, stride);
        // Attribut 1 : Couleur
        m_program.enableAttributeArray(1);
        m_program.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, stride);
        // Attribut 2 : Normale (pour la lumière)
        m_program.enableAttributeArray(2);
        m_program.setAttributeBuffer(2, GL_FLOAT, 6 * sizeof(float), 3, stride);

        m_vaoPolys.release();
    }

    void resizeGL(int w, int h) override {
        m_projection.setToIdentity();
        m_projection.perspective(45.0f, GLfloat(w) / h, 0.1f, 100.0f);
    }

    void paintGL() override {
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. Upload des données si nécessaire (Thread-Safe)
        std::shared_ptr<GpuData> currentData;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_dataDirty && m_nextData) {
                m_currentData = m_nextData;
                m_dataDirty = false;

                // Upload Lignes
                m_vboLines.bind();
                m_vboLines.allocate(m_currentData->linesData.data(), m_currentData->linesData.size() * sizeof(float));
                m_vboLines.release();

                // Upload Polygones
                m_vboPolys.bind();
                m_vboPolys.allocate(m_currentData->polysData.data(), m_currentData->polysData.size() * sizeof(float));
                m_vboPolys.release();
            }
            currentData = m_currentData;
        }

        if (!currentData) return;

        m_program.bind();

        // 2. Gestion Caméra (Model / View)
        QMatrix4x4 view;
        view.translate(0, 0, -m_dist);
        view.rotate(m_xRot, 1, 0, 0);
        view.rotate(m_yRot, 0, 1, 0);

        m_program.setUniformValue("projection", m_projection);
        m_program.setUniformValue("view", view);
        m_program.setUniformValue("lightPos", QVector3D(5.0f, 5.0f, 5.0f));
        m_program.setUniformValue("viewPos", QVector3D(0.0f, 0.0f, m_dist));

        // 3. DESSIN DES POLYGONES (VORONOI)
        // Besoin de lumière + rotation spécifique vers la normale globale
        {
            QMatrix4x4 model;
            // -- Appliquer la rotation pour aligner Z avec la normale globale des données --
            // (Logique reprise de votre ancien code : rotationTo)
            // Cependant, vos données faces semblent déjà être en coordonnées globales ?
            // Si les faces sont en coordonnées locales, décommentez ceci :
            /*
            QVector3D n(0,0,1); // Défaut
            // ... logique de rotation si besoin ...
            */

            m_program.setUniformValue("model", model);
            m_program.setUniformValue("useLighting", true); // Allumer la lumière

            m_vaoPolys.bind();
            glDrawArrays(GL_TRIANGLES, 0, currentData->polysCount);
            m_vaoPolys.release();
        }

        // 4. DESSIN DES LIGNES (AXES & DIRECTIONS)
        // Pas de lumière, couleur pure
        {
            QMatrix4x4 model; // Identité
            m_program.setUniformValue("model", model);
            m_program.setUniformValue("useLighting", false);

            m_vaoLines.bind();
            glDrawArrays(GL_LINES, 0, currentData->linesCount);
            m_vaoLines.release();
        }

        m_program.release();
    }

    // --- GESTION SOURIS ---
    void mousePressEvent(QMouseEvent *event) override {
        m_lastPos = event->pos();
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        int dx = event->pos().x() - m_lastPos.x();
        int dy = event->pos().y() - m_lastPos.y();

        if (event->buttons() & Qt::LeftButton) {
            m_xRot += dy;
            m_yRot += dx;
            update();
        }
        m_lastPos = event->pos();
    }

    void wheelEvent(QWheelEvent *event) override {
        float delta = event->angleDelta().y() / 120.0f;
        m_dist -= delta * 0.5f;
        if (m_dist < 0.1f) m_dist = 0.1f;
        update();
    }

private:
    // --- Helpers de transformation de données ---
    void prepareGeometry(const std::vector<Vec3>& dirs, const std::vector<Color>& contribs, const Vec3& normal, const std::vector<std::vector<std::vector<Point_3>>>& faces, GpuData& outData) {

        // A. Préparation des LIGNES (Rayons)
        outData.linesData.reserve((dirs.size() + 4) * 12); // +4 pour axes et normale

        // 1. Axes XYZ
        auto addLine = [&](float x1, float y1, float z1, float x2, float y2, float z2, float r, float g, float b) {
            outData.linesData.insert(outData.linesData.end(), {x1, y1, z1, r, g, b});
            outData.linesData.insert(outData.linesData.end(), {x2, y2, z2, r, g, b});
        };
        addLine(0,0,0, 1,0,0, 1,0,0); // X Rouge
        addLine(0,0,0, 0,1,0, 0,1,0); // Y Vert
        addLine(0,0,0, 0,0,1, 0,0,1); // Z Bleu

        // 2. Directions (Rayons)
        for(size_t i=0; i<dirs.size(); ++i) {
            float r = static_cast<float>(contribs[i].x()); // Supposant Color est Vec3(r,g,b)
            float g = static_cast<float>(contribs[i].y());
            float b = static_cast<float>(contribs[i].z());

            float dx = static_cast<float>(dirs[i].x() * 1.2);
            float dy = static_cast<float>(dirs[i].y() * 1.2);
            float dz = static_cast<float>(dirs[i].z() * 1.2);

            addLine(0,0,0, dx, dy, dz, r, g, b);
        }

        // 3. Normale Globale (Blanche)
        float nx = static_cast<float>(normal.x() * 1.5);
        float ny = static_cast<float>(normal.y() * 1.5);
        float nz = static_cast<float>(normal.z() * 1.5);
        addLine(0,0,0, nx, ny, nz, 1, 1, 1);

        outData.linesCount = outData.linesData.size() / 6;

        // B. Préparation des POLYGONES (Voronoi)
        // Structure : [pos(3), col(3), norm(3)]

        size_t idx = 0;
        for (const auto& face : faces) {
            if (idx >= contribs.size()) break;

            float r = static_cast<float>(contribs[idx].x());
            float g = static_cast<float>(contribs[idx].y());
            float b = static_cast<float>(contribs[idx].z());
            idx++;

            for (const auto& triangle : face) {
                if(triangle.size() < 3) continue;

                // Conversion points CGAL -> QVector3D pour calcul
                QVector3D p0(static_cast<float>(triangle[0].x()), static_cast<float>(triangle[0].y()), static_cast<float>(triangle[0].z()));
                QVector3D p1(static_cast<float>(triangle[1].x()), static_cast<float>(triangle[1].y()), static_cast<float>(triangle[1].z()));
                QVector3D p2(static_cast<float>(triangle[2].x()), static_cast<float>(triangle[2].y()), static_cast<float>(triangle[2].z()));

                // Calcul normale
                QVector3D u = p1 - p0;
                QVector3D v = p2 - p0;
                QVector3D n = QVector3D::crossProduct(u, v).normalized();

                auto addVertex = [&](QVector3D p) {
                    outData.polysData.insert(outData.polysData.end(), {
                        p.x(), p.y(), p.z(),  // Pos
                        r, g, b,              // Col
                        n.x(), n.y(), n.z()   // Norm
                    });
                };

                // On pousse les 3 sommets du triangle
                addVertex(p0);
                addVertex(p1);
                addVertex(p2);

                // Si le polygone original a plus de 3 points, il faut trianguler (fan)
                // Votre code original semblait dire que 'triangle' contient deja 3 points.
                // Sinon il faut adapter la boucle.
                for(size_t k=3; k<triangle.size(); ++k) {
                   QVector3D pk(static_cast<float>(triangle[k].x()), static_cast<float>(triangle[k].y()), static_cast<float>(triangle[k].z()));
                   addVertex(p0); // Pivot
                   QVector3D p_prev(static_cast<float>(triangle[k-1].x()), static_cast<float>(triangle[k-1].y()), static_cast<float>(triangle[k-1].z()));
                   addVertex(p_prev);
                   addVertex(pk);
                }
            }
        }
        outData.polysCount = outData.polysData.size() / 9;
    }

private:
    QOpenGLShaderProgram m_program;

    QOpenGLVertexArrayObject m_vaoLines;
    QOpenGLBuffer m_vboLines;

    QOpenGLVertexArrayObject m_vaoPolys;
    QOpenGLBuffer m_vboPolys;

    QMatrix4x4 m_projection;

    // Variables d'état
    float m_xRot = 0.0f;
    float m_yRot = 0.0f;
    float m_dist = 5.0f;
    QPoint m_lastPos;

    // Gestion Threading
    std::mutex m_mutex;
    std::shared_ptr<GpuData> m_currentData; // Données actuellement affichées
    std::shared_ptr<GpuData> m_nextData;    // Données en attente
    bool m_dataDirty = false;
};

// ==========================================
// 4. MAIN
// ==========================================

int main(int argc, char **argv) {
    qputenv("QT_QPA_PLATFORM", "xcb");
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

    QApplication app(argc, argv);

    // Création de la fenêtre Scene3D (shared_ptr pour que l'observer reste vivant)
    auto window = std::make_shared<Scene3D>();

    // Ajout à la liste des observers
    std::vector<std::shared_ptr<SphericalVoronoiIntegratorObserver>> observers;
    observers.push_back(window);

    const auto sampling_strategy = camera->samplingStrategy;

    camera->scattering_strategy =
        std::make_shared<ObservableCVorNestedRayEvaluator>(
            sampling_strategy,
            make_shared<SimpleRayEvaluator>(sampling_strategy),
            config.nested_sample_size,
            observers
        );

    // Lancement du calcul (probablement async)
    camera->render(scene);

    window->show();
    return app.exec();
}