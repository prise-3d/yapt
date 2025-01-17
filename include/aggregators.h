//
// Created by franck on 19/09/24.
//



#ifndef YAPT_AGGREGATORS_H
#define YAPT_AGGREGATORS_H

#include "Vec3.h"
#include "color.h"

class SampleAggregator {
public:
    virtual ~SampleAggregator() = default;
    virtual Color aggregate() = 0;
    virtual void sampleFrom(std::shared_ptr<SamplerFactory>, double x, double y) = 0;
    virtual void insertContribution(Color color) = 0;
    virtual void traverse() = 0;
    virtual bool hasNext() = 0;
    virtual Sample next() = 0;
};

class MCSampleAggregator : public SampleAggregator {
public:
    MCSampleAggregator() {
        contributions_index = 0;
    }

    void sampleFrom(std::shared_ptr<SamplerFactory> factory, const double x, const double y) override {
        const auto pixelSampler = factory->create(x, y);
        pixelSampler->begin();
        const std::size_t size = pixelSampler->sampleSize();
        samples = std::vector<Sample>(size);
        contributions = std::vector<Color>(size);

        std::size_t i = 0;
        for ( ; pixelSampler->hasNext() ; i++) {
            samples[i] = pixelSampler->get();
        }

        contributions_index = 0;
    }

    Color aggregate() override {
        Vec3 v(0, 0,0);

        for (const auto & color : contributions) {
            v += color;
        }

        return v / static_cast<double>(samples.size());
    }

    void insertContribution(Color color) override {
        contributions[contributions_index] = color;
    }

    void traverse() override {
        current_index = nextIndexFrom(-1);
        if (current_index < 0) can_traverse = false;
        else can_traverse = true;
    }

    bool hasNext() override {
        return can_traverse;
    }

    Sample next() override {
        contributions_index = current_index;
        const Sample sample = samples[current_index];
        current_index = nextIndexFrom(current_index);
        if (current_index < 0) can_traverse = false;
        else can_traverse = true;

        return sample;
    }

private:
    int nextIndexFrom(std::size_t start) {
        size_t i = start + 1;
        int value_found = -1;

        bool search = i < samples.size();
        bool found = false;

        while (search && !found) {
            Sample sample = samples[i];
            if (sample.dx > -.5 && sample.dx < .5 && sample.dy > -.5 && sample.dy < .5) {
                found = true;
                value_found = static_cast<int>(i);
            }
            ++i;
            search = i < samples.size();
        }
        return value_found;
    }

protected:
    std::vector<Sample> samples;
    std::vector<Color> contributions;
    std::size_t contributions_index;
    int current_index;
    bool can_traverse;
};

#include <cassert>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Voronoi_diagram_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_traits_2.h>
#include <CGAL/Delaunay_triangulation_adaptation_policies_2.h>

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

class MedianAggregator: public MCSampleAggregator {
public:
    MedianAggregator() = default;
    Color aggregate() override {

        std::sort(contributions.begin(), contributions.end(), [](const Color & a, const Color & b) {
            return luminance(a) < luminance(b);
        });
        const int mid = contributions.size() / 2;
        return contributions[mid];
    }
};

class MonAggregator: public MCSampleAggregator {
public:
    MonAggregator(size_t nb_block) : MCSampleAggregator(), nb_block(nb_block) {}

    Color aggregate() override {
        std::vector<Color> block(nb_block);
        std::vector<size_t> block_size(nb_block);

        // fold each block
        for (size_t i = 0; i < contributions.size(); ++i) {
            block[i % nb_block] += contributions[i];
            block_size[i % nb_block] += 1;
        }
        // compute the mean in each block and sort
        for (size_t i = 0; i < nb_block; ++i){
            block[i] /= static_cast<double>(block_size[i]);
        }
        std::sort(block.begin(), block.end(), [](const Color & a, const Color & b) {
            return luminance(a) < luminance(b);
        });
        // if the number of block is even, return de mean of central block
        // else return the value of the central block
        if (nb_block % 2 == 0)
            return (block[nb_block / 2] + block[nb_block / 2 - 1]) / 2.0;
        else
            return block[nb_block / 2];
    }

private:
    size_t nb_block;
};

