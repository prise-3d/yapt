//
// Created by franck on 19/09/24.
//



#ifndef YAPT_SAMPLE_H
#define YAPT_SAMPLE_H

#include "yapt.h"
#include "Vec3.h"


struct Sample {
    double x;
    double y;
    Color color;
};

class SampleAggregator {
public:
    virtual Color aggregate() = 0;
    virtual SampleAggregator& operator<<(Sample sample) = 0;
};

class MCSampleAggregator : public SampleAggregator {
public:
    MCSampleAggregator(int size) : samples(std::vector<Sample>(size)) {}

    Color aggregate() override {
        Vec3 v(0, 0,0);

        for (auto & sample : samples) {
            v += sample.color;
        }

        return v / samples.size();
    }

    SampleAggregator& operator<<(const Sample sample) override {
        samples[index] = sample;
        ++index;
        return *this;
    }

private:
    std::vector<Sample> samples;
    int index = 0;
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
typedef AT::Site_2 Site;
typedef AT::Point_2 Point;
typedef Voronoi::Locate_result Locate_result;
typedef Voronoi::Face_handle Face_handle;
typedef Voronoi::Halfedge_handle Halfedge_handle;
typedef Voronoi::Ccb_halfedge_circulator Ccb_halfedge_circulator;
typedef CGAL::Polygon_2<K> Polygon;

struct VertexHandleHash {
    std::size_t operator()(const Delaunay::Vertex_handle& vh) const {
        return std::hash<void*>()(&(*vh));
    }
};

class VoronoiAggregator: public SampleAggregator {
public:
    VoronoiAggregator(int size): samples(std::vector<Sample>(size)) {}

    Color aggregate() override {

        // Voronoi regions weights
        std::vector<double> weights(samples.size(), 0.);

        // Voronoi point sites
        std::vector<Point> points(samples.size());

        // vertex -> index mapping -- CGAL does not preserve sites order. We need to establish
        // a correspondance by hand to be able to map sites to weights and thus sites to samples
        std::unordered_map<Delaunay::Vertex_handle, std::size_t, VertexHandleHash> vertexToIndex;

        // underlying Delaunay triangulation
        Delaunay delaunay;

        // here we populate the Delaunay triangulation and the vertex -> index mapping
        for (std::size_t i = 0 ; i < samples.size() ; i++) {
            Sample sample = samples[i];
            Delaunay::Vertex_handle vertexHandle = delaunay.insert(Point(sample.x, sample.y));
            vertexToIndex[vertexHandle] = i;
        }

        Voronoi voronoi(delaunay);

        // We traverse the Delaunay triangulation. Its vertices are the Voronoi site points
        for (auto vertex = delaunay.vertices_begin() ; vertex != delaunay.vertices_end() ; ++vertex) {
            Face_handle face = voronoi.dual(vertex);

            Polygon polygon;
            Ccb_halfedge_circulator halfEdge = face->ccb();

            // Voronoi region area computation
            bool isValid = true;
            do {
                if (!halfEdge->is_unbounded()) {
                    Point p = halfEdge->source()->point();
                    polygon.push_back(p);
                    // quick and dirty region pruning: if an edge is located outside the sampled pixel, flag this polygon as invalid
                    if (p.x() < -.5 || p.x() > .5 || p.y() < -.5 || p.y() > .5) isValid = false;
                } else isValid = false;
            } while (++halfEdge != face->ccb() && isValid);

            if (isValid) {
                std::size_t pointIndex = vertexToIndex[vertex];
                double area = polygon.area();
                weights[pointIndex] = area;
            }
        }

        Color color(0, 0, 0);

        // And finally, we weight the samples
        for (int i = 0 ; i < samples.size() ; i++) {
            Sample sample = samples[i];
            double weight = weights[i];
            color += weight * sample.color;
        }

        return color;
    }

    SampleAggregator& operator<<(const Sample sample) override {
        samples[index] = sample;
        ++index;
        return *this;
    }
private:
    std::vector<Sample> samples;
    int index = 0;
};

#endif //YAPT_SAMPLE_H
