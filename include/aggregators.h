#ifndef YAPT_AGGREGATORS_H
#define YAPT_AGGREGATORS_H

#include "Vec3.h"
#include "color.h"
#include "sampler.h"
#include <memory>
#include <vector>
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

class SampleAggregator {
public:
    virtual ~SampleAggregator() = default;
    virtual Color aggregate() = 0;
    virtual void sample_from(std::shared_ptr<SamplerFactory>, double x, double y) = 0;
    virtual void insert_contribution(Color color);
    using const_iterator = std::vector<Sample>::const_iterator;

    const_iterator begin() const {
        return _samples.begin();
    }

    const_iterator end() const {
        return _samples.begin() + _usable_sample_count;
    }

    std::vector<Sample> _samples;
    std::vector<Color> contributions;

protected:
    std::size_t _usable_sample_count = 0;
    std::size_t _total_sample_count = 0;
};

class MCSampleAggregator : public SampleAggregator {
public:
    MCSampleAggregator();
    Color aggregate() override;
    void sample_from(std::shared_ptr<SamplerFactory>, double x, double y) override;
};

class MedianAggregator: public MCSampleAggregator {
public:
    MedianAggregator() = default;
    Color aggregate() override;
};

class MonAggregator: public MCSampleAggregator {
public:
    explicit MonAggregator(size_t nb_block);
    Color aggregate() override;

private:
    size_t nb_block;
};

class WinsorAggregator: public MCSampleAggregator {
public:
    WinsorAggregator(double rejectRate, bool clipped);
    Color aggregate() override;

private:
    bool clipped;
    double rejectRate;
};

class VoronoiAggregator: public SampleAggregator {
public:
    VoronoiAggregator() = default;

    /**
     * collect samples from a pixel sampler. Makes sure the sampling is correct
     * ie: no sample drawn inside the pixel has an infinite area
     * @param factory pixel sampler factory
     * @param x x coordinate of the pixel to sample
     * @param y y coordinate of the pixel to sample
     */
    void sample_from(std::shared_ptr<SamplerFactory> factory, double x, double y) override;
    Color aggregate() override;
    void fill_delaunay();

    Voronoi voronoi;
    Delaunay delaunay;
    std::vector<double> weights;

protected:
    double compute_voronoi_cell_area(Face_handle face) const;
};


class FilteringVoronoiAggregator: public VoronoiAggregator {
public:
    double margin;

    explicit FilteringVoronoiAggregator();
    explicit FilteringVoronoiAggregator(double margin);

    Color aggregate() override;
    bool is_good(const Point &point) const;
};

class ClippedVoronoiAggregator: public VoronoiAggregator {
public:
    ClippedVoronoiAggregator() = default;
    void sample_from(std::shared_ptr<SamplerFactory> factory, double x, double y) override;
};

class NicoVoronoiAggregator: public VoronoiAggregator {
public:
    double margin;

    explicit NicoVoronoiAggregator();
    explicit NicoVoronoiAggregator(double margin);

    void sample_from(std::shared_ptr<SamplerFactory> factory, double x, double y) override;
};

class AggregatorFactory {
public:
    virtual ~AggregatorFactory() = default;

    virtual std::shared_ptr<SampleAggregator> create() = 0;
};

class MCAggregatorFactory: public AggregatorFactory {
public:
    std::shared_ptr<SampleAggregator> create() override;
};

class VoronoiAggregatorFactory: public AggregatorFactory {
public:
    std::shared_ptr<SampleAggregator> create() override;
};

class ClippedVoronoiAggregatorFactory: public AggregatorFactory {
public:
    shared_ptr<SampleAggregator> create() override;
};

class MedianAggregatorFactory: public AggregatorFactory {
public:
    shared_ptr<SampleAggregator> create() override;
};

class MonAggregatorFactory: public AggregatorFactory {
public:
    explicit MonAggregatorFactory(size_t nb_blocks);
    shared_ptr<SampleAggregator> create() override;

private:
    size_t nb_blocks;
};

