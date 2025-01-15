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
    PixelSampler(double x, double y): x(x), y(y) {}
    virtual ~PixelSampler() = default;
    virtual void begin() = 0;
    virtual bool hasNext() = 0;
    virtual Sample get() = 0;
    virtual std::size_t sampleSize() = 0;

public:
    double x;
    double y;
};

class TrivialSampler: public PixelSampler {
public:
    TrivialSampler(const double x, const double y, const int size): PixelSampler(x, y), size(size), _dx(0), _dy(0) {}

    void begin() override {
        index = 0;
    }

    bool hasNext() override {
        return index < size;
    }

    Sample get() override {
        index++;
        _dx = randomDouble() - .5;
        _dy = randomDouble() - .5;
        return {
            x + _dx,
            y + _dy,
            _dx,
            _dy
        };
    }

    std::size_t sampleSize() override {
        return this->size;
    }

protected:
    int size = 10;
    int index = 0;
    double _dx;
    double _dy;
};

class UniformSampler: public PixelSampler {
public:
    UniformSampler(double x, double y, const size_t sqrtSpp) :
        PixelSampler(x,y),
        _dx(0),
        _dy(0),
        index(0),
        sqrtSpp(sqrtSpp),
        x_samples(sqrtSpp * sqrtSpp),
        y_samples(sqrtSpp * sqrtSpp) {

        total_samples_amount = sqrtSpp * sqrtSpp;

        const double n_steps = (static_cast<double>(sqrtSpp));
        const double margin = 1./ (2 * static_cast<double>(sqrtSpp));
        // inner part of the [0 ; 1] x [0 ; 1 ] square to sample
        for (auto row = 0; row < sqrtSpp; ++row) {
            const auto y_value = margin + static_cast<double>(row) / n_steps - .5;
            for (auto column = 0; column < sqrtSpp; ++column) {
                x_samples[row * sqrtSpp + column] = margin + static_cast<double>(column) / n_steps - .5;
                y_samples[row * sqrtSpp + column] = y_value;
            }
        }
    }

    void begin() override {
        index = 0;
    }

    size_t sampleSize() override {
        return total_samples_amount;
    }

    Sample get() override {
        _dx = x_samples[index];
        _dy = y_samples[index];
        Sample sample{
            x + _dx,
            y + _dy,
            _dx,
            _dy
        };
        ++index;
        return sample;
    };

    bool hasNext() override { return index < total_samples_amount; }

private:
    size_t sqrtSpp;
    std::size_t total_samples_amount = 0;
    std::size_t index;
    double _dx;
    double _dy;
    std::vector<double> x_samples;
    std::vector<double> y_samples;
};

class StratifiedSampler: public PixelSampler {
public:
    StratifiedSampler(double x, double y, const size_t sqrtSpp) :
        PixelSampler(x,y),
        sqrtSpp(sqrtSpp),
        index(0),
        _dx(0),
        _dy(0),
        x_samples(sqrtSpp * sqrtSpp),
        y_samples(sqrtSpp * sqrtSpp) {

        total_samples_amount = sqrtSpp * sqrtSpp;

        const double n_steps = (static_cast<double>(sqrtSpp));
        const double margin = 1./ (2 * static_cast<double>(sqrtSpp));
        // inner part of the [0 ; 1] x [0 ; 1 ] square to sample
        for (auto row = 0; row < sqrtSpp; ++row) {
            const auto y_value = margin + static_cast<double>(row) / n_steps - .5;
            for (auto column = 0; column < sqrtSpp; ++column) {
                x_samples[row * sqrtSpp + column] = margin + static_cast<double>(column) / n_steps - .5 + randomDouble(-margin, margin);
                y_samples[row * sqrtSpp + column] = y_value + randomDouble(-margin, margin);
            }
        }
    }

    void begin() override {
        index = 0;
    }

    size_t sampleSize() override {
        return total_samples_amount;
    }

    Sample get() override {
        _dx = x_samples[index];
        _dy = y_samples[index];
        Sample sample{
            x + _dx,
            y + _dy,
            _dx,
            _dy
        };
        ++index;
        return sample;
    };

    bool hasNext() override { return index < total_samples_amount; }

private:
    size_t sqrtSpp;
    std::size_t total_samples_amount = 0;
    std::size_t index;
    double _dx;
    double _dy;
    std::vector<double> x_samples;
    std::vector<double> y_samples;
};

