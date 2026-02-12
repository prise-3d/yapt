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

#ifndef YAPT_SPHERICAL_VORONOI_H
#define YAPT_SPHERICAL_VORONOI_H

#include "aggregators.h"
// typedefs for Voronoi diagrams on the surface of a sphere
typedef CGAL::Projection_on_sphere_traits_3<K> Traits;
typedef CGAL::Delaunay_triangulation_on_sphere_2<Traits> SDT;
typedef K::Point_3 Point_3;
typedef K::Vector_3 Vector_3;

class SphericalVoronoiIntegratorObserver {
public:
    virtual ~SphericalVoronoiIntegratorObserver() = default;
    virtual void on_computation_complete(
        const Color &total_contribution,
        const Vec3 &normal,
        const double total_area,
        const std::vector<Vec3> directions,
        const std::vector<Color> contributions,
        const std::vector<double> weights,
        const SDT &delaunay,
        const std::vector<std::vector<std::vector<Point_3>>>&) = 0;
};

class SphericalVoronoiIntegrator {
public:
    virtual ~SphericalVoronoiIntegrator() = default;

    static double solid_angle(const Point_3& p1, const Point_3& p2, const Point_3& p3);
    static Point_3 get_spherical_dual(const SDT::Face_handle& f);
    void add_contribution(const Vec3 &direction, const Color &contribution);
    virtual Color integrate();
    explicit SphericalVoronoiIntegrator(const Vec3 &normal);

protected:
    SDT dt;
    Traits traits;
    std::vector<Vec3> directions;
    std::vector<Color> contributions;
    std::vector<Color> auxiliary_contributions;
    std::vector<double> weights;
    Vec3 normal;
    double total_area;
    Color contribution;
};

class ObservableSphericalVoronoiIntegrator : public SphericalVoronoiIntegrator {
public:
    explicit ObservableSphericalVoronoiIntegrator(const Vec3 &normal, const std::vector<shared_ptr<SphericalVoronoiIntegratorObserver>> &observers): SphericalVoronoiIntegrator(normal), observers(observers) {}
    Color integrate() override;

protected:
    std::vector<std::shared_ptr<SphericalVoronoiIntegratorObserver>> observers;
};

class SphericalVoronoiIntegratorFactory {
public:
    SphericalVoronoiIntegratorFactory() = default;

    virtual ~SphericalVoronoiIntegratorFactory() = default;
    virtual shared_ptr<SphericalVoronoiIntegrator> create(const Vec3 &normal);
};

class ObservableSphericalVoronoiIntegratorFactory : public SphericalVoronoiIntegratorFactory {
public:
    ObservableSphericalVoronoiIntegratorFactory() = default;
    ~ObservableSphericalVoronoiIntegratorFactory() override = default;
    shared_ptr<SphericalVoronoiIntegrator> create(const Vec3 &normal) override;
};

#endif //YAPT_SPHERICAL_VORONOI_H