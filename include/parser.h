/*
 * This file is part of the YAPT distribution (https://github.com/prise-3d/yapt).
 * Copyright (c) 2025 PrISE-3D.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * --- ADDITIONAL PERMISSION UNDER GNU GPL VERSION 3 SECTION 7 ---
 *
 * If you modify this Program, or any covered work, by linking or
 * combining it with the Intel Math Kernel Library (MKL) (or a modified
 * version of that library), containing parts covered by the terms of
 * the Intel Simplified Software License, the licensors of this
 * Program grant you additional permission to convey the resulting work.
 */

#ifndef PARSER_H
#define PARSER_H

#include "yapt.h"

#include <filesystem>
#include <regex>

#include "image_exporter.h"
#include "sceneloader.h"
#include "scene.h"
#include "exprtk/exprtk.hpp"

class Parser {

protected:
    std::shared_ptr<Camera> camera = std::make_shared<ForwardParallelCamera>();
    std::shared_ptr<HittableList> content = make_shared<HittableList>();
    std::shared_ptr<HittableList> lights = make_shared<HittableList>();
    std::shared_ptr<SamplerFactory> samplerFactory;
    std::shared_ptr<AggregatorFactory> aggregatorFactory;
    std::filesystem::path source = "../scenes/cornell.ypt";
    std::string cameraType = "std";
    std::string aggregator = "vor";
    std::string sampler = "sppp";
    std::size_t spp = 100;
    double confidence = .999;
    std::size_t maxDepth = 25;
    std::size_t numThreads = 0;
    std::size_t width = 0;
    std::size_t pixel_x = 0;
    std::size_t pixel_y = 0;
    std::size_t monSize = 5;
    bool winClip = false;
    double winRate = .05;
    bool nee = false;

    long seed;
    bool silent = false;

    std::chrono::time_point<std::chrono::system_clock> start;
    std::chrono::time_point<std::chrono::system_clock> end;
    std::chrono::duration<long, std::ratio<1, 1000>> render_time;

    public:
    Parser() = default;
    ~Parser() = default;

    shared_ptr<Camera> getCamera() { return camera; }
    shared_ptr<HittableList> getContent() { return content; }
    shared_ptr<HittableList> getLights() { return lights; }
    shared_ptr<SamplerFactory> getSamplerFactory() { return samplerFactory; }
    shared_ptr<AggregatorFactory> getAggregatorFactory() { return aggregatorFactory; }

    long getSeed() const { return seed; }
    std::size_t getWidth() const { return width; }
    std::size_t getSPP() const { return spp; }

    void startTimer() {
        start = std::chrono::system_clock::now();
    }

    void stopTimer() {
        end = std::chrono::high_resolution_clock::now();
        render_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    }

