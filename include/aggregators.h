//
// Created by franck on 19/09/24.
//



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

#endif //YAPT_AGGREGATORS_H
