//
// Created by franck on 02/02/2026.
//

#ifndef YAPT_SPHERICAL_VORONOI_H
#define YAPT_SPHERICAL_VORONOI_H

#include "aggregators.h"
// typedefs for Voronoi diagrams on the surface of a sphere
typedef CGAL::Projection_on_sphere_traits_3<K> Traits;
typedef CGAL::Delaunay_triangulation_on_sphere_2<Traits> SDT;
typedef K::Point_3 Point_3;
typedef K::Vector_3 Vector_3;

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

#endif //YAPT_SPHERICAL_VORONOI_H