class WinsorAggregator: public MCSampleAggregator {
public:
    WinsorAggregator(double rejectRate, bool clipped) : MCSampleAggregator(), rejectRate(rejectRate), clipped(clipped) {}
    Color aggregate() override {

        std::sort(contributions.begin(), contributions.end(), [](const Color & a, const Color & b) {
            return luminance(a) < luminance(b);
        });
        auto size = contributions.size();

        const auto min = static_cast<size_t>(size * rejectRate / 2.0);
        const auto max = static_cast<size_t>(size * (1 - rejectRate / 2.0));

        Color sum(0, 0, 0);

        size_t corrected_size;

        if (!clipped) {
            for (size_t i = 0 ; i < min - 1 ; ++i) {
                sum += contributions[min];
            }
            for (size_t i = max + 1 ; i < size ; ++i) {
                sum += contributions[max];
            }
            corrected_size = size;
        } else {
            corrected_size = max - min;
        }

        for (size_t i = min; i < max; ++i) {
            sum += contributions[i];
        }

        return sum / corrected_size;
    }

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
    void sampleFrom(std::shared_ptr<SamplerFactory> factory, double x, double y) override {
        bool isInvalid = false;

        do {
            isInvalid = false;
            // we collect samples
            const auto pixelSampler = factory->create(x, y);
            pixelSampler->begin();
            const std::size_t size = pixelSampler->sampleSize();
            samples = std::vector<Sample>(size);
            contributions = std::vector<Color>(size);
            std::size_t i = 0;
            for (; pixelSampler->hasNext(); i++) {
                const auto sample = pixelSampler->get();
                samples[i] = sample;
                delaunay.insert(Point(sample.dx, sample.dy));
            }

            // we now need to construct the voronoi diagram to check if it satisfies our
            // robustness criterion
            voronoi = Voronoi(delaunay);

            for (auto f = voronoi.unbounded_faces_begin(); f != voronoi.unbounded_faces_end(); ++f) {
                const auto site = f->dual()->point();
                if (site.x() < -.5 || site.x() >= .5 || site.y() < -.5 || site.y() >= .5) continue;
                isInvalid = true;
                delaunay.clear();
                break;
            }
        } while (isInvalid);
        current_index = 0;
    }

    Color aggregate() override {
        weights = std::vector<double>(samples.size());
        double total_weight = 0.;
        int idx = 0;

        for (auto vertex = delaunay.vertices_begin() ; vertex != delaunay.vertices_end() ; ++vertex) {
            const Point &site = vertex->point();
            if (site.x() < -.5 || site.x() >= .5 || site.y() < -.5 || site.y() >= .5) continue;

            Face_handle face = voronoi.dual(vertex);

            Polygon polygon;

            Ccb_halfedge_circulator halfEdge = face->ccb(), done(halfEdge);

            do {
                polygon.push_back(halfEdge->source()->point());
            } while (++halfEdge != done);

            const double area = polygon.area();
            weights[idx] = area;
            total_weight += area;
            ++idx;
        }

        Color color(0, 0, 0);

        // And finally, we weight the samples
        for (int i = 0 ; i < samples.size() ; i++) {
            const double weight = weights[i];
            color += weight * contributions[i];
        }

        return color / total_weight;
    }

    void insertContribution(Color color) override {
        contributions[contributions_index] = color;
    }

    void traverse() override {
        current_index = nextIndexFrom(-1);
        if (current_index < 0) can_traverse = false;
        else can_traverse = true;
    }

    bool hasNext() override {
        return can_traverse;
    }

    Sample next() override {
        contributions_index = current_index;
        Sample sample = samples[current_index];
        current_index = nextIndexFrom(current_index);
        if (current_index < 0) can_traverse = false;
        else can_traverse = true;

        return sample;
    }

    std::vector<Color> contributions;
    Voronoi voronoi;
    Delaunay delaunay;
    std::vector<Sample> samples;
    std::vector<double> weights;

protected:
    int nextIndexFrom(const std::size_t start) const {
        size_t i = start + 1;
        int value_found = -1;

        bool search = i < samples.size();
        bool found = false;

        while (search && !found) {
            const Sample &sample = samples[i];
            if (sample.dx > -.5 && sample.dx < .5 && sample.dy > -.5 && sample.dy < .5) {
                found = true;
                value_found = i;
            }
            ++i;
            search = i < samples.size();
        }
        return value_found;
    }


    int current_index = 0;
    std::size_t contributions_index;
    bool can_traverse = false;
};


class FilteringVoronoiAggregator: public VoronoiAggregator {

    public:

    double margin;
    bool fake_mc;

