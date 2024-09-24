//
// Created by franck on 07/06/24.
//

#include "yapt.h"
#include "camera.h"
#include <chrono>
#include <iomanip>
#include "demo.h"

int main(int argc, char* argv[]) {

    std::string path;
    std::string aggregator = "vor";
    std::string sampler = "sppp";
    std::string dir;
    std::size_t spp = 500;
    double confidence = .999;
    std::string source;
    std::size_t maxDepth = 25;
    std::size_t numThreads = 0;

    std::string pathprefix = "path=";
    std::string sppprefix = "spp=";
    std::string samplerprefix = "sampler=";
    std::string aggregatorprefix = "aggregator=";
    std::string confidenceprefix = "confidence=";
    std::string sourceprefix = "source=";
    std::string maxDepthprefix = "maxdepth=";
    std::string dirprefix = "dir=";
    std::string numThreadsprefix = "threads=";

    for (int i = 0 ; i < argc ; i++) {
        std::string parameter(argv[i]);
        if (parameter.rfind(sppprefix, 0) == 0) {
            spp = std::stoi(parameter.substr(sppprefix.size()));
        }
        else if (parameter.rfind(pathprefix, 0) == 0) {
            path = parameter.substr(pathprefix.size());
        }
        else if (parameter.rfind(samplerprefix, 0) == 0) {
            sampler = parameter.substr(samplerprefix.size());
        }
        else if (parameter.rfind(aggregatorprefix, 0) == 0) {
            aggregator = parameter.substr(aggregatorprefix.size());
        }
        else if (parameter.rfind(confidenceprefix, 0) == 0) {
            confidence = std::stod(parameter.substr(confidenceprefix.size()));
        }
        else if (parameter.rfind(sourceprefix, 0) == 0) {
            source = parameter.substr(sourceprefix.size());
        }
        else if (parameter.rfind(maxDepthprefix, 0) == 0) {
            maxDepth = std::stoi(parameter.substr(maxDepthprefix.size()));
        }
        else if (parameter.rfind(numThreadsprefix, 0) == 0) {
            numThreads = std::stoi(parameter.substr(numThreadsprefix.size()));
        }
        else if (parameter.rfind(dirprefix, 0) == 0) {
            dir = parameter.substr(dirprefix.size());
        }
        else if (parameter.rfind("help", 0) == 0) {
            std::cout << "usage: yapt path=out/pic.png spp=1000 sampler=sppp aggregator=vor" << std::endl;
            std::cout << " - path       => path to render output (optional)" << std::endl;
            std::cout << " - spp        => samples per pixel (DEFAULT=500)" << std::endl;
            std::cout << " - sampler    => pixel sampling method:" << std::endl;
            std::cout << "                 - rnd   => pure random sampling" << std::endl;
            std::cout << "                 - strat => stratified sampling" << std::endl;
            std::cout << "                 - ppp   => Poisson Point Process sampling with margin" << std::endl;
            std::cout << "                 - sppp  => Skewed Poisson Point Process sampling with margin (DEFAULT)" << std::endl;
            std::cout << " - aggregator => path aggregation method:" << std::endl;
            std::cout << "                 - mc  => Monte Carlo integration" << std::endl;
            std::cout << "                 - vor => Voronoi aggregation (DEFAULT)" << std::endl;
            std::cout << " - confidence => Voronoi aggregation confidence (DEFAULT=.999)" << std::endl;
            std::cout << " - source     => Scene model to import" << std::endl;
            std::cout << " - maxdepth   => maximum path depth (DEFAULT=25)" << std::endl;
            std::cout << " - dir        => output directory (optional, ignored if path is specified)" << std::endl;
            std::cout << " - threads    => number of threads used (DEFAULT=hardware_concurrency)" << std::endl;
            return 0;
        }
    }

    std::shared_ptr<SamplerFactory> samplerFactory;
    std::shared_ptr<AggregatorFactory> aggregatorFactory;

    // SAMPLER FACTORY INIT
    if (sampler == "rnd") {
        samplerFactory = std::make_shared<TrivialSamplerFactory>(spp);
    } else if (sampler == "strat") {
        auto sqrtSpp = (std::size_t) sqrt(spp);
        samplerFactory = std::make_shared<StratifiedSamplerFactory>(sqrtSpp);
        if ((sqrtSpp * sqrtSpp) < spp) {
            std::cout << "WARNING: spp is not a square. using spp=" << sqrtSpp * sqrtSpp << std::endl;
        }
        spp = sqrtSpp * sqrtSpp;
    } else if (sampler == "ppp") {
        samplerFactory = std::make_shared<PPPSamplerFactory>(spp, confidence);
    } else if (sampler == "sppp") {
        samplerFactory = std::make_shared<SkewedPPPSamplerFactory>(spp, confidence);
    }

    // AGGREGATOR FACTORY INIT
    if (aggregator == "mc") {
        aggregatorFactory = std::make_shared<MCAggregatorFactory>();
    } else if (aggregator == "vor") {
        aggregatorFactory = std::make_shared<VoronoiAggregatorFactory>();
    }

    //  PATH CONSTRUCTION
    if (path.empty()) {
        std::ostringstream stream;
        if (!dir.empty()) stream << dir;
        if (!source.empty()) stream << source << "-";
        stream << aggregator << "-" << sampler << "-" << spp << "-depth" << maxDepth;

        stream << ".png";
        path = stream.str();
    }

    std::cout << "path=       " << path        << std::endl;
    std::cout << "spp=        " << spp         << std::endl;
    std::cout << "sampler=    " << sampler     << std::endl;
    std::cout << "aggregator= " << aggregator  << std::endl;
    std::cout << "confidence= " << confidence  << std::endl;
    std::cout << "maxdepth=   " << maxDepth    << std::endl;
    std::cout << "source=     " << source      << std::endl;
    std::cout << "dir=        " << dir         << std::endl;
    std::cout << "threads=    " << numThreads  << std::endl;


    auto start = std::chrono::high_resolution_clock::now();


    if (source == "test") {
        test(path, samplerFactory, aggregatorFactory, maxDepth, numThreads);
    } else {
        cornellBox(path, samplerFactory, aggregatorFactory, maxDepth, numThreads);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
    std::clog << "Rendering duration: " << double(duration.count()) / 1000. << " s" << std::endl;
}

//Color f(Sample sample) {
//    double x = sample.x;
//    double y = sample.y;
//
//    if (x * x + y * y < 1)
//        return {1., 0, 0};
//    else
//        return {0, 0, 0};
//}
//
//int main() {
//    for (int i = 0; i < 20; i++) {
//        randomSeed();
//        std::shared_ptr<SamplerFactory> samplerFactory = std::make_shared<SkewedPPPSamplerFactory>(50000,
//                                                                                                             .999);
//
//        std::shared_ptr<AggregatorFactory> aggregatorFactory = std::make_shared<VoronoiAggregatorFactory>();
//
//        std::shared_ptr<SampleAggregator> aggregator = aggregatorFactory->create();
//
//        aggregator->sampleFrom(samplerFactory, .5, .5);
//        for (aggregator->traverse(); aggregator->hasNext();) {
//            Sample sample = aggregator->next();
//            Color color = f(sample);
//            aggregator->insertContribution(color);
//        }
//        std::cout << aggregator->aggregate() << std::endl;
//    }
//}