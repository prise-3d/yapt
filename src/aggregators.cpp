//
// Created by franck on 19/09/24.
//

#include "aggregators.h"
#include <algorithm>
#include <cassert>

// ============================================================================
// MCSampleAggregator
// ============================================================================

MCSampleAggregator::MCSampleAggregator() {
    contributions_index = 0;
}

void MCSampleAggregator::sample_from(std::shared_ptr<SamplerFactory> factory, const double x, const double y) {
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

Color MCSampleAggregator::aggregate() {
    Vec3 v(0, 0,0);

    for (const auto & color : contributions) {
        v += color;
    }

    return v / static_cast<double>(samples.size());
}

void MCSampleAggregator::insert_contribution(Color color) {
    contributions[contributions_index] = color;
}

void MCSampleAggregator::traverse() {
    current_index = nextIndexFrom(-1);
    if (current_index < 0) can_traverse = false;
    else can_traverse = true;
}

bool MCSampleAggregator::has_next() {
    return can_traverse;
}

Sample MCSampleAggregator::next() {
    contributions_index = current_index;
    const Sample sample = samples[current_index];
    current_index = nextIndexFrom(current_index);
    if (current_index < 0) can_traverse = false;
    else can_traverse = true;

    return sample;
}

int MCSampleAggregator::nextIndexFrom(std::size_t start) {
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

// ============================================================================
// MedianAggregator
// ============================================================================

Color MedianAggregator::aggregate() {
    std::sort(contributions.begin(), contributions.end(), [](const Color & a, const Color & b) {
        return luminance(a) < luminance(b);
    });
    const int mid = contributions.size() / 2;
    return contributions[mid];
}

// ============================================================================
// MonAggregator
// ============================================================================

MonAggregator::MonAggregator(size_t nb_block) : MCSampleAggregator(), nb_block(nb_block) {}

Color MonAggregator::aggregate() {
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

// ============================================================================
// WinsorAggregator
// ============================================================================

WinsorAggregator::WinsorAggregator(double rejectRate, bool clipped)
    : MCSampleAggregator(), rejectRate(rejectRate), clipped(clipped) {}

Color WinsorAggregator::aggregate() {
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

// ============================================================================
// VoronoiAggregator
// ============================================================================

void VoronoiAggregator::sample_from(std::shared_ptr<SamplerFactory> factory, double x, double y) {
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

Color VoronoiAggregator::aggregate() {
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

void VoronoiAggregator::insert_contribution(Color color) {
    contributions[contributions_index] = color;
}

void VoronoiAggregator::traverse() {
    current_index = nextIndexFrom(-1);
    if (current_index < 0) can_traverse = false;
    else can_traverse = true;
}

bool VoronoiAggregator::has_next() {
    return can_traverse;
}

Sample VoronoiAggregator::next() {
    contributions_index = current_index;
    Sample sample = samples[current_index];
    current_index = nextIndexFrom(current_index);
    if (current_index < 0) can_traverse = false;
    else can_traverse = true;

    return sample;
}

int VoronoiAggregator::nextIndexFrom(const std::size_t start) const {
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

// ============================================================================
// FilteringVoronoiAggregator
// ============================================================================

FilteringVoronoiAggregator::FilteringVoronoiAggregator() : VoronoiAggregator(), margin(0.1) {}

FilteringVoronoiAggregator::FilteringVoronoiAggregator(const double margin)
    : VoronoiAggregator(), margin(margin) {}

Color FilteringVoronoiAggregator::aggregate() {
    weights = std::vector<double>(samples.size());
    double total_weight = 0.;
    int idx = 0;

    for (auto vertex = delaunay.vertices_begin() ; vertex != delaunay.vertices_end() ; ++vertex) {
        const Point &site = vertex->point();
        if (site.x() < -.5 || site.x() >= .5 || site.y() < -.5 || site.y() >= .5) continue;

        Face_handle face = voronoi.dual(vertex);

        Polygon polygon;

        Ccb_halfedge_circulator halfEdge = face->ccb(), done(halfEdge);

        bool good;
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

bool FilteringVoronoiAggregator::is_good(const Point &point) const {
    const double x = point.x();
    const double y = point.y();

    if (x > .5 + margin || x < -.5 - margin) return false;
    if (y > .5 + margin || y < -.5 - margin) return false;

    return true;
}

// ============================================================================
// FilteringMCAggregator
// ============================================================================

Color FilteringMCAggregator::aggregate() {
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

// ============================================================================
// ClippedVoronoiAggregator Implementation
// ============================================================================

void ClippedVoronoiAggregator::sample_from(std::shared_ptr<SamplerFactory> factory, double x, double y) {
    // we collect samples
    auto pixelSampler = factory->create(x, y);
    pixelSampler->begin();
    std::size_t size = pixelSampler->sampleSize();
    samples = std::vector<Sample>(9 * size);

    int i = -1;
    size_t j = size;

    while (pixelSampler->hasNext()) {
        Sample sample = pixelSampler->get();
        samples[++i] = sample;
        if (sample.dx < -.5 || sample.dx >= .5 || sample.dy < -.5 || sample.dy >= .5) continue;
        samples[++j] = {sample.x, sample.y, 1 - sample.dx, sample.dy};
        samples[++j] = {sample.x, sample.y, -1 - sample.dx, sample.dy};
        samples[++j] = {sample.x, sample.y, sample.dx, 1 - sample.dy};
        samples[++j] = {sample.x, sample.y, sample.dx, -1 - sample.dy};
        samples[++j] = {sample.x, sample.y, 1 - sample.dx, -1 - sample.dy};
        samples[++j] = {sample.x, sample.y, -1 - sample.dx, -1 - sample.dy};
        samples[++j] = {sample.x, sample.y, -1 - sample.dx, 1 - sample.dy};
        samples[++j] = {sample.x, sample.y, 1 - sample.dx, 1 - sample.dy};
    }

    samples.resize(j + 1);
    contributions = std::vector<Color>(j + 1);

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

// ============================================================================
// InnerVoronoiAggregator
// ============================================================================

void InnerVoronoiAggregator::sample_from(std::shared_ptr<SamplerFactory> factory, double x, double y) {
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

Color InnerVoronoiAggregator::aggregate() {
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

// ============================================================================
// NonZeroVoronoiAggregator
// ============================================================================

NonZeroVoronoiAggregator::NonZeroVoronoiAggregator(double margin)
    : FilteringVoronoiAggregator(margin) {}

void NonZeroVoronoiAggregator::sample_from(std::shared_ptr<SamplerFactory> factory, const double x, const double y) {
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

Color NonZeroVoronoiAggregator::aggregate() {
    size_t contributing_samples = 0;

    // number of samples inside the pixel
    size_t n_inner_samples = 0;

    // number of samples used to draw the Voronoi diagram
    size_t n_voronoi_samples;

    // non-zero contributions INSIDE the pixel
    std::vector<Color> non_zero_inner_contributions;
    // contributions used to compute the Voronoi tesselation
    std::vector<Sample> voronoi_samples;

    for (size_t i = 0 ; i < samples.size() ; ++i) {
        const auto contribution = contributions[i];
        const auto sample = samples[i];

        if (sample.dx >= -.5 && sample.dx < .5 && sample.dy >= -.5 && sample.dy < .5) {
            ++n_inner_samples;

            if (contribution.x() != 0 || contribution.y() != 0 || contribution.z() != 0) {
                ++contributing_samples;
                non_zero_inner_contributions.push_back(contribution);
                voronoi_samples.push_back(sample);
            }
        }
    }

    for (size_t i = 0 ; i < samples.size() ; ++i) {
        const auto sample = samples[i];

        if (!(sample.dx >= -.5 && sample.dx < .5 && sample.dy >= -.5 && sample.dy < .5)) {
            voronoi_samples.push_back(sample);
        }
    }

    for (const auto sample: voronoi_samples) {
        delaunay.insert(Point(sample.dx, sample.dy));
    }
    voronoi = Voronoi(delaunay);

    weights = std::vector<double>(voronoi_samples.size());
    double total_weight = 0.;
    int idx = 0;

    for (auto vertex = delaunay.vertices_begin() ; vertex != delaunay.vertices_end() ; ++vertex) {
        const Point &site = vertex->point();
        if (site.x() < -.5 || site.x() >= .5 || site.y() < -.5 || site.y() >= .5) {
            ++idx;
            continue;
        }

        Face_handle face = voronoi.dual(vertex);

        Polygon polygon;

        Ccb_halfedge_circulator halfEdge = face->ccb(), done(halfEdge);

        bool good;
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
        ++idx;
    }

    if (total_weight == 0) return {0, 0, 0};

    Color color(0, 0, 0);

    // And finally, we weight the samples
    for (int i = 0 ; i < non_zero_inner_contributions.size() ; i++) {
        const double weight = weights[i];
        color += weight * non_zero_inner_contributions[i];
    }

    Color contribution = color / total_weight;

    contribution *= static_cast<double>(non_zero_inner_contributions.size()) / static_cast<double>(n_inner_samples);

    samples = voronoi_samples;
    contributions = non_zero_inner_contributions;

    return contribution;
}

// ============================================================================
// AggregatorFactories
// ============================================================================

std::shared_ptr<SampleAggregator> MCAggregatorFactory::create() {
    return std::make_shared<MCSampleAggregator>();
}

std::shared_ptr<SampleAggregator> VoronoiAggregatorFactory::create() {
    return std::make_shared<VoronoiAggregator>();
}

shared_ptr<SampleAggregator> ClippedVoronoiAggregatorFactory::create() {
    return std::make_shared<ClippedVoronoiAggregator>();
}

shared_ptr<SampleAggregator> InnerVoronoiAggregatorFactory::create() {
    return std::make_shared<InnerVoronoiAggregator>();
}

shared_ptr<SampleAggregator> MedianAggregatorFactory::create() {
    return std::make_shared<MedianAggregator>();
}

MonAggregatorFactory::MonAggregatorFactory(size_t nb_blocks)
    : AggregatorFactory(), nb_blocks(nb_blocks) {}

shared_ptr<SampleAggregator> MonAggregatorFactory::create() {
    return std::make_shared<MonAggregator>(nb_blocks);
}

WinsorAggregatorFactory::WinsorAggregatorFactory(double rejectRate, bool clipped)
    : AggregatorFactory(), rejectRate(rejectRate), clipped(clipped) {}

shared_ptr<SampleAggregator> WinsorAggregatorFactory::create() {
    return std::make_shared<WinsorAggregator>(rejectRate, clipped);
}

FilteringMCAggregatorFactory::FilteringMCAggregatorFactory(): AggregatorFactory() {}

shared_ptr<SampleAggregator> FilteringMCAggregatorFactory::create() {
    return std::make_shared<FilteringMCAggregator>();
}

FilteringVoronoiAggregatorFactory::FilteringVoronoiAggregatorFactory(): AggregatorFactory(), margin(.1) {}

FilteringVoronoiAggregatorFactory::FilteringVoronoiAggregatorFactory(const double m)
    : AggregatorFactory(), margin(m) {}

shared_ptr<SampleAggregator> FilteringVoronoiAggregatorFactory::create() {
    return std::make_shared<FilteringVoronoiAggregator>(margin);
}

NonZeroVoronoiAggregatorFactory::NonZeroVoronoiAggregatorFactory(): AggregatorFactory(), margin(.1) {}

NonZeroVoronoiAggregatorFactory::NonZeroVoronoiAggregatorFactory(const double m)
    : AggregatorFactory(), margin(m) {}

shared_ptr<SampleAggregator> NonZeroVoronoiAggregatorFactory::create() {
    return std::make_shared<NonZeroVoronoiAggregator>(margin);
}
