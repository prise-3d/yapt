//
// Created by franck on 07/06/24.
//

#include "yapt.h"
#include "hittable_list.h"
#include "sphere.h"
#include "camera.h"
#include "material.h"
#include <chrono>
#include "texture.h"
#include "image_exporter.h"
#include "triangle.h"
#include <iomanip>
#include "demo.h"

#define PI_N 1000000000
#define PI_N_THREADS 20

void compute_pi() {

    int n_inside = 0;
    for (int i = 0 ; i < PI_N ; i++) {
        double x = randomDouble();
        double y = randomDouble();
        if (x * x + y * y < 1) n_inside++;
    }
    std::cout << 4 * double(n_inside) / double(PI_N) << std::endl;
}

#include <random>
#include <thread>
#include <vector>
#include <atomic>

struct alignas(64) AlignedInt {
    std::atomic<int> value{0};
};

void compute_some_pi(AlignedInt* count) {
    thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (int i = 0; i < PI_N / PI_N_THREADS; i++) {
        double x = dist(rng);
        double y = dist(rng);
        if (x * x + y * y < 1) count->value++;
    }
}

void compute_pi_several_threads() {
    AlignedInt n_inside[PI_N_THREADS];

    std::vector<std::thread> threads;
    for (int i = 0; i < PI_N_THREADS; ++i) {
        threads.emplace_back(compute_some_pi, &n_inside[i]);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    int inside = 0;
    for (int i = 0; i < PI_N_THREADS; i++) {
        inside += n_inside[i].value.load();
    }

    std::cout << 4 * double(inside) / double(PI_N) << std::endl;
}

int main(int argc, char* argv[]) {

    std::string path ="out.png";
    std::string aggregator = "vor";
    std::string sampler = "sppp";
    std::size_t spp = 500;
    double confidence = .999;

    std::string pathprefix = "path=";
    std::string sppprefix = "spp=";
    std::string samplerprefix = "sampler=";
    std::string aggregatorprefix = "aggregator=";
    std::string confidenceprefix = "confidence=";

    for (int i = 0 ; i < argc ; i++) {
        std::string parameter(argv[i]);
        if (parameter.rfind(sppprefix, 0) == 0) {
            spp = std::stoi(parameter.substr(sppprefix.size()));
        }
        if (parameter.rfind(pathprefix, 0) == 0) {
            path = parameter.substr(pathprefix.size());
        }
        if (parameter.rfind(samplerprefix, 0) == 0) {
            sampler = parameter.substr(samplerprefix.size());
        }
        if (parameter.rfind(aggregatorprefix, 0) == 0) {
            aggregator = parameter.substr(aggregatorprefix.size());
        }
        if (parameter.rfind(confidenceprefix, 0) == 0) {
            confidence = std::stod(parameter.substr(confidenceprefix.size()));
        }
        if (parameter.rfind("help", 0) == 0) {
            std::cout << "usage: yapt path=out/pic.png spp=1000 sampler=sppp aggregator=vor" << std::endl;
            std::cout << " - path       => path to render output (DEFAULT = out.png)" << std::endl;
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
            return 0;
        }

    }

    std::shared_ptr<PixelSamplerFactory> samplerFactory;
    std::shared_ptr<AggregatorFactory> aggregatorFactory;

    // SAMPLER FACTORY INIT
    if (sampler == "rnd") {
        samplerFactory = std::make_shared<TrivialPixelSamplerFactory>(spp);
    } else if (sampler == "strat") {
        auto sqrtSpp = (std::size_t) sqrt(spp);
        samplerFactory = std::make_shared<StratifiedPixelSamplerFactory>(sqrtSpp);
        if ((sqrtSpp * sqrtSpp) < spp) {
            std::cout << "WARNING: spp is not a square. using spp=" << sqrtSpp * sqrtSpp << std::endl;
        }
        spp = sqrtSpp * sqrtSpp;
    } else if (sampler == "ppp") {
        samplerFactory = std::make_shared<PPPPixelSamplerFactory>(spp, confidence);
    } else if (sampler == "sppp") {
        samplerFactory = std::make_shared<SkewedPPPPixelSamplerFactory>(spp, .999);
    }

    // AGGREGATOR FACTORY INIT
    if (aggregator == "mc") {
        aggregatorFactory = std::make_shared<MCAggregatorFactory>();
    } else if (aggregator == "vor") {
        aggregatorFactory = std::make_shared<VoronoiAggregatorFactory>();
    }

    std::cout << "path=       " << path        << std::endl;
    std::cout << "spp=        " << spp         << std::endl;
    std::cout << "sampler=    " << sampler     << std::endl;
    std::cout << "aggregator= " << aggregator  << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    cornellBox(path, samplerFactory, aggregatorFactory);

//    switch(1) {
//        case 1:  break;
//        case 2: simple_light(path); break;
//        case 3: compute_pi(); break;
//        case 4: compute_pi_several_threads(); break;
//        case 5: original(path); break;
//    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
    std::clog << "Rendering duration: " << double(duration.count()) / 1000. << " s" << std::endl;
}