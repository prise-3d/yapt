//
// Created by franck on 18/06/24.
//

#ifndef YAPT_SAMPLER_H
#define YAPT_SAMPLER_H

#include "yapt.h"

class PixelSampler {
public:
    PixelSampler(double x, double y): x(x), y(y) {}
    virtual ~PixelSampler() = default;
    virtual void begin() = 0;
    virtual bool hasNext() = 0;
    virtual Point3 get() = 0;
    virtual std::size_t sampleSize() = 0;
    virtual double dx() = 0;
    virtual double dy() = 0;

    virtual bool isVirtual() = 0;

protected:
    double x;
    double y;
};

class TrivialPixelSampler: public PixelSampler {
public:
    TrivialPixelSampler(double x, double y, int size): PixelSampler(x, y), size(size), _dx(0), _dy(0) {}

    void begin() override {
        index = 0;
    }

    bool hasNext() override {
        return index < size;
    }

    Point3 get() override {
        index++;
        _dx = randomDouble() - .5;
        _dy = randomDouble() - .5;
        return {
            x + _dx,
            y + _dy,
            0
        };
    }

    double dx() override {
        return _dx;
    }

    double dy() override {
        return _dy;
    }

    std::size_t sampleSize() override {
        return this->size;
    }

    bool isVirtual() override {
        return false;
    }

protected:
    int size = 10;
    int index = 0;
    double _dx;
    double _dy;
};


class StratifiedPixelSampler: public PixelSampler {
public:
    StratifiedPixelSampler(double x, double y, int sqrtSpp): PixelSampler(x, y), sqrtSpp(sqrtSpp), _dx(0), _dy(0) {step = 1. / (sqrtSpp + 1);}

    void begin() override {
        internal_dx = 0;
        internal_dy = 0;
        step = 1. / (sqrtSpp);
    }

    bool hasNext() override {
        return internal_dy < sqrtSpp;
    }

    Point3 get() override {
        _dx = step * (internal_dx + randomDouble());
        _dy = step * (internal_dy + randomDouble());
        Point3 p(
                x + _dx,
                y + _dy,
                0);
        internal_dx++;
        if (internal_dx >= sqrtSpp) {
            internal_dx = 0;
            internal_dy++;
        }

        return p;
    }

    double dx() override {
        return _dx;
    }

    double dy() override {
        return _dy;
    }

    std::size_t sampleSize() override {
        return sqrtSpp * sqrtSpp;
    }

    bool isVirtual() override {
        return false;
    }

protected:
    int sqrtSpp = 10;
    double step;
    int internal_dx = 0;
    int internal_dy = 0;
    double _dx;
    double _dy;
};

class PixelSamplerFactory {
public:
    virtual shared_ptr<PixelSampler> create(double x, double y) = 0;

};

class TrivialPixelSamplerFactory: public PixelSamplerFactory {
public:
    explicit TrivialPixelSamplerFactory(int samples): samples(samples) {}
    shared_ptr<PixelSampler> create(double x, double y) override {
        return make_shared<TrivialPixelSampler>(x, y, samples);
    }

protected:
    int samples;
};

class StratifiedPixelSamplerFactory : public PixelSamplerFactory {
public:
    explicit StratifiedPixelSamplerFactory(int sqrtSpp): sqrtSpp(sqrtSpp) {}

    shared_ptr<PixelSampler> create(double x, double y) override {
        return make_shared<StratifiedPixelSampler>(x, y, sqrtSpp);
    }
protected:
    int sqrtSpp;
};

class PPPPixelSampler : public PixelSampler {
public:
    PPPPixelSampler(double x, double y, double intensity, double confidence) : PixelSampler(x, y), intensity(intensity), _dx(0), _dy(0), index(0) {
        epsilon_margin = sqrt(log(intensity * log(intensity)) - log(log(1./confidence))) / sqrt(pi * intensity);
        referencePoint = Point3(x, y, 0);
        min_value = -.5 - epsilon_margin;
        max_value = .5 + epsilon_margin;
    }

    void begin() override {
        index = 0;
        total_samples_amount = Poisson(intensity).next();

//        all_samples = std::vector<Vec3>(total_samples_amount);
//
//        double min = -.5 - epsilon_margin;
//        double max = .5 + epsilon_margin;
//        for (int i = 0 ; i < total_samples_amount ; i++) {
//            Vec3 v(
//                    randomDouble(min, max),
//                    randomDouble(min, max),
//                    0
//                );
//            all_samples[i] = v;
//        }
    }
    bool hasNext() override { return index < total_samples_amount; }
    Point3 get() override {
        Vec3 v(
                randomDouble(min_value, max_value),
                randomDouble(min_value, max_value),
                0
        );
        _dx = v.x();
        _dy = v.y();
        ++index;
        return v + referencePoint;
    };
    size_t sampleSize() override { return total_samples_amount; }
    double dx() override { return _dx; }
    double dy() override { return _dy; }