class ClippedStratifiedSampler : public PixelSampler {
public:
    ClippedStratifiedSampler(double x, double y, const size_t sqrtSpp) :
        PixelSampler(x,y),
        sqrtSpp(sqrtSpp),
        index(0),
        _dx(0),
        _dy(0),
        x_samples(sqrtSpp * (sqrtSpp + 4)),
        y_samples(sqrtSpp * (sqrtSpp + 4)) {

        total_samples_amount = sqrtSpp * (sqrtSpp + 4);

        const double n_steps = (static_cast<double>(sqrtSpp));
        const double margin = 1./ (2 * static_cast<double>(sqrtSpp));
        // inner part of the [0 ; 1] x [0 ; 1 ] square to sample
        for (auto row = 0; row < sqrtSpp; ++row) {
            const auto y_value = margin + static_cast<double>(row) / n_steps - .5;
            for (auto column = 0; column < sqrtSpp; ++column) {
                x_samples[row * sqrtSpp + column] = margin + static_cast<double>(column) / n_steps - .5 + randomDouble(-margin, margin);
                y_samples[row * sqrtSpp + column] = y_value + randomDouble(-margin, margin);
            }
        }

        const size_t pos = sqrtSpp * sqrtSpp;
        for (auto t = 0 ; t < sqrtSpp; ++t) {
            x_samples[pos + t] = x_samples[t];
            y_samples[pos + t] = -1 - y_samples[t]; // close to y = -.5

            x_samples[pos + sqrtSpp + t] = x_samples[pos - sqrtSpp + t];
            y_samples[pos + sqrtSpp + t] = 1 - y_samples[pos - sqrtSpp + t]; // close to y = .5;

            x_samples[pos + 2 * sqrtSpp + t] = -1 - x_samples[t * sqrtSpp]; // close to x = -.5
            y_samples[pos + 2 * sqrtSpp + t] = y_samples[t * sqrtSpp];

            x_samples[pos + 3 * sqrtSpp + t] = 1 - x_samples[t * sqrtSpp + sqrtSpp - 1]; // close to x = .5
            y_samples[pos + 3 * sqrtSpp + t] = y_samples[t * sqrtSpp + sqrtSpp - 1];
        }
    }

    void begin() override {
        index = 0;
    }

    size_t sampleSize() override {
        return total_samples_amount;
    }

    Sample get() override {
        _dx = x_samples[index];
        _dy = y_samples[index];
        Sample sample{
            x + _dx,
            y + _dy,
            _dx,
            _dy
        };
        ++index;
        return sample;
    };

    bool hasNext() override { return index < total_samples_amount; }

private:
    size_t sqrtSpp;
    std::size_t total_samples_amount = 0;
    std::size_t index;
    double _dx;
    double _dy;
    std::vector<double> x_samples;
    std::vector<double> y_samples;
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

class ClippedStratifiedSamplerFactory : public SamplerFactory {
public:
    explicit ClippedStratifiedSamplerFactory(const int sqrtSpp): sqrtSpp(sqrtSpp) {}

    shared_ptr<PixelSampler> create(double x, double y) override {
        return make_shared<ClippedStratifiedSampler>(x, y, sqrtSpp);
    }

protected:
    int sqrtSpp;
};

class PPPSampler : public PixelSampler {
public:
    PPPSampler(double x, double y, double intensity, double confidence) : PixelSampler(x, y), intensity(intensity), _dx(0), _dy(0), index(0) {
        epsilon_margin = sqrt(log(intensity * log(intensity)) - log(log(1./confidence))) / sqrt(pi * intensity);
        min_value = -.5 - epsilon_margin;
        max_value = .5 + epsilon_margin;
    }

    void begin() override {
        index = 0;
        total_samples_amount = Poisson(intensity).next();
    }

    bool hasNext() override { return index < total_samples_amount; }
    Sample get() override {
        _dx = randomDouble(min_value, max_value);
        _dy = randomDouble(min_value, max_value);
        Sample sample{
                x + _dx,
                y + _dy,
                _dx,
                _dy
        };
        ++index;
        return sample;
    };
    size_t sampleSize() override { return total_samples_amount; }

private:
    double intensity;
    std::size_t total_samples_amount = 0;

    std::size_t index;
    double epsilon_margin;
    double _dx;
    double _dy;
    double min_value;
    double max_value;
};

class CPPPSampler: public PixelSampler {
public:
    CPPPSampler(double x, double y, const size_t intensity) :
        PixelSampler(x,y),
        _dx(0),
        _dy(0),
        index(0),
        intensity(intensity),
        x_samples(intensity * (intensity + 4)),
        y_samples(intensity * (intensity + 4)) {

        total_samples_amount = intensity * (intensity + 4);

        const double n_steps = (static_cast<double>(intensity));

        const double margin = 1./ (2 * static_cast<double>(intensity));
        // inner part of the [0 ; 1] x [0 ; 1 ] square to sample
        for (auto row = 0; row < intensity; ++row) {
            const auto y_value = margin + static_cast<double>(row) / n_steps - .5;
            for (auto column = 0; column < intensity; ++column) {
                x_samples[row * intensity + column] = margin + static_cast<double>(column) / n_steps - .5;
                y_samples[row * intensity + column] = y_value;
            }
        }

        size_t pos = intensity * intensity;
        for (auto t = 0 ; t < intensity; ++t) {
            const auto dt = static_cast<double>(t);
            // upper and lower line

            x_samples[pos    ] =  margin + dt / n_steps - .5;
            x_samples[pos + 1] =  margin + dt / n_steps - .5;
            y_samples[pos    ] = -.5 - margin;
            y_samples[pos + 1] =  .5 + margin;

            // left / right columns
            x_samples[pos + 2] = -margin -.5;
            x_samples[pos + 3] = margin +  .5;
            y_samples[pos + 2] = margin + dt / n_steps - .5;
            y_samples[pos + 3] = margin + dt / n_steps - .5;
            pos += 4;
        }
    }