class WinsorAggregatorFactory: public AggregatorFactory {
public:
    WinsorAggregatorFactory(double rejectRate, bool clipped);
    shared_ptr<SampleAggregator> create() override;

private:
    double rejectRate;
    bool clipped;
};

class FilteringVoronoiAggregatorFactory: public AggregatorFactory {
public:
    FilteringVoronoiAggregatorFactory();
    explicit FilteringVoronoiAggregatorFactory(double m);

    shared_ptr<SampleAggregator> create() override;

private:
    double margin;
};

class NicoVoronoiAggregatorFactory: public AggregatorFactory {
public:
    NicoVoronoiAggregatorFactory();
    explicit NicoVoronoiAggregatorFactory(double m);

    shared_ptr<SampleAggregator> create() override;

private:
    double margin;
};

/*
 * Pour échantillonner l'hémisphère des directions, on a toujours le même problème : le bord
 * pour imiter l'approche à l'échelle du pixel, on peut envisager de mettre au point une
 * correspondance carré <-> hémisphère par une transformation qui aurait ces propriétés :
 *  - la transformation est conforme, de façon à préserver les relations de proximité
 *  - la transformation envoie le plan muni d'un point à l'infini sur la sphère unité
 *  - l'intérieur du carré est envoyé dans l'hémiphère
 *  - le bord du carré est envoyé au bord de l'hémisphère, ie le cercle des points (x,y,0) tels que x²+y² = 1
 *
 * Le plus simple serait quand même d'échantillonner dans un disque (r, \theta) et de rabattre
 * la distribution par projection stéréographique. La contrainte d'intégrité sur une marge assurant
 * un Voronoi avec de bonnes propriétés serait maintenue
 *
 * L'aggrégateur c'est pas le bon endroit pour faire ce genre de choses, il faudrait une caméra Voronoi
 *
 */

class FirstBounceVoronoi: public SampleAggregator {
public:
    double solid_angle(const Point_3& p1, const Point_3& p2, const Point_3& p3) {
        const Vector_3 a = p1 - CGAL::ORIGIN;
        const Vector_3 b = p2 - CGAL::ORIGIN;
        const Vector_3 c = p3 - CGAL::ORIGIN;

        // see pbrt v4 for solid angle of a spherical triangle
        // https://www.pbr-book.org/4ed/Geometry_and_Transformations/Spherical_Geometry#SphericalPolygons
        const double numerator = CGAL::scalar_product(a, CGAL::cross_product(b, c));
        const double denominator = 1.0 + (a * b) + (a * c) + (b * c);

        return std::abs(2.0 * std::atan2(numerator, denominator));
    }

    Point_3 get_spherical_dual(const SDT::Face_handle& f) {
        const Point_3& p0 = f->vertex(0)->point();
        const Point_3& p1 = f->vertex(1)->point();
        const Point_3& p2 = f->vertex(2)->point();

        // normal to the triangle plane : (p1-p0) x (p2-p0)
        Vector_3 v0 = p0 - CGAL::ORIGIN;
        Vector_3 v1 = p1 - CGAL::ORIGIN;
        Vector_3 v2 = p2 - CGAL::ORIGIN;

        // direction to the spherical circumcenter of the triangle
        Vector_3 normal = CGAL::cross_product(v1 - v0, v2 - v0);

        // take colinear points into account
        if (normal.squared_length() < 1e-15) {
            return Point_3(0,0,0);
        }

        // normalization
        normal = normal / std::sqrt(normal.squared_length());

        // make sure the orientation is correct (ie pointing towards the triangle)
        if (normal * v0 < 0) {
            normal = -normal;
        }

        return CGAL::ORIGIN + normal;
    }

    inline Point_3 sample_to_sphere(Sample sample, bool under) {
        double t = sample.dx * M_PI; // [-PI / 2 ; PI / 2)
        if (under) t += M_PI;
        const double p = 2 * sample.dy * M_PI; // [-PI ; PI)
        const double sint = std::sin(t);
        const double cosp = std::cos(p);
        const double sinp = std::sin(p);
        double cost = std::cos(t);

        return {
            sint * cosp,
            sint * sinp,
            cost
        };
    }