    explicit FilteringVoronoiAggregator() : VoronoiAggregator(), margin(0.1), fake_mc(false) {}
    explicit FilteringVoronoiAggregator(const double margin, const bool fake_mc=false) : VoronoiAggregator(), margin(margin), fake_mc(fake_mc) {}

    Color aggregate() override {
        fake_mc = true;
        weights = std::vector<double>(samples.size());
        double total_weight = 0.;
        int idx = 0;

        for (auto vertex = delaunay.vertices_begin() ; vertex != delaunay.vertices_end() ; ++vertex) {
            const Point &site = vertex->point();
            if (site.x() < -.5 || site.x() >= .5 || site.y() < -.5 || site.y() >= .5) continue;

            Face_handle face = voronoi.dual(vertex);

            Polygon polygon;

            Ccb_halfedge_circulator halfEdge = face->ccb(), done(halfEdge);


            bool good = true;
            do {
                Point point = halfEdge->source()->point();

                good = is_good(point);

                polygon.push_back(point);
            } while (++halfEdge != done && good);

            if (good) {
                const double area = polygon.area();
                weights[idx] = area;
                total_weight += area;
            } else {
                weights[idx] = 0.;
            }

            if (fake_mc) {
                weights[idx] = 0.01;
            }

            ++idx;
        }

        Color color(0, 0, 0);

        // And finally, we weight the samples
        for (int i = 0 ; i < samples.size() ; i++) {
            const double weight = weights[i];
            color += weight * contributions[i];
        }

        return color / total_weight;
    }

    bool is_good(const Point &point) const {

        const double x = point.x();
        const double y = point.y();

        if (x > .5 + margin || x < -.5 - margin) return false;
        if (y > .5 + margin || y < -.5 - margin) return false;

        return true;
    }
};

class FilteringMCAggregator: public VoronoiAggregator {
    Color aggregate() override {
        Color color(0, 0, 0);
        size_t n_relevant_samples = 0;
        // And finally, we weight the samples
        for (int i = 0 ; i < samples.size() ; i++) {
            const auto sample = samples[i];
            if (sample.dx < -.5 || sample.dx >= .5 || sample.dy < -.5 || sample.dy >= .5) continue;
            color += contributions[i];
            ++n_relevant_samples;
        }

        return color / static_cast<double>(n_relevant_samples);
    }
};

class ClippedVoronoiAggregator: public VoronoiAggregator {
public:
    ClippedVoronoiAggregator() = default;

    void sampleFrom(std::shared_ptr<SamplerFactory> factory, double x, double y) override {
        // we collect samples
        auto pixelSampler = factory->create(x, y);
        pixelSampler->begin();
        std::size_t size = pixelSampler->sampleSize();
        samples = std::vector<Sample>(9 * size);
        contributions = std::vector<Color>(9 * size);
        int i = -1;

        while (pixelSampler->hasNext()) {
            Sample sample = pixelSampler->get();
            samples[++i] = sample;
            samples[++i] = {sample.x, sample.y, 1 - sample.dx, sample.dy};
            samples[++i] = {sample.x, sample.y, -1 - sample.dx, sample.dy};
            samples[++i] = {sample.x, sample.y, sample.dx, 1 - sample.dy};
            samples[++i] = {sample.x, sample.y, sample.dx, -1 - sample.dy};
            samples[++i] = {sample.x, sample.y, 1 - sample.dx, -1 - sample.dy};
            samples[++i] = {sample.x, sample.y, -1 - sample.dx, -1 - sample.dy};
            samples[++i] = {sample.x, sample.y, -1 - sample.dx, 1 - sample.dy};
            samples[++i] = {sample.x, sample.y, 1 - sample.dx, 1 - sample.dy};
        }

        // Voronoi point sites
        std::vector<Point> points(samples.size());

        // here we populate the Delaunay triangulation and the vertex -> index mapping
        for (i = 0 ; i < samples.size() ; i++) {
            Sample sample = samples[i];
            Point p(sample.dx, sample.dy);
            delaunay.insert(p);
        }

        voronoi = Voronoi(delaunay);

        current_index = 0;
    }
};

class InnerVoronoiAggregator: public VoronoiAggregator {
public:
    void sampleFrom(std::shared_ptr<SamplerFactory> factory, double x, double y) override {
        // we collect samples
        auto pixelSampler = factory->create(x, y);
        pixelSampler->begin();
        std::size_t size = pixelSampler->sampleSize();
        samples = std::vector<Sample>(9 * size);
        contributions = std::vector<Color>(9 * size);

        for (int i = 0; pixelSampler->hasNext(); i++) {
            samples[i] = pixelSampler->get();
        }

        // Voronoi point sites
        std::vector<Point> points(samples.size());

        for (auto sample : samples) {
            Point p(sample.dx, sample.dy);
            delaunay.insert(p);
        }

        voronoi = Voronoi(delaunay);

        current_index = 0;
    }

