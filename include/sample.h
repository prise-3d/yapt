//
// Created by franck on 19/09/24.
//



#ifndef YAPT_SAMPLE_H
#define YAPT_SAMPLE_H

#include "yapt.h"
#include "Vec3.h"

class SampleAggregator {
public:
    virtual ~SampleAggregator() = default;
    virtual Color aggregate() = 0;
    virtual void sampleFrom(std::shared_ptr<SamplerFactory>, double x, double y) = 0;
    virtual void insertContribution(Color color) = 0;
    virtual void traverse() = 0;
    virtual bool hasNext() = 0;
    virtual Sample next() = 0;
    virtual void debug() = 0;
};

class MCSampleAggregator : public SampleAggregator {
public:
    MCSampleAggregator() {
        contributions_index = 0;
    }

    void sampleFrom(std::shared_ptr<SamplerFactory> factory, double x, double y) override {
        auto pixelSampler = factory->create(x, y);
        std::size_t size = pixelSampler->sampleSize();
        samples = std::vector<Sample>(size);
        contributions = std::vector<Color>(size);

        std::size_t i = 0;
        for (pixelSampler->begin() ; pixelSampler->hasNext() ; i++) {
            samples[i] = pixelSampler->get();
        }

        contributions_index = 0;
    }


    Color aggregate() override {
        Vec3 v(0, 0,0);

        for (auto & color : contributions) {
            v += color;
        }

        return v / (double)samples.size();
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

    void debug() override {

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
                value_found = i;
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

struct VertexHandleHash {
    std::size_t operator()(const Delaunay::Vertex_handle& vh) const {
        return std::hash<void*>()(&(*vh));
    }
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
            // we collect samples
            auto pixelSampler = factory->create(x, y);
            pixelSampler->begin();
            std::size_t size = pixelSampler->sampleSize();
            samples = std::vector<Sample>(size);
            contributions = std::vector<Color>(size);
            std::size_t i = 0;
            for (; pixelSampler->hasNext(); i++) {
                samples[i] = pixelSampler->get();
            }

            // we now need to construct the voronoi diagram to check if it satisfies our
            // robustness criterion

            // Voronoi point sites
            std::vector<Point> points(samples.size());

            // vertex -> index mapping -- CGAL does not preserve sites order. We need to establish
            // a correspondence by hand to be able to map sites to weights and thus sites to samples
            vertexToIndex.clear();

            // here we populate the Delaunay triangulation and the vertex -> index mapping
            for (i = 0 ; i < samples.size() ; i++) {
                Sample sample = samples[i];
                Delaunay::Vertex_handle vertexHandle = delaunay.insert(Point(sample.dx, sample.dy));
                vertexToIndex[vertexHandle] = i;
            }

            voronoi = Voronoi(delaunay);

            for (auto vertex = delaunay.vertices_begin() ; vertex != delaunay.vertices_end() && !isInvalid ; ++vertex) {
                Point site = vertex->point();
                if (site.x() < -.5 || site.x() > .5 || site.y() < -.5 || site.y() > .5) continue;
                Face_handle face = voronoi.dual(vertex);

                if (face->is_unbounded()) {
                    isInvalid = true;
                }
            }
        } while (isInvalid);
        current_index = 0;
    }

    Color aggregate() override {
        weights = std::vector<double>(samples.size());
        for (auto vertex = delaunay.vertices_begin() ; vertex != delaunay.vertices_end() ; ++vertex) {
            Point site = vertex->point();
            if (site.x() < -.5 || site.x() > .5 || site.y() < -.5 || site.y() > .5) continue;

            Face_handle face = voronoi.dual(vertex);

            Polygon polygon;

            Ccb_halfedge_circulator halfEdge = face->ccb();

            // Voronoi region area computation
            do {
                Point p = halfEdge->source()->point();
                polygon.push_back(p);
            } while (++halfEdge != face->ccb());

            std::size_t pointIndex = vertexToIndex[vertex];
            double area = polygon.area();
            if (area < 0) area = -area;
            weights[pointIndex] = area;
        }

        Color color(0, 0, 0);

        // And finally, we weight the samples
        for (int i = 0 ; i < samples.size() ; i++) {
            double weight = weights[i];
            color += weight * contributions[i];
        }
        return color;
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

    void debug() override {
        for (int i = 0 ; i < samples.size() ; i++) {
            std::cout << i << "  =>  " << samples[i] << "  -  " << weights[i] << std::endl;
        }
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
                value_found = i;
            }
            ++i;
            search = i < samples.size();
        }
        return value_found;
    }

    std::vector<Sample> samples;
    std::vector<double> weights;
    std::vector<Color> contributions;
    std::unordered_map<Delaunay::Vertex_handle, std::size_t, VertexHandleHash> vertexToIndex;
    Voronoi voronoi;
    Delaunay delaunay;

    int current_index = 0;
    std::size_t contributions_index;
    bool can_traverse = false;
};

class AggregatorFactory {
public:
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

#endif //YAPT_SAMPLE_H
