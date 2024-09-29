//
// Created by franck on 07/06/24.
//

#include "yapt.h"
#include "camera.h"
#include "demo.h"
#include "sceneloader.h"
#include <chrono>
#include <iomanip>
#include <filesystem>

int main(int argc, char* argv[]) {
    std::filesystem::path path;
    std::string aggregator = "vor";
    std::string sampler = "sppp";
    std::string dir;
    std::size_t spp = 500;
    double confidence = .999;
    std::filesystem::path source;
    std::size_t maxDepth = 25;
    std::size_t numThreads = 0;
    std::size_t width = 0;

    std::string pathprefix = "path=";
    std::string format = "png";
    std::string sppprefix = "spp=";
    std::string samplerprefix = "sampler=";
    std::string aggregatorprefix = "aggregator=";
    std::string confidenceprefix = "confidence=";
    std::string sourceprefix = "source=";
    std::string maxDepthprefix = "maxdepth=";
    std::string dirprefix = "dir=";
    std::string numThreadsprefix = "threads=";
    std::string widthprefix="width=";

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
        else if (parameter.rfind(widthprefix, 0) == 0) {
            width = std::stoi(parameter.substr(widthprefix.size()));
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
            std::cout << "                 - mc   => Monte Carlo integration" << std::endl;
            std::cout << "                 - vor  => Voronoi aggregation (DEFAULT)" << std::endl;
            std::cout << "                 - cvor => Clipped Voronoi aggregation" << std::endl;
            std::cout << "                 - ivor => Inner Voronoi aggregation" << std::endl;
            std::cout << " - confidence => Voronoi aggregation confidence (DEFAULT=.999)" << std::endl;
            std::cout << " - source     => Scene model to import" << std::endl;
            std::cout << " - maxdepth   => maximum path depth (DEFAULT=25)" << std::endl;
            std::cout << " - dir        => output directory (optional, ignored if path is specified)" << std::endl;
            std::cout << " - threads    => number of threads used (DEFAULT=hardware_concurrency)" << std::endl;
            std::cout << " - width      => force image width (DEFAULT=scene dependent)" << std::endl;
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
    } else if (aggregator == "cvor") {
        aggregatorFactory = std::make_shared<ClippedVoronoiAggregatorFactory>();
    } else if (aggregator == "ivor") {
        aggregatorFactory = std::make_shared<InnerVoronoiAggregatorFactory>();
    }

    //  PATH CONSTRUCTION
    if (path.empty()) {
        std::ostringstream stream;
        if (!dir.empty()) stream << dir;
        if (!source.empty()) stream << source << "-";
        stream << aggregator << "-" << sampler << "-" << spp << "-depth-" << maxDepth;

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
    std::cout << "width=      " << width       << std::endl;


    auto start = std::chrono::high_resolution_clock::now();

    std::shared_ptr<Camera> cam = std::make_shared<ParallelCamera>();
    if (source == "test") {
        cam = std::make_shared<TestCamera>();
    } else {
        cam = std::make_shared<ParallelCamera>();
    }

    cam->numThreads = numThreads;
    cam->maxDepth = maxDepth;
    cam->samplerAggregator = aggregatorFactory;
    cam->pixelSamplerFactory = samplerFactory;
    cam->imageWidth = width;

    cam->aspect_ratio      = 1.0;
    if (cam->imageWidth == 0)
        cam->imageWidth = 900;
    cam->background = Color(0, 0, 0);
    cam->vfov     = 40;
    cam->lookFrom = Point3(278, 278, -800);
    cam->lookAt   = Point3(278, 278, 0);
    cam->vup      = Vec3(0, 1, 0);
    cam->defocusAngle = 0;

    shared_ptr scene = make_shared<HittableList>();
    shared_ptr lights = make_shared<HittableList>();

    if (source.empty()) source = "../scenes/cornell.ypt";

    if (source == "test") {
        test(cam);
    } else {
        YaptSceneLoader loader;
        loader.load(source, scene, lights, cam);
        cam->render(*scene, *lights);
    }

    std::shared_ptr<ImageExporter> exporter;
    std::string extension = path.extension();

    if (extension == ".exr") {
        std::clog << "Exporting to EXR format" << std::endl;
        exporter = make_shared<EXRImageExporter>(cam->data());
    } else if (extension == ".png") {
        std::clog << "Exporting to PNG format" << std::endl;
        exporter = make_shared<PNGImageExporter>(cam->data());
    }

    if (!exporter) {
        std::cerr << "Unrecognized file extension: \"" << extension << "\"." << std::endl << "Terminating." << std::endl;
        return 1;
    }

    exporter->write(path);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
    std::clog << "Rendering duration: " << double(duration.count()) / 1000. << " s" << std::endl;
}