    Color aggregate() override {
        weights = std::vector<double>(samples.size());
        double total_weight = 0.;
        std::size_t idx = 0;
        for (auto vertex = delaunay.vertices_begin() ; vertex != delaunay.vertices_end() ; ++vertex) {
            Point site = vertex->point();
            if (site.x() < -.5 || site.x() >= .5 || site.y() < -.5 || site.y() >= .5) continue;

            Face_handle face = voronoi.dual(vertex);

            if (face->is_unbounded()) continue;

            Polygon polygon;

            Ccb_halfedge_circulator halfEdge = face->ccb();

            // Voronoi region area computation
            bool valid = true;
            do {
                Point p = halfEdge->source()->point();
                if (p.x() < -.5 || p.x() >= .5 || p.y() < -.5 || p.y() >= .5) valid = false;
                polygon.push_back(p);
            } while (valid && ++halfEdge != face->ccb());
            if (!valid) continue;
            double area = polygon.area();

            if (area < 0) area = -area;
            weights[idx] = area;
            total_weight += area;
            ++idx;
        }

        Color color(0, 0, 0);

        // And finally, we weight the samples
        for (int i = 0 ; i < samples.size() ; i++) {
            double weight = weights[i];
            color += weight * contributions[i] / total_weight;
        }
        return color;
    }
};

class AggregatorFactory {
public:
    virtual ~AggregatorFactory() = default;

    virtual std::shared_ptr<SampleAggregator> create() = 0;
};

class MCAggregatorFactory: public AggregatorFactory {
    std::shared_ptr<SampleAggregator> create() override {
        return std::make_shared<MCSampleAggregator>();
    }
};

class VoronoiAggregatorFactory: public AggregatorFactory {
    std::shared_ptr<SampleAggregator> create() override {
        return std::make_shared<VoronoiAggregator>();
    }
};

class ClippedVoronoiAggregatorFactory: public AggregatorFactory {
public:
    shared_ptr<SampleAggregator> create() override {
        return std::make_shared<ClippedVoronoiAggregator>();
    }
};

class InnerVoronoiAggregatorFactory: public AggregatorFactory {
public:
    shared_ptr<SampleAggregator> create() override {
        return std::make_shared<InnerVoronoiAggregator>();
    }
};

class MedianAggregatorFactory: public AggregatorFactory {
public:
    shared_ptr<SampleAggregator> create() override {
        return std::make_shared<MedianAggregator>();
    }
};

class MonAggregatorFactory: public AggregatorFactory {
public:
    explicit MonAggregatorFactory(size_t nb_blocks) : AggregatorFactory(), nb_blocks(nb_blocks) {}

    shared_ptr<SampleAggregator> create() override {
        return std::make_shared<MonAggregator>(nb_blocks);
    }

private:
    size_t nb_blocks;
};

class WinsorAggregatorFactory: public AggregatorFactory {
public:
    WinsorAggregatorFactory(double rejectRate, bool clipped) : AggregatorFactory(), rejectRate(rejectRate), clipped(clipped) {}
    shared_ptr<SampleAggregator> create() override {
        return std::make_shared<WinsorAggregator>(rejectRate, clipped);
    }

private:
    double rejectRate;
    bool clipped;
};

class FilteringMCAggregatorFactory: public AggregatorFactory {
public:
    FilteringMCAggregatorFactory(): AggregatorFactory() {}
    shared_ptr<SampleAggregator> create() override {
        return std::make_shared<FilteringMCAggregator>();
    }
};

class FilteringVoronoiAggregatorFactory: public AggregatorFactory {
public:
    FilteringVoronoiAggregatorFactory(): AggregatorFactory(), margin(.1) {}

    explicit FilteringVoronoiAggregatorFactory(const double m, const bool fake_mc = false): AggregatorFactory(), margin(m) {}

    shared_ptr<SampleAggregator> create() override {
        return std::make_shared<FilteringVoronoiAggregator>(margin);
    }

    private:
    double margin;
    bool fake_mc;
};

#endif //YAPT_AGGREGATORS_H