    bool parseScene(int argc, char* argv[], Scene& scene) {
        seed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        const std::string sppprefix = "spp=";
        const std::string samplerprefix = "sampler=";
        const std::string aggregatorprefix = "aggregator=";
        const std::string confidenceprefix = "confidence=";
        const std::string sourceprefix = "source=";
        const std::string maxDepthprefix = "maxdepth=";
        const std::string numThreadsprefix = "threads=";
        const std::string widthprefix = "width=";
        const std::string camprefix = "cam=";
        const std::string monsizeprefix = "monsize=";
        const std::string winclipprefix = "winclip=";
        const std::string winrateprefix = "winrate=";
        const std::string silentprefix = "silent";
        const std::string seedprefix = "seed=";
        const std::string neeprefix = "nee=";

        const std::regex pixelcam_coords(R"(cam=pixel-([0-9]+),([0-9]+))");
        const std::regex singlecam_coords(R"(cam=one-([0-9]+),([0-9]+))");
        std::smatch matches;

        for (int i = 1 ; i < argc ; i++) {
            std::string parameter(argv[i]);
            if (parameter.rfind(sppprefix, 0) == 0) {
                spp = std::stoi(parameter.substr(sppprefix.size()));
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

            else if (parameter.rfind(camprefix, 0) == 0) {
                cameraType = parameter.substr(camprefix.size());
            }
            else if (parameter.rfind(monsizeprefix, 0) == 0) {
                monSize = std::stoi(parameter.substr(monsizeprefix.size()));
            }
            else if (parameter.rfind(winrateprefix, 0) == 0) {
                winRate = std::stod(parameter.substr(winrateprefix.size()));
            }
            else if (parameter.rfind(winclipprefix, 0) == 0) {
                std::string b = parameter.substr(winclipprefix.size());
                winClip = (b == "true");
            }
            else if (parameter.rfind(neeprefix, 0) == 0) {
                std::string b = parameter.substr(neeprefix.size());
                nee = (b == "true");
            }
            else if (parameter.rfind(silentprefix, 0) == 0) {
                silent = true;
            }
            else if (parameter.rfind(seedprefix, 0) == 0) {
                seed = std::stol(parameter.substr(seedprefix.size()));
            }
            else if (parameter.rfind("help", 0) == 0) {
                std::cout << "usage: yapt path=out/pic.png spp=1000 sampler=sppp aggregator=vor" << std::endl;
                std::cout << " - path       => path to render output (optional)" << std::endl;
                std::cout << " - spp        => samples per pixel (DEFAULT=500)" << std::endl;
                std::cout << " - sampler    => pixel sampling method:" << std::endl;
                std::cout << "                 - rnd    => pure random sampling" << std::endl;
                std::cout << "                 - strat  => stratified sampling" << std::endl;
                std::cout << "                 - sppp   => Skewed Poisson Point Process sampling with margin (DEFAULT)" << std::endl;
                std::cout << " - aggregator => path aggregation method:" << std::endl;
                std::cout << "                 - mc      => Monte Carlo integration" << std::endl;
                std::cout << "                 - vor     => Voronoi aggregation (DEFAULT)" << std::endl;
                std::cout << "                 - cvor    => Clipped Voronoi aggregation" << std::endl;
                std::cout << "                 - fvor    => Filtering Voronoi aggregation" << std::endl;
                std::cout << "                 - nvor    => Nico Voronoi aggregation" << std::endl;
                std::cout << "                 - median  => Median aggregation" << std::endl;
                std::cout << "                 - mon     => MoN (Median Of meaNs) aggregation" << std::endl;
                std::cout << "                 - winsor  =>  Winsorization" << std::endl;
                std::cout << "                 - fbounce =>  First Bounce Voronoi aggregation" << std::endl;
                std::cout << " - confidence => Voronoi aggregation confidence (DEFAULT=.999)" << std::endl;
                std::cout << " - source     => Scene model to import" << std::endl;
                std::cout << " - maxdepth   => maximum path depth (DEFAULT=25)" << std::endl;
                std::cout << " - dir        => output directory (optional, ignored if path is specified)" << std::endl;
                std::cout << " - threads    => number of threads used (DEFAULT=hardware_concurrency)" << std::endl;
                std::cout << " - width      => force image width (DEFAULT=scene dependent)" << std::endl;
                std::cout << " - cam        => camera type" << std::endl;
                std::cout << "                 - std       => standard camera type (DEFAULT) " << std::endl;
                std::cout << "                 - norm      => renders normals to surfaces " << std::endl;
                std::cout << "                 - biased    => biased, low non-contribution camera" << std::endl;
                std::cout << "                 - test      => test camera" << std::endl;
                std::cout << "                 - pixel-x,y => pixel cartography camera @coords (x,y)" << std::endl;
                std::cout << "                 - one-x,y   => renders only one pixel @coords (x,y)" << std::endl;
                std::cout << " - monsize    => number of MoN blocks (DEFAULT = 5)" << std::endl;
                std::cout << " - winrate    => Winsor reject rate (DEFAULT = 0.05)" << std::endl;
                std::cout << " - winclip    => Winsor clipping (DEFAULT = false)" << std::endl;
                std::cout << " - seed       => RNG seed (DEFAULT = random seed)" << std::endl;
                std::cout << " - nee        => Next Event Estimation (DEFAULT = false)" << std::endl;
                return false;
            }
            if (std::regex_match(parameter, matches, pixelcam_coords)) {
                cameraType = "pixel";
                pixel_x = std::stoi(matches[1]);
                pixel_y = std::stoi(matches[2]);
            } else if (std::regex_match(parameter, matches, singlecam_coords)) {
                cameraType = "single";
                pixel_x = std::stoi(matches[1]);
                pixel_y = std::stoi(matches[2]);
                std::cout << " \n\n  SINGLE = " << pixel_x << " ; " << pixel_y << std::endl << std::endl << std::endl;
            }
        }

        if (silent) {
            freopen("/dev/null", "w", stderr);
        }



        // SAMPLER FACTORY INIT
        if (sampler == "rnd") {
            samplerFactory = std::make_shared<TrivialSamplerFactory>(spp);
        } else if (sampler == "strat") {
            auto sqrtSpp = static_cast<std::size_t>(sqrt(spp));
            samplerFactory = std::make_shared<StratifiedSamplerFactory>(sqrtSpp);
            if ((sqrtSpp * sqrtSpp) < spp) {
                std::cout << "WARNING: spp is not a square. using spp=" << sqrtSpp * sqrtSpp << std::endl;
            }
            spp = sqrtSpp * sqrtSpp;
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
        } else if (aggregator == "fvor" || aggregator == "nvor") {

            auto sampler = samplerFactory->create(0, 0);
            auto sppp_sampler = dynamic_cast<SkewedPPPSampler*>(sampler.get());

            double margin = .1;
            if (sppp_sampler != nullptr) {
                margin = sppp_sampler->margin;
            }

            if (aggregator == "fvor") aggregatorFactory = std::make_shared<FilteringVoronoiAggregatorFactory>(margin);
            if (aggregator == "nvor") aggregatorFactory = std::make_shared<NicoVoronoiAggregatorFactory>(margin);
        } else if (aggregator == "median") {
            aggregatorFactory = std::make_shared<MedianAggregatorFactory>();
        } else if (aggregator == "mon") {
            aggregatorFactory = std::make_shared<MonAggregatorFactory>(monSize);
        } else if (aggregator == "winsor") {
            aggregatorFactory = std::make_shared<WinsorAggregatorFactory>(winRate, winClip);
        } else if (aggregator == "fbounce") {
            aggregatorFactory = std::make_shared<FirstBounceVoronoiFactory>();
        }

        if (cameraType == "pixel") {
            camera = std::make_shared<CartographyCamera>(pixel_x, pixel_y);
        } else if (cameraType == "single") {
            camera = std::make_shared<SinglePixelCamera>(pixel_x, pixel_y);
        } else if (cameraType == "biased") {
            camera = std::make_shared<BiasedForwardParallelCamera>();
        } else if (cameraType == "norm") {
            camera = std::make_shared<NormalCamera>();
        } else {
            camera = std::make_shared<ForwardParallelCamera>();
        }

        if (width == 0) width = 900;

        camera->numThreads = numThreads;
        camera->maxDepth = maxDepth;
        camera->samplerAggregator = aggregatorFactory;
        camera->pixelSamplerFactory = samplerFactory;
        camera->imageWidth = width;

        camera->aspect_ratio   = 1.0;
        camera->background     = Color(0, 0, 0);
        camera->vfov           = 40;
        camera->lookFrom       = Point3(278, 278, -800);
        camera->lookAt         = Point3(278, 278, 0);
        camera->vup            = Vec3(0, 1, 0);
        camera->defocusAngle   = 0;
        camera->seed           = seed;

        // Set the sampling strategy based on nee flag
        if (nee) {
            camera->samplingStrategy = make_shared<NEESamplingStrategy>();
        } else {
            camera->samplingStrategy = make_shared<MixtureSamplingStrategy>();
        }

        if (source.extension() == ".ypt") {
            YaptSceneLoader loader;
            loader.load(source, content, lights, camera);
            if (silent) {
                freopen("/dev/tty", "w", stderr);
            }
        } else if (source.extension() == ".obj") {
            camera->background = Color(1., .5, .5);
        }

#ifdef FUNCTION_PARSING
        else if (source.extension() == ".func") {

            camera = std::make_shared<FunctionCamera>(Function::from_file(source.string()));
            camera->background = Color(0., .0, .0);
            camera->aspect_ratio = 1.;
            camera->seed = seed;
            camera->numThreads = numThreads;
            camera->maxDepth = maxDepth;
            camera->samplerAggregator = aggregatorFactory;
            camera->pixelSamplerFactory = samplerFactory;
            camera->imageWidth = width;
        }
#endif
        else {
            std::cerr << "Unrecognized source extension: " << source.extension() << std::endl;
            return false;
        }

        scene.camera = camera;
        scene.lights = lights;
        scene.content = content;

        return true;
    }

    bool exportImage(const int argc, char* argv[], const Scene& scene) const {

        std::filesystem::path dir;
        std::filesystem::path path;

        const std::string pathprefix = "path=";
        const std::string dirprefix = "dir=";

        const std::regex coords(R"(cam=pixel-([0-9]+),([0-9]+))");
        std::smatch matches;

        for (int i = 1 ; i < argc ; i++) {
            std::string parameter(argv[i]);

            if (parameter.rfind(pathprefix, 0) == 0) {
                path = parameter.substr(pathprefix.size());
            } else if (parameter.rfind(dirprefix, 0) == 0) {
                dir = parameter.substr(dirprefix.size());
            }
        }

        if (path.empty()) {
            if (!dir.empty())
                path = dir;
            std::filesystem::path filename;

            filename += source.stem();
            filename += "-";

            std::string with_nee = nee ? "-nee" : "";
            filename += aggregator + "-" + sampler + "-spp-" + std::to_string(spp) + "-w-" + std::to_string(width) + "-d-" + std::to_string(maxDepth) + with_nee + ".exr";
            path /= filename;
        }

        std::shared_ptr<ImageExporter> exporter;
        std::string destination_extension = path.extension();

        if (destination_extension == ".exr") {
            exporter = make_shared<EXRImageExporter>(scene.camera->data());
        } else if (destination_extension == ".png") {
            exporter = make_shared<PNGImageExporter>(scene.camera->data());
        }

        if (!exporter) {
            std::cerr << "Unrecognized destination extension: \"" << destination_extension << "\"." << std::endl << "Terminating." << std::endl;
            return false;
        }

        exporter->set_render_time(static_cast<size_t>(render_time.count()));
        std::cout << "Rendering duration: " << static_cast<double>(render_time.count()) / 1000. << " s" << std::endl;
        exporter->write(path);

        std::cout << "Image saved to: " << path << std::endl;

        return true;
    }
};
#endif //PARSER_H