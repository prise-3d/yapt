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
    virtual void insert_contribution(Color color) = 0;
    virtual void traverse() = 0;
    virtual bool has_next() = 0;
    virtual Sample next() = 0;
};

class MCSampleAggregator : public SampleAggregator {
public:
    MCSampleAggregator();

    void sample_from(std::shared_ptr<SamplerFactory> factory, double x, double y) override;
    Color aggregate() override;
    void insert_contribution(Color color) override;
    void traverse() override;
    bool has_next() override;
    Sample next() override;

private:
    int nextIndexFrom(std::size_t start);

protected:
    std::vector<Sample> samples;
    std::vector<Color> contributions;
    std::size_t contributions_index;
    int current_index;
    bool can_traverse;
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
    void insert_contribution(Color color) override;
    void traverse() override;
    bool has_next() override;
    Sample next() override;

    std::vector<Color> contributions;
    Voronoi voronoi;
    Delaunay delaunay;
    std::vector<Sample> samples;
    std::vector<double> weights;

protected:
    int nextIndexFrom(std::size_t start) const;
    double compute_voronoi_cell_area(Face_handle face) const;

    int current_index = 0;
    std::size_t contributions_index;
    bool can_traverse = false;
};


class FilteringVoronoiAggregator: public VoronoiAggregator {
public:
    double margin;

    explicit FilteringVoronoiAggregator();
    explicit FilteringVoronoiAggregator(double margin);

    Color aggregate() override;
    bool is_good(const Point &point) const;
};

class FilteringMCAggregator: public VoronoiAggregator {
public:
    Color aggregate() override;
};

class ClippedVoronoiAggregator: public VoronoiAggregator {
public:
    ClippedVoronoiAggregator() = default;
    void sample_from(std::shared_ptr<SamplerFactory> factory, double x, double y) override;
};

class InnerVoronoiAggregator: public VoronoiAggregator {
public:
    void sample_from(std::shared_ptr<SamplerFactory> factory, double x, double y) override;
    Color aggregate() override;
};


/**
 * Creates a Voronoi Diagram using only non zero contributions
 */
class NonZeroVoronoiAggregator: public FilteringVoronoiAggregator {
public:
    explicit NonZeroVoronoiAggregator(double margin);

    void sample_from(std::shared_ptr<SamplerFactory> factory, double x, double y) override;
    Color aggregate() override;
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

class InnerVoronoiAggregatorFactory: public AggregatorFactory {
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

class FilteringMCAggregatorFactory: public AggregatorFactory {
public:
    FilteringMCAggregatorFactory();
    shared_ptr<SampleAggregator> create() override;
};

class FilteringVoronoiAggregatorFactory: public AggregatorFactory {
public:
    FilteringVoronoiAggregatorFactory();
    explicit FilteringVoronoiAggregatorFactory(double m);

    shared_ptr<SampleAggregator> create() override;

private:
    double margin;
};

class NonZeroVoronoiAggregatorFactory: public AggregatorFactory {
public:
    NonZeroVoronoiAggregatorFactory();
    explicit NonZeroVoronoiAggregatorFactory(double m);

    shared_ptr<SampleAggregator> create() override;

private:
    double margin;
};

#endif //YAPT_AGGREGATORS_H
