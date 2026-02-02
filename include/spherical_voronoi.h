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

class SphericalVoronoiIntegrator {
public:
    static double solid_angle(const Point_3& p1, const Point_3& p2, const Point_3& p3);
    static Point_3 get_spherical_dual(const SDT::Face_handle& f);
    static Point_3 sample_to_sphere(Sample sample, bool under);
    void add_contribution(const Vec3 &direction, const Color &contribution);
    Color integrate();
    explicit SphericalVoronoiIntegrator(const Vec3 &normal);

protected:
    SDT dt;
    Traits traits;
    std::vector<Vec3> directions;
    std::vector<Color> contributions;
    std::vector<double> weights;
    Vec3 normal;
};

#endif //YAPT_SPHERICAL_VORONOI_H