//
// Created by franck on 25/09/24.
//

#include "doctest.h"
#include "yapt.h"
#include "sampler.h"
#include "aggregators.h"

Color noise(Sample sample) {
    return {randomDouble(), randomDouble(), randomDouble()};
}

Color unit_disk(Sample sample) {
    double x = sample.x;
    double y = sample.y;

    if (x * x + y * y < 1)
        return {1., 0, 0};
    else
        return {0, 0, 0};
}

TEST_CASE("vor-unit-disk") {
    randomSeed();
    std::shared_ptr<SamplerFactory> samplerFactory = std::make_shared<SkewedPPPSamplerFactory>(10000, .999);
    std::shared_ptr<AggregatorFactory> aggregatorFactory = std::make_shared<VoronoiAggregatorFactory>();

    size_t valid = 0;
    size_t N = 100;
    for (int i = 0; i < N; i++) {
        std::shared_ptr<SampleAggregator> aggregator = aggregatorFactory->create();

        aggregator->sampleFrom(samplerFactory, .5, .5);
        for (aggregator->traverse(); aggregator->hasNext();) {
            Sample sample = aggregator->next();
            aggregator->insertContribution(unit_disk(sample));
        }

        Color out =  aggregator->aggregate();

        bool zero = out.y() == 0. && out.z() == 0. && fabs(pi - 4 * out.x()) < .01;
        if (zero) valid++;
    }
    CHECK(valid == N);
}

TEST_CASE("vor-noise") {
    randomSeed();
    std::shared_ptr<SamplerFactory> samplerFactory = std::make_shared<SkewedPPPSamplerFactory>(10000, .999);
    std::shared_ptr<AggregatorFactory> aggregatorFactory = std::make_shared<VoronoiAggregatorFactory>();

    size_t valid = 0;
    size_t N = 100;
    for (int i = 0; i < N; i++) {
        std::shared_ptr<SampleAggregator> aggregator = aggregatorFactory->create();

        aggregator->sampleFrom(samplerFactory, .5, .5);
        for (aggregator->traverse(); aggregator->hasNext();) {
            Sample sample = aggregator->next();
            aggregator->insertContribution(noise(sample));
        }

        Color out =  aggregator->aggregate();

        bool ok = fabs(.5 - out.x()) < .01 &&  fabs(.5 - out.y()) < .01 && fabs(.5 - out.z()) < .01;
        if (ok) ++valid;
    }
    CHECK(valid == N);
}

TEST_CASE("mc-unit-disk") {
    randomSeed();
    std::shared_ptr<SamplerFactory> samplerFactory = std::make_shared<TrivialSamplerFactory>(10000);
    std::shared_ptr<AggregatorFactory> aggregatorFactory = std::make_shared<MCAggregatorFactory>();

    size_t valid = 0;
    size_t N = 100;
    for (int i = 0; i < N; i++) {
        std::shared_ptr<SampleAggregator> aggregator = aggregatorFactory->create();

        aggregator->sampleFrom(samplerFactory, .5, .5);
        for (aggregator->traverse(); aggregator->hasNext();) {
            Sample sample = aggregator->next();
            aggregator->insertContribution(unit_disk(sample));
        }

        Color out =  aggregator->aggregate();

        bool zero = out.y() == 0. && out.z() == 0. && fabs(pi - 4 * out.x()) < .01;
        if (zero) valid++;
    }
    CHECK(valid == N);
}

TEST_CASE("mc-noise") {
    randomSeed();
    std::shared_ptr<SamplerFactory> samplerFactory = std::make_shared<TrivialSamplerFactory>(10000);
    std::shared_ptr<AggregatorFactory> aggregatorFactory = std::make_shared<MCAggregatorFactory>();

    size_t valid = 0;
    size_t N = 100;
    for (int i = 0; i < N; i++) {
        std::shared_ptr<SampleAggregator> aggregator = aggregatorFactory->create();

        aggregator->sampleFrom(samplerFactory, .5, .5);
        for (aggregator->traverse(); aggregator->hasNext();) {
            Sample sample = aggregator->next();
            aggregator->insertContribution(noise(sample));
        }

        Color out =  aggregator->aggregate();

        bool ok = fabs(.5 - out.x()) < .01 &&  fabs(.5 - out.y()) < .01 && fabs(.5 - out.z()) < .01;
        if (ok) ++valid;
    }
}
