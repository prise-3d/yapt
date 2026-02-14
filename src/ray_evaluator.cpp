//
// Created by franck on 30/01/2026.
//

#include "ray_evaluator.h"
#include "spherical_voronoi.h"

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

Color CVorNestedRayEvaluator::evaluate(const Ray &r, const int depth, const Scene &scene, const Color &background) {
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
    // end of standard ray tracing algorithm
    // scattering

    // Delegate to the sampling strategy
    const SamplingStrategy::SamplingContext ctx{r, rec, scatterRecord, scene, depth - 1};

    auto ray_color_function = [this, &scene, &background](const Ray& ray, const int d) {
        return next_step->evaluate(ray, d, scene, background);
    };

    SphericalVoronoiIntegrator integrator(rec.normal);

    for (int i = 0 ; i < sample_size ; ++i) {
        const ScatteredContribution contribution = sampling_strategy->compute_scattered_color(ctx, ray_color_function);
        const Color colorFromScatter = contribution.color;
        auto direction = contribution.outgoing.direction();
        direction /= direction.length();
        integrator.add_contribution(direction, colorFromScatter);
    }

    return color_from_emission + integrator.integrate();
}

shared_ptr<SphericalVoronoiIntegrator> SphericalVoronoiIntegratorFactory::create(const Vec3 &normal) {
    return std::make_shared<SphericalVoronoiIntegrator>(normal);
}

// shared_ptr<SphericalVoronoiIntegrator> ObservableSphericalVoronoiIntegratorFactory::create(const Vec3 &normal) {
//     return std::make_shared<ObservableSphericalVoronoiIntegratorFactory>(normal);
// }


//TODO: this is super dirty
Color ObservableCVorNestedRayEvaluator::evaluate(const Ray &r, const int depth, const Scene &scene, const Color &background) {
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
    // end of standard ray tracing algorithm
    // scattering

    // Delegate to the sampling strategy
    const SamplingStrategy::SamplingContext ctx{r, rec, scatterRecord, scene, depth - 1};

    auto ray_color_function = [this, &scene, &background](const Ray& ray, const int d) {
        return next_step->evaluate(ray, d, scene, background);
    };

    ObservableSphericalVoronoiIntegrator integrator(rec.normal, observers);

    for (int i = 0 ; i < sample_size ; ++i) {
        const ScatteredContribution contribution = sampling_strategy->compute_scattered_color(ctx, ray_color_function);
        const Color colorFromScatter = contribution.color;
        auto direction = contribution.outgoing.direction();
        direction /= direction.length();
        integrator.add_contribution(direction, colorFromScatter);
    }

    return color_from_emission + integrator.integrate();
}

void ObservableCVorNestedRayEvaluator::clear_observers()
{
    observers.clear();
}



