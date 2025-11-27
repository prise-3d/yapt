//
// Created by franck on 18/06/24.
//

#ifndef YAPT_SAMPLER_H
#define YAPT_SAMPLER_H

#include <boost/math/tools/precision.hpp>

#include "yapt.h"

struct Sample {
    double x;
    double y;
    double dx;
    double dy;
};

inline std::ostream &operator<<(std::ostream &out, const Sample &sample) {
    return out <<'(' << sample.x << ' ' << sample.y << ' ' << sample.dx << ' ' << sample.dy << ')';
}

class PixelSampler {
public:
    virtual ~PixelSampler() = default;

    PixelSampler(double x, double y): x(x), y(y) {}
    PixelSampler(const PixelSampler&) = delete;
    PixelSampler& operator=(const PixelSampler&) = delete;

    double x;
    double y;

    void get_samples(std::vector<Sample>& out) const {
        out.clear();
        out.reserve(total_sample_count());
        generate_samples(out);
    }

    virtual std::size_t total_sample_count() const = 0;
    virtual std::size_t usable_sample_count() const = 0;

protected:
    virtual void generate_samples(std::vector<Sample>& out) const = 0;
};

class TrivialSampler: public PixelSampler {
public:
    TrivialSampler(const double x, const double y, const int size): PixelSampler(x, y), size(size) {}

    std::size_t size;

    [[nodiscard]] std::size_t total_sample_count() const override {
        return size;
    }

    [[nodiscard]] std::size_t usable_sample_count() const override {
        return size;
    }

protected:
    void generate_samples(std::vector<Sample> &out) const override {
        for (std::size_t i = 0 ; i < size ; ++i) {
            const double _dx = random_double() - .5;
            const double _dy = random_double() - .5;

            out.push_back(Sample{
                x + _dx,
                y + _dy,
                _dx,
                _dy
            });
        }
    }
};

class StratifiedSampler: public PixelSampler {
public:

    StratifiedSampler(double x, double y, const size_t sqrtSpp): PixelSampler(x,y), sqrtSpp(sqrtSpp) {}

    [[nodiscard]] std::size_t total_sample_count() const override {
        return sqrtSpp * sqrtSpp;
    }

    [[nodiscard]] std::size_t usable_sample_count() const override {
        return sqrtSpp * sqrtSpp;
    }

    void generate_samples(std::vector<Sample> &out) const override {
        double step = 1.0 / sqrtSpp;
        double start_offset = -0.5;

        for (size_t i = 0 ; i < sqrtSpp ; ++i) {
            for (size_t j = 0 ; j < sqrtSpp ; ++j) {
                double cell_x = start_offset + i * step;
                double cell_y = start_offset + j * step;

                double jitter_x = step * random_double();
                double jitter_y = step * random_double();

                double dx = cell_x + jitter_x;
                double dy = cell_y + jitter_y;

                out.push_back(Sample{
                    x + jitter_x,
                    y + jitter_y,
                    dx,
                    dy
                });
            }
        }
    }

private:
    std::size_t sqrtSpp;
};

class SamplerFactory {
public:
    virtual ~SamplerFactory() = default;
    virtual shared_ptr<PixelSampler> create(double x, double y) = 0;
};

class TrivialSamplerFactory: public SamplerFactory {
public:
    explicit TrivialSamplerFactory(int samples): samples(samples) {}
    shared_ptr<PixelSampler> create(double x, double y) override {
        return make_shared<TrivialSampler>(x, y, samples);
    }

protected:
    int samples;
};

class StratifiedSamplerFactory : public SamplerFactory {
public:
    explicit StratifiedSamplerFactory(const int sqrtSpp): sqrtSpp(sqrtSpp) {}

    shared_ptr<PixelSampler> create(double x, double y) override {
        return make_shared<StratifiedSampler>(x, y, sqrtSpp);
    }
protected:
    int sqrtSpp;
};

class SkewedPPPSampler : public PixelSampler {
public:
    SkewedPPPSampler(double x, double y, std::size_t number_of_samples, double intensity, double margin) :
        PixelSampler(x, y),
        _usable_sample_count(number_of_samples),
        _total_sample_count(intensity),
        margin(margin)
    {}

    [[nodiscard]] std::size_t total_sample_count() const override {
        return _total_sample_count;
    }

    [[nodiscard]] std::size_t usable_sample_count() const override {
        return _usable_sample_count;
    }

    void generate_samples(std::vector<Sample> &out) const override {
        // inner samples
        for (size_t i = 0 ; i < _usable_sample_count ; ++i) {
            const double _dx = random_double() - .5;
            const double _dy = random_double() - .5;
            out.push_back(Sample{
                x + _dx,
                y + _dy,
                _dx,
                _dy
            });
        }

        // outer samples
        for (size_t i = _usable_sample_count ; i < total_sample_count() ; ++i) {
            double _dx;
            double _dy;
            if (random_double() < .5) {
                _dx = random_double(-.5 - margin, .5 + margin);
                _dy = random_double(.5, .5 + margin);
            } else {
                _dx = random_double(.5, .5 + margin);
                _dy = random_double(-.5 - margin, .5 + margin);
            }

            if (random_double() < .5) _dx = -_dx;
            if (random_double() < .5) _dy = -_dy;
            out.push_back(Sample{
                x + _dx,
                y + _dy,
                _dx,
                _dy
            });
        }
    }

    double margin;

private:
    std::size_t _total_sample_count;
    std::size_t _usable_sample_count;
};

class SkewedPPPSamplerFactory: public SamplerFactory {
public:
    SkewedPPPSamplerFactory(std::size_t number_of_samples, double confidence): confidence(confidence), number_of_samples(number_of_samples) {
        // here we try to estimate the gamma parameter of a PPP thrown into an eps-dilated square which would
        // have an expected number of generated points inside the unit square of exactly "intensity"
        const auto N = static_cast<double>(number_of_samples);
        double n = N;
        const double logz = log(4000 * N);
        margin = sqrt((logz - log(logz)) / (M_PI * n));

        n = N * (1 + 2 * margin) * (1 + 2 * margin);
        skewed_intensity = static_cast<size_t>(n + 1);
    }

    shared_ptr<PixelSampler> create(double x, double y) override {
        return make_shared<SkewedPPPSampler>(x, y, number_of_samples, skewed_intensity, margin);
    }

    std::size_t skewed_intensity;

protected:

    std::size_t number_of_samples;
    double margin;
    double confidence;
};

#endif //YAPT_SAMPLER_H