    void sample_from(std::shared_ptr<SamplerFactory> factory, double x, double y) override {
        SampleAggregator::sample_from(factory, x, y);
        contributions.clear();
        contributions.reserve(_usable_sample_count);

        Traits traits(Point_3(0, 0, 0), 1.0); // Unit sphere
        dt = SDT(traits);

        for (const auto &sample : _samples) {
            dt.insert(sample_to_sphere(sample, false));

            // Let A = (0,0,-1) \in S, the unit sphere
            // Let p = (x,y,0) an extra point from a SPPP distribution
            // Let us transform p by intersecting the line (Ax) with S into a point M \in S, M != A
            // M verifies : M = A + k(p - A) for some k > 0
            // ie           M = (kx, ky, k-1) for some k > 0
            // and M \in S =>   (kx)² + (ky)² + (k-1)² = 1
            //             => k²(x²+y²+1) - 2k = 0
            //             => k(k(x²+y²+1) -2) = 0
            //             => k(x²+y²+1) -2 = 0 (because M != A => k !=0)
            //             => k = 2 / (x²+y²+1)
            // Note that in our context, (x,y,0) is outside S, meaning that
            //                (x²+y²+1) > 2, hence
            //                k < 1, which complies with intuition
        }

        for (const auto &sample: _samples) {
            dt.insert(sample_to_sphere(sample, true)); // clipping
        }

        double total_area = 0.0;

        // we visit every vertex of the DT
        for (auto v = dt.finite_vertices_begin(); v != dt.finite_vertices_end(); ++v) {
            Point_3 site = v->point();
            if (site.z() < 0) continue;

            double cell_solid_angle = 0.0;
            SDT::Face_circulator fc = dt.incident_faces(v), done(fc);
            std::vector<Point_3> voronoi_vertices;

            if (fc != nullptr) {
                do {
                    // if samples are drawn from a hemisphere, dt.is_infinite() may return true
                    if (!dt.is_infinite(fc)) {
                        Point_3 p = get_spherical_dual(fc);
                        voronoi_vertices.push_back(p);
                    }
                } while (++fc != done);
            }

            // to compute the solid angle of a Voronoi cell, we compute
            // the sum of every spherical triangle solid angle
            if (!voronoi_vertices.empty()) {
                for (std::size_t i = 0; i < voronoi_vertices.size(); ++i) {
                    const Point_3& v1 = voronoi_vertices[i];
                    const Point_3& v2 = voronoi_vertices[(i + 1) % voronoi_vertices.size()];
                    cell_solid_angle +=  solid_angle(site, v1, v2);
                }
            }

            std::cout << "Site (" << site << ") -> Solid angle = " << cell_solid_angle << " sr" << std::endl;
            total_area += cell_solid_angle;
        }

        std::cout << "------------------------------------------------" << std::endl;
        std::cout << "Total area : " << total_area << " (Expected : " << 4 * M_PI << ")" << std::endl;

        std::cout << "    points = np.array([" << std::endl;
        for (auto v = dt.finite_vertices_begin(); v != dt.finite_vertices_end(); ++v) {
            Point_3 p = v->point();

            std::cout << "        [" << p.x() << ", " << p.y() << ", " << p.z() << "]," << std::endl;

        }
        std::cout << "    ])" << std::endl;

        std::exit(EXIT_SUCCESS);
    }

    Color aggregate() override {
        return {0, 0, 0};
    }
    void insert_contribution(Color color) override {}

    SDT dt;
};

class FirstBounceVoronoiFactory: public AggregatorFactory {
public:
    FirstBounceVoronoiFactory() = default;
    shared_ptr<SampleAggregator> create() override {
        return make_shared<FirstBounceVoronoi>();
    }
};

#endif //YAPT_AGGREGATORS_H
