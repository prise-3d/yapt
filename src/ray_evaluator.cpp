//
// Created by franck on 30/01/2026.
//

#include "ray_evaluator.h"

Color SimpleRayEvaluator::evaluate(const Ray &r, const int depth, const Scene &scene, const Color &background) {
    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return {0, 0, 0};

    HitRecord rec;
    // If the ray hits nothing, return the background color.
    if (!scene.geometry.hit(r, Interval(0.001, infinity), rec))
        return background;

    ScatterRecord scatterRecord;
    const Color color_from_emission = rec.mat->emitted(r, rec, rec.u, rec.v, rec.p);

    if (!rec.mat->scatter(r, rec, scatterRecord))
        return color_from_emission;

    if (scatterRecord.skip_pdf) {
        return scatterRecord.attenuation * evaluate(scatterRecord.skip_pdf_ray, depth - 1, scene, background);
    }

    // Delegate to the sampling strategy
    SamplingStrategy::SamplingContext ctx{r, rec, scatterRecord, scene, depth - 1};

    auto ray_color_function = [this, &scene, background](const Ray& ray, const int d) {
        return this->evaluate(ray, d, scene, background);
    };

    const ScatteredContribution contribution = sampling_strategy->compute_scattered_color(ctx, ray_color_function);
    const Color colorFromScatter = contribution.color;

    return color_from_emission + colorFromScatter;
}

Color NormalRayEvaluator::evaluate(const Ray &ray, const int depth, const Scene &scene, const Color &background) {
    if (depth <= 0)
        return {0, 0, 0};

    HitRecord rec;
    // If the ray hits nothing, return the background color.
    if (!scene.geometry.hit(ray, Interval(0.001, infinity), rec))
        return background;

    const auto n = unit_vector(rec.normal);

    double red = (n.x() + 1.) / 2.;
    double green = (n.y() + 1.) / 2.;
    double blue = (n.z() + 1.) / 2.;

    return {red, green, blue};
}

Color NestedRayEvaluator::evaluate(const Ray &r, const int depth, const Scene &scene, const Color &background) {
    HitRecord rec;
    // If the ray hits nothing, return the background color.
    if (!scene.geometry.hit(r, Interval(0.001, infinity), rec))
        return background;

    ScatterRecord scatterRecord;
    const Color color_from_emission = rec.mat->emitted(r, rec, rec.u, rec.v, rec.p);

    if (!rec.mat->scatter(r, rec, scatterRecord))
        return color_from_emission;

    if (scatterRecord.skip_pdf) {
        return scatterRecord.attenuation * evaluate(scatterRecord.skip_pdf_ray, depth - 1, scene, background);
    }
    // end of standard ray tracing algorithm
    // scattering

    // Delegate to the sampling strategy
    SamplingStrategy::SamplingContext ctx{r, rec, scatterRecord, scene, depth - 1};

    auto ray_color_function = [this, &scene, background](const Ray& ray, const int d) {
        return next_step->evaluate(ray, d, scene, background);
    };

    Color colorFromScatter(0,0,0);

    for (int i = 0 ; i < sample_size ; ++i) {
        const ScatteredContribution contribution = sampling_strategy->compute_scattered_color(ctx, ray_color_function);
        colorFromScatter += contribution.color;
    }


    colorFromScatter /= static_cast<double>(sample_size);

    return color_from_emission + colorFromScatter;
}

#include "aggregators.h"

Color CVorNestedRayEvaluator::evaluate(const Ray &r, const int depth, const Scene &scene, const Color &background) {
     HitRecord rec;
     // If the ray hits nothing, return the background color.
     if (!scene.geometry.hit(r, Interval(0.001, infinity), rec))
         return background;

     ScatterRecord scatterRecord;
     const Color color_from_emission = rec.mat->emitted(r, rec, rec.u, rec.v, rec.p);

     if (!rec.mat->scatter(r, rec, scatterRecord))
         return color_from_emission;

     if (scatterRecord.skip_pdf) {
         return scatterRecord.attenuation * evaluate(scatterRecord.skip_pdf_ray, depth - 1, scene, background);
     }
     // end of standard ray tracing algorithm
     // scattering

     // Delegate to the sampling strategy
     SamplingStrategy::SamplingContext ctx{r, rec, scatterRecord, scene, depth - 1};

     auto ray_color_function = [this, &scene, &background](const Ray& ray, int d) {
         return next_step->evaluate(ray, d, scene, background);
     };

     // now this is dirty
     FirstBounceVoronoi ag;
     Traits traits(Point_3(0, 0, 0), 1.0); // Unit sphere
     auto dt = SDT(traits);

     std::vector<Vec3> directions;
     std::vector<Color> contributions;
     std::vector<double> weights;

     double total_area = 0.;

     for (int i = 0 ; i < sample_size ; ++i) {
         const ScatteredContribution contribution = sampling_strategy->compute_scattered_color(ctx, ray_color_function);
         const Color colorFromScatter = contribution.color;
         auto direction = contribution.outgoing.direction();
         direction /= direction.length();
         directions.push_back(direction);
         contributions.push_back(colorFromScatter);
         dt.insert(Point_3(direction.x(), direction.y(), direction.z()));
     }

     for (auto &direction: directions) {
         const Vec3 ref = reflect(direction, rec.normal);
         dt.insert(Point_3(ref.x(), ref.y(), ref.z()));
     }

     for (auto v = dt.vertices_begin(); v != dt.vertices_end() ; ++v) {
         const Point_3 site = v->point();
         const Vec3 p(site.x(), site.y(), site.z());
         if (dot(p, rec.normal) <= 0) continue;

         double cell_solid_angle = 0.0;
         SDT::Face_circulator fc = dt.incident_faces(v), done(fc);

         std::vector<Point_3> voronoi_vertices;
         if (fc != nullptr) {
             do {
                 // if samples are drawn from a hemisphere, dt.is_infinite() may return true
                 if (!dt.is_infinite(fc)) {
                     Point_3 dual = ag.get_spherical_dual(fc);
                     voronoi_vertices.push_back(dual);
                 }
             } while (++fc != done);
         }
         if (!voronoi_vertices.empty()) {
             for (std::size_t i = 0; i < voronoi_vertices.size(); ++i) {
                 const Point_3& v1 = voronoi_vertices[i];
                 const Point_3& v2 = voronoi_vertices[(i + 1) % voronoi_vertices.size()];
                 cell_solid_angle += ag.solid_angle(site, v1, v2);
             }
             weights.push_back(cell_solid_angle);
         }
         total_area += cell_solid_angle;
     }

     Color colorFromScatter(0,0,0);

     for (int i = 0 ; i < sample_size ; ++i) {
         colorFromScatter += weights[i] * contributions[i];
     }

     colorFromScatter /= total_area;

     return color_from_emission + colorFromScatter;
}


