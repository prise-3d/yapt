//
// Created by franck on 18/06/24.
//

#ifndef YAPT_SAMPLER_H
#define YAPT_SAMPLER_H

#include "yapt.h"

class PixelSampler {
public:
    PixelSampler(double x, double y): x(x), y(y) {}
    virtual void begin() = 0;
    virtual bool hasNext() = 0;
    virtual Point3 get() = 0;
    virtual int sampleSize() = 0;
    virtual double dx() = 0;
    virtual double dy() = 0;

protected:
    double x;
    double y;
};

class TrivialPixelSampler: public PixelSampler {
public:
    TrivialPixelSampler(double x, double y, int size): PixelSampler(x, y), size(size) {}

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

    int sampleSize() override {
        return this->size;
    }

protected:
    int size = 10;
    int index = 0;
    double _dx;
    double _dy;
};


class StratifiedPixelSampler: public PixelSampler {
public:
    StratifiedPixelSampler(double x, double y, int sqrtSpp): PixelSampler(x, y), sqrtSpp(sqrtSpp) {step = 1. / (sqrtSpp + 1);}

    void begin() override {
        inernal_dx = 0;
        internal_dy = 0;
        step = 1. / (sqrtSpp);
    }

    bool hasNext() override {
        return internal_dy < sqrtSpp;
    }

    Point3 get() override {
        _dx = step * (inernal_dx + randomDouble());
        _dy = step * (internal_dy + randomDouble());
        Point3 p(
                x + _dx,
                y + _dy,
                0);
        inernal_dx++;
        if (inernal_dx >= sqrtSpp) {
            inernal_dx = 0;
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

    int sampleSize() override {
        return sqrtSpp * sqrtSpp;
    }

protected:
    int sqrtSpp = 10;
    double step;
    int inernal_dx = 0;
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
    PPPPixelSampler(double x, double y, std::size_t intensity, double confidence) : PixelSampler(x, y), intensity(intensity), confidence(confidence) {}

    void begin() override {}
    bool hasNext() override { return false; }
    Point3 get() override {return Point3(0, 0, 0); };
    int sampleSize() override { return 0; }
    double dx() override { return 0.; }
    double dy() override { return 0.; }

private:
    std::size_t intensity;
    double confidence;
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

#endif //YAPT_SAMPLER_H
