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
        return {
            x + randomDouble() - .5,
            y + randomDouble() - .5,
            0
        };
    }

    int sampleSize() override {
        return this->size;
    }

protected:
    int size = 10;
    int index = 0;

};


class StratifiedPixelSampler: public PixelSampler {
public:
    StratifiedPixelSampler(double x, double y, int sqrtSpp): PixelSampler(x, y), sqrtSpp(sqrtSpp) {step = 1. / (sqrtSpp + 1);}

    void begin() override {
        dx = 0;
        dy = 0;
        step = 1. / (sqrtSpp);
    }

    bool hasNext() override {
        return dy < sqrtSpp;
    }

    Point3 get() override {
        Point3 p(
                x + step * (dx + randomDouble()),
                y + step * (dy + randomDouble()),
                0);
        dx++;
        if (dx >= sqrtSpp) {
            dx = 0;
            dy++;
        }

        return p;
    }

    int sampleSize() override {
        return sqrtSpp * sqrtSpp;
    }

protected:
    int sqrtSpp = 10;
    double step;
    int dx = 0;
    int dy = 0;
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

#endif //YAPT_SAMPLER_H