    bool isVirtual() override {
        return _dx >= .5 || _dx < -.5 || _dy < -.5 || _dy >= .5;
    }

private:
    double intensity;
    std::size_t total_samples_amount = 0;
//    std::vector<Vec3> all_samples;

    std::size_t index;
    double epsilon_margin;
    double _dx;
    double _dy;
    Point3 referencePoint;
    double min_value;
    double max_value;
};

class PPPPixelSamplerFactory: public PixelSamplerFactory {
public:
    PPPPixelSamplerFactory(std::size_t intensity, double confidence): intensity(intensity), confidence(confidence) {}

    shared_ptr<PixelSampler> create(double x, double y) override {
        return make_shared<PPPPixelSampler>(x, y, intensity, confidence);
    }

protected:
    std::size_t intensity;
    double confidence;
};





class SkewedPPPPixelSampler : public PixelSampler {
public:
    SkewedPPPPixelSampler(double x, double y, std::size_t number_of_samples, double intensity, double confidence) :
        PixelSampler(x, y),
        number_of_samples(number_of_samples),
        intensity(intensity) {
        epsilon_margin = sqrt(log(intensity * log(intensity)) - log(log(1./confidence))) / sqrt(pi * intensity);
        referencePoint = Point3(x, y, 0);
        min_value = -.5 - epsilon_margin;
        max_value = .5 + epsilon_margin;
        _dx = 0;
        _dy = 0;
        _is_virtual = false;
        index = 0;
        total_area = 4 * epsilon_margin * (1 + epsilon_margin);
        bottom_side_crit = (3 + 2 * epsilon_margin) * epsilon_margin;
    }

    void begin() override {
        index = 0;
    }

    bool hasNext() override { return index < intensity; }

    Point3 get() override {
        if (index < number_of_samples) {
            Vec3 v(
                randomDouble(-.5, .5),
                randomDouble(-.5, .5),
                0
            );
            _dx = v.x();
            _dy = v.y();
            ++index;
            return v + referencePoint;
        } else {
            _is_virtual = true;
            double r = randomDouble() * total_area;
            Vec3 v(0, 0, 0);
            if (r < epsilon_margin) { // left side
                v.e[0] = randomDouble(min_value, -.5);
                v.e[1] = randomDouble(-.5, .5);
            } else if (r < 2 * epsilon_margin) { // right side
                v.e[0] = randomDouble(.5, max_value);
                v.e[1] = randomDouble(-.5, .5);
            } else if (r < bottom_side_crit) { // bottom
                v.e[0] = randomDouble(-min_value, max_value);
                v.e[1] = randomDouble(min_value, -.5);
            } else { // top
                v.e[0] = randomDouble(-min_value, max_value);
                v.e[1] = randomDouble(.5, max_value);
            }
            _dx = v.x();
            _dy = v.y();
            ++index;
            return v + referencePoint;
        }
    };
    size_t sampleSize() override { return intensity; }
    double dx() override { return _dx; }
    double dy() override { return _dy; }

    bool isVirtual() override {
        return _dx >= .5 || _dx < -.5 || _dy < -.5 || _dy >= .5;
    }

private:
    std::size_t intensity;
    std::size_t number_of_samples;
    std::size_t index;

    double epsilon_margin;
    double _dx;
    double _dy;
    Point3 referencePoint;
    double min_value;
    double max_value;
    bool _is_virtual;
    double total_area;
    double bottom_side_crit;
};

class SkewedPPPPixelSamplerFactory: public PixelSamplerFactory {
public:
    SkewedPPPPixelSamplerFactory(std::size_t number_of_samples, double confidence): confidence(confidence), number_of_samples(number_of_samples) {
        // here we try to estimate the gamma parameter of a PPP thrown into an eps-dilated square which would
        // have an expected number of generated points inside the unit square of exactly "intensity"

        auto N = (double)number_of_samples;
        double n = N;
        double margin;

        for (int i = 0 ; i < 10 ; i++) {
            margin = sqrt(log(n * log(n)) - log(log(1./confidence))) / sqrt(pi * n);
            n = N * (1 + margin) * (1 + margin);
        }

        skewed_intensity = (std::size_t)n + 1;
        std::cout << "skewed_intensity = " << skewed_intensity << std::endl;
        std::cout << "margin = " << margin << std::endl;
    }

    shared_ptr<PixelSampler> create(double x, double y) override {
        return make_shared<SkewedPPPPixelSampler>(x, y, number_of_samples, skewed_intensity, confidence);
    }

protected:
    std::size_t skewed_intensity;
    std::size_t number_of_samples;
    double confidence;
};







#endif //YAPT_SAMPLER_H
