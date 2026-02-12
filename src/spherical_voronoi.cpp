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

#include "spherical_voronoi.h"

double SphericalVoronoiIntegrator::solid_angle(const Point_3& p1, const Point_3& p2, const Point_3& p3)  {
    const Vector_3 a = p1 - CGAL::ORIGIN;
    const Vector_3 b = p2 - CGAL::ORIGIN;
    const Vector_3 c = p3 - CGAL::ORIGIN;

    // see pbrt v4 for solid angle of a spherical triangle
    // https://www.pbr-book.org/4ed/Geometry_and_Transformations/Spherical_Geometry#SphericalPolygons
    const double numerator = CGAL::scalar_product(a, CGAL::cross_product(b, c));
    const double denominator =
        1.0 + CGAL::scalar_product(a, b) + CGAL::scalar_product(a, c) + CGAL::scalar_product(b, c);

    return std::abs(2.0 * std::atan2(numerator, denominator));
}
Point_3 SphericalVoronoiIntegrator::get_spherical_dual(const SDT::Face_handle & f) {
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

SphericalVoronoiIntegrator::SphericalVoronoiIntegrator(const Vec3 &normal)  : normal(normal), total_area(0.0) {
    traits = Traits(Point_3(0, 0, 0), 1.0); // Unit sphere
    dt = SDT(traits);
}


void SphericalVoronoiIntegrator::add_contribution(const Vec3 &direction, const Color &new_contribution) {
    directions.push_back(direction);
    contributions.push_back(new_contribution);
    dt.insert(Point_3(direction.x(), direction.y(), direction.z()));
}

Color SphericalVoronoiIntegrator::integrate() {
    contribution = {0, 0, 0};
    total_area = 0.0;

    for (auto &direction: directions) {
        const Vec3 ref = reflect(direction, normal);
        auxiliary_contributions.push_back(ref);
        dt.insert(Point_3(ref.x(), ref.y(), ref.z()));
    }

    for (auto v = dt.vertices_begin(); v != dt.vertices_end() ; ++v) {
        const Point_3 site = v->point();
        const Vec3 p(site.x(), site.y(), site.z());
        if (dot(p, normal) <= 0) continue;

        double cell_solid_angle = 0.0;
        SDT::Face_circulator fc = dt.incident_faces(v), done(fc);

        std::vector<Point_3> voronoi_vertices;
        if (fc != nullptr) {
            do {
                // if samples are drawn from a hemisphere, dt.is_infinite() may return true
                if (!dt.is_infinite(fc)) {
                    const Point_3 dual = get_spherical_dual(fc);
                    voronoi_vertices.push_back(dual);
                }
            } while (++fc != done);
        }
        if (!voronoi_vertices.empty()) {
            for (std::size_t i = 0; i < voronoi_vertices.size(); ++i) {
                const Point_3& v1 = voronoi_vertices[i];
                const Point_3& v2 = voronoi_vertices[(i + 1) % voronoi_vertices.size()];
                cell_solid_angle += solid_angle(site, v1, v2);
            }
            weights.push_back(cell_solid_angle);
        }
        total_area += cell_solid_angle;
    }

    for (int i = 0 ; i < directions.size() ; ++i) {
        contribution += weights[i] * contributions[i];
    }

    contribution /= total_area;

    for (auto c : auxiliary_contributions) directions.push_back(c);

    return contribution;
}

Color ObservableSphericalVoronoiIntegrator::integrate() {
    SphericalVoronoiIntegrator::integrate();
    std::vector<std::vector<std::vector<Point_3>>> faces;

    // THIS IS DIRTY
    for (auto v = dt.vertices_begin(); v != dt.vertices_end() ; ++v) {
        const Point_3 site = v->point();
        const Vec3 p(site.x(), site.y(), site.z());
        if (dot(p, normal) <= 0) continue;

        double cell_solid_angle = 0.0;
        SDT::Face_circulator fc = dt.incident_faces(v), done(fc);

        std::vector<Point_3> voronoi_vertices;
        if (fc != nullptr) {
            do {
                // if samples are drawn from a hemisphere, dt.is_infinite() may return true
                if (!dt.is_infinite(fc)) {
                    const Point_3 dual = get_spherical_dual(fc);
                    voronoi_vertices.push_back(dual);
                }
            } while (++fc != done);
        }
        if (!voronoi_vertices.empty()) {
            std::vector<std::vector<Point_3>> face;
            for (std::size_t i = 0; i < voronoi_vertices.size(); ++i) {
                std::vector<Point_3> triangle;

                const Point_3& v1 = voronoi_vertices[i];
                const Point_3& v2 = voronoi_vertices[(i + 1) % voronoi_vertices.size()];
                triangle.push_back(site);
                triangle.push_back(v1);
                triangle.push_back(v2);
                face.push_back(triangle);
                cell_solid_angle += solid_angle(site, v1, v2);
            }
            faces.push_back(face);
            weights.push_back(cell_solid_angle);
        }
        total_area += cell_solid_angle;
    }

    for (const auto &observer : observers) {
        observer->on_computation_complete(
            contribution,
            normal,
            total_area,
            directions,
            contributions,
            weights,
            dt,
            faces
        );
    }
    return contribution;
}
