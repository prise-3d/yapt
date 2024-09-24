//
// Created by franck on 18/06/24.
//

#ifndef YAPT_SAMPLER_H
#define YAPT_SAMPLER_H

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

protected:
    double x;
    double y;
};

class TrivialSampler: public PixelSampler {
public:
    TrivialSampler(double x, double y, int size): PixelSampler(x, y), size(size), _dx(0), _dy(0) {}

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


class StratifiedSampler: public PixelSampler {
public:
    StratifiedSampler(double x, double y, int sqrtSpp): PixelSampler(x, y), sqrtSpp(sqrtSpp), _dx(0), _dy(0) { step = 1. / (sqrtSpp + 1);}

    void begin() override {
        internal_dx = 0;
        internal_dy = 0;
        step = 1. / (sqrtSpp);
    }

    bool hasNext() override {
        return internal_dy < sqrtSpp;
    }

    Sample get() override {
        _dx = -.5 + step * (internal_dx + randomDouble());
        _dy = -.5 + step * (internal_dy + randomDouble());
        Sample p{
                x + _dx,
                y + _dy,
                _dx,
                _dy};
        internal_dx++;
        if (internal_dx >= sqrtSpp) {
            internal_dx = 0;
            internal_dy++;
        }

        return p;
    }

    std::size_t sampleSize() override {
        return sqrtSpp * sqrtSpp;
    }

protected:
    int sqrtSpp = 10;
    double step;
    int internal_dx = 0;
    int internal_dy = 0;
    double _dx;
    double _dy;
};

class SamplerFactory {
public:
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
    explicit StratifiedSamplerFactory(int sqrtSpp): sqrtSpp(sqrtSpp) {}

    shared_ptr<PixelSampler> create(double x, double y) override {
        return make_shared<StratifiedSampler>(x, y, sqrtSpp);
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




/**
 * TODO: FIXME
  */
class SkewedPPPSampler : public PixelSampler {
public:
    SkewedPPPSampler(double x, double y, std::size_t number_of_samples, double intensity, double confidence) :
        PixelSampler(x, y),
        number_of_samples(number_of_samples),
        intensity(intensity) {
        epsilon_margin = sqrt(log(intensity * log(intensity)) - log(log(1./confidence))) / sqrt(pi * intensity);
        min_value = -.5 - epsilon_margin;
        max_value = .5 + epsilon_margin;
        _dx = 0;
        _dy = 0;
        index = 0;
        total_area = 4 * epsilon_margin * (1 + epsilon_margin);
        bottom_side_crit = (3 + 2 * epsilon_margin) * epsilon_margin;
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
            double r = randomDouble() * total_area;

            if (r < epsilon_margin) { // left side
                _dx = randomDouble(min_value, -.5);
                _dy = randomDouble(-.5, .5);
            } else if (r < 2 * epsilon_margin) { // right side
                _dx = randomDouble(.5, max_value);
                _dy = randomDouble(-.5, .5);
            } else if (r < bottom_side_crit) { // bottom
                _dx = randomDouble(-min_value, max_value);
                _dy = randomDouble(min_value, -.5);
            } else { // top
                _dx = randomDouble(-min_value, max_value);
                _dy = randomDouble(.5, max_value);
            }
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

private:
    std::size_t intensity;
    std::size_t number_of_samples;
    std::size_t index;

    double epsilon_margin;
    double _dx;
    double _dy;
    double min_value;
    double max_value;
    double total_area;
    double bottom_side_crit;
};

class SkewedPPPSamplerFactory: public SamplerFactory {
public:
    SkewedPPPSamplerFactory(std::size_t number_of_samples, double confidence): confidence(confidence), number_of_samples(number_of_samples) {
        // here we try to estimate the gamma parameter of a PPP thrown into an eps-dilated square which would
        // have an expected number of generated points inside the unit square of exactly "intensity"

        auto N = (double)number_of_samples;
        double n = N;
        double margin;

        for (int i = 0 ; i < 10 ; i++) {
            margin = sqrt(log(n * log(n)) - log(log(1./confidence))) / sqrt(pi * n);
            n = N * (1 + 2 * margin) * (1 + 2 * margin);
        }

        skewed_intensity = (std::size_t)n + 1;
    }

    shared_ptr<PixelSampler> create(double x, double y) override {
//        return make_shared<SkewedPPPSampler>(x, y, number_of_samples, skewed_intensity, confidence);
        return make_shared<PPPSampler>(x, y, skewed_intensity, confidence);
    }

protected:
    std::size_t skewed_intensity;
    std::size_t number_of_samples;
    double confidence;
};

#endif //YAPT_SAMPLER_H