    void begin() override {
        index = 0;
    }

    size_t sampleSize() override {
        return total_samples_amount;
    }

    Sample get() override {
        _dx = x_samples[index];
        _dy = y_samples[index];
        Sample sample{
            x + _dx,
            y + _dy,
            _dx,
            _dy
        };
        ++index;
        return sample;
    };

    bool hasNext() override { return index < total_samples_amount; }

private:
    size_t intensity;
    std::size_t total_samples_amount = 0;
    std::size_t index;
    double _dx;
    double _dy;
    std::vector<double> x_samples;
    std::vector<double> y_samples;
};

class PPPSamplerFactory: public SamplerFactory {
public:
    PPPSamplerFactory(std::size_t intensity, double confidence): intensity(intensity), confidence(confidence) {}

    shared_ptr<PixelSampler> create(double x, double y) override {
        return make_shared<PPPSampler>(x, y, intensity, confidence);
    }

protected:
    std::size_t intensity;
    double confidence;
};

class CPPPSamplerFactory: public SamplerFactory {
public:
    explicit CPPPSamplerFactory(const std::size_t intensity): intensity(intensity) {}

    shared_ptr<PixelSampler> create(double x, double y) override {
        return make_shared<CPPPSampler>(x, y, intensity);
    }

    protected:
    std::size_t intensity;
};

class UniformSamplerFactory: public SamplerFactory {
public:
    explicit UniformSamplerFactory(const std::size_t intensity): intensity(intensity) {}

    shared_ptr<PixelSampler> create(double x, double y) override {
        return make_shared<UniformSampler>(x, y, intensity);
    }

protected:
    std::size_t intensity;
};

class SkewedPPPSampler : public PixelSampler {
public:
    SkewedPPPSampler(double x, double y, std::size_t number_of_samples, double intensity, double confidence) :
        PixelSampler(x, y),
        number_of_samples(number_of_samples),
        intensity(intensity) {
        epsilon_margin = sqrt(log(intensity * log(intensity)) - log(log(1./confidence))) / sqrt(pi * intensity);
        _dx = 0;
        _dy = 0;
        index = 0;
    }

    void begin() override {
        index = 0;
    }

    bool hasNext() override { return index < intensity; }

    Sample get() override {
        if (index < number_of_samples) {
            _dx = randomDouble() - .5;
            _dy = randomDouble() - .5;
        } else {
            if (randomDouble() < .5) {
                _dx = randomDouble(-.5 - epsilon_margin, .5 + epsilon_margin);
                _dy = randomDouble(.5, .5 + epsilon_margin);
            } else {
                _dx = randomDouble(.5, .5 + epsilon_margin);
                _dy = randomDouble(-.5 - epsilon_margin, .5 + epsilon_margin);
            }

            if (randomDouble() < .5) _dx = -_dx;
            if (randomDouble() < .5) _dy = -_dy;
        }
        ++index;
        return {
            x + _dx,
            y + _dy,
            _dx,
            _dy
        };
    }

    size_t sampleSize() override { return intensity; }

    double epsilon_margin;

private:
    std::size_t intensity;
    std::size_t number_of_samples;
    std::size_t index;


    double _dx;
    double _dy;
};

class SkewedPPPSamplerFactory: public SamplerFactory {
public:
    SkewedPPPSamplerFactory(std::size_t number_of_samples, double confidence): confidence(confidence), number_of_samples(number_of_samples) {
        // here we try to estimate the gamma parameter of a PPP thrown into an eps-dilated square which would
        // have an expected number of generated points inside the unit square of exactly "intensity"

        const auto N = static_cast<double>(number_of_samples);
        double n = N;

        for (int i = 0 ; i < 10 ; i++) {
            const double margin = sqrt(log(n * log(n)) - log(log(1./confidence))) / sqrt(pi * n);
            n = N * (1 + 2 * margin) * (1 + 2 * margin);
        }

        skewed_intensity = static_cast<size_t>(n + 1);
    }

    shared_ptr<PixelSampler> create(double x, double y) override {
        return make_shared<SkewedPPPSampler>(x, y, number_of_samples, skewed_intensity, confidence);
    }

    std::size_t skewed_intensity;

protected:

    std::size_t number_of_samples;
    double confidence;
};

#endif //YAPT_SAMPLER_H
