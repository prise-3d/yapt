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
#include "ray_evaluator.h"

enum class SamplerType {
    Uniform,
    Stratified,
    SkewedPPP
};
enum class AggregatorType {
    MonteCarlo,
    Voronoi,
    ClippedVoronoi,
    FilteringVoronoi,
    NicoVoronoi,
    Median,
    MoN,
    Winsor
};
enum class CameraType {
    Parallel,
    Nested,
    Nested4,
    ClippedVoronoiNested,
    Single,
    Pixel
};
enum class EvaluatorType {
    Standard,
    Normals,
    Nested,
    ClippedVoronoiNested,
    ClippedVoronoiNestedDebug
};

enum class SceneFormatType {
    YAPT
};

enum class SamplingStrategyType {
    NextEventEstimation,
    MixturePDF
};

struct RenderConfig {
    std::filesystem::path sourcePath = "../scenes/cornell.ypt";
    std::filesystem::path outputPath; // Optional
    std::filesystem::path outputDir;  // Optional

    // Rendering parameters
    std::size_t spp = 100;
    std::size_t width = 900;
    std::size_t maxDepth = 25;
    std::size_t numThreads = 0; // 0 = auto
    long seed = 0; // 0 = random
    bool silent = false;
    SamplingStrategyType sampling_strategy_type = SamplingStrategyType::MixturePDF;
    bool help = false;

    // algorithms
    SamplerType sampler = SamplerType::SkewedPPP;
    AggregatorType aggregator = AggregatorType::Voronoi;
    CameraType camera = CameraType::Parallel;
    SceneFormatType sceneFormatType = SceneFormatType::YAPT;
    EvaluatorType evaluatorType = EvaluatorType::Standard;

    // Specific parameters
    double confidence = 0.999;
    std::size_t nested_sample_size = 100;

    // Camera-specific parameters
    std::pair<int, int> pixelCoords = {0, 0};
};

inline void display_help(const RenderConfig& config) {
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
                std::cout << " - confidence => Voronoi aggregation confidence (DEFAULT=.999)" << std::endl;
                std::cout << " - source     => Scene model to import" << std::endl;
                std::cout << " - maxdepth   => maximum path depth (DEFAULT=25)" << std::endl;
                std::cout << " - dir        => output directory (optional, ignored if path is specified)" << std::endl;
                std::cout << " - threads    => number of threads used (DEFAULT=hardware_concurrency)" << std::endl;
                std::cout << " - width      => force image width (DEFAULT=scene dependent)" << std::endl;
                std::cout << " - cam        => camera type" << std::endl;
                std::cout << "                 - std       => standard camera type (DEFAULT) " << std::endl;
                std::cout << "                 - nest      => MC Nesting (DEPRECATED)" << std::endl;
                std::cout << "                 - nest4     => MC Nesting (depth 4, DEPRECATED)" << std::endl;
                std::cout << "                 - cvnest    => Clipped Voronoi Nesting (DEPRECATED)" << std::endl;
                std::cout << "                 - normals   => renders normals to surfaces " << std::endl;
                std::cout << "                 - one-x,y   => renders only one pixel @coords (x,y)" << std::endl;
                std::cout << " - eval       => ray evaluation method" << std::endl;
                std::cout << "                 - nest      => MC Nesting" << std::endl;
                std::cout << "                 - cvnest    => Clipped Voronoi Nesting" << std::endl;
                std::cout << " - seed       => RNG seed (DEFAULT = random seed)" << std::endl;
                std::cout << " - nee        => Next Event Estimation (DEFAULT = false)" << std::endl;
                std::cout << " - nestsamples => sample count for first bounce voronoi cameras (DEFAULT = 100)" << std::endl;
}

#include <map>
#include <functional>
#include <memory>
#include <iostream>
#include <cmath>

class CommandLineParser {
    using ArgumentConsumer = std::function<void(const std::string&, RenderConfig&)>;
    std::map<std::string, ArgumentConsumer> handlers;

public:
    CommandLineParser() {
        registerHandlers();
    }

    RenderConfig parse(int argc, char* argv[]) {
        RenderConfig config;

        config.seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            for (const auto& [prefix, consumer] : handlers) {
                if (arg.rfind(prefix, 0) == 0) { // Si commence par prefix
                    std::string value = arg.substr(prefix.length());
                    consumer(value, config);
                    break;
                }
            }
        }
        return config;
    }

    void registerHandlers() {
        handlers["maxdepth="] = [](const std::string& v, RenderConfig& c) { c.maxDepth = std::stoi(v);};
        handlers["spp="] = [](const std::string& v, RenderConfig& c) { c.spp = std::stoi(v);};
        handlers["width="] = [](const std::string& v, RenderConfig& c) { c.width = std::stoi(v);};
        handlers["source="] = [](const std::string& v, RenderConfig& c) { c.sourcePath = v;};
        handlers["path="] = [](const std::string& v, RenderConfig& c) {c.outputPath = v; };
        handlers["seed="] = [](const std::string& v, RenderConfig& c) { c.seed = std::stoi(v); };
        handlers["nestsamples="] = [](const std::string& v, RenderConfig& c) { c.nested_sample_size = std::stoi(v); };
        handlers["threads="] = [](const std::string& v, RenderConfig& c) { c.numThreads = std::stoi(v); };
        handlers["nee="] = [](const std::string& v, RenderConfig& c) { c.sampling_strategy_type = ((v=="true") ? SamplingStrategyType::NextEventEstimation : SamplingStrategyType::MixturePDF); };
        handlers["help"] = [](const std::string&, RenderConfig& c) { c.help = true; };

        handlers["sampler="] = [](const std::string& v, RenderConfig& c) {
            if (v == "rnd") c.sampler = SamplerType::Uniform;
            // else if (v == "strat") c.sampler = SamplerType::Stratified; // IS BUGGY ?!
            else if (v == "sppp") c.sampler = SamplerType::SkewedPPP;
            else {
                std::cerr << "Unknown sampler: " << v << std::endl;
                std::exit(1);
            }
        };

        handlers["aggregator="] = [](const std::string& v, RenderConfig& c) {
            if (v == "mc") c.aggregator = AggregatorType::MonteCarlo;
            else if (v == "vor") c.aggregator = AggregatorType::Voronoi;
            else if (v == "cvor") c.aggregator = AggregatorType::ClippedVoronoi;
            else if (v == "fvor") c.aggregator = AggregatorType::FilteringVoronoi;
            else if (v == "nvor") c.aggregator = AggregatorType::NicoVoronoi;
            // median
            // mon
            // winsor
            else {
                std::cerr << "Unknown aggregator: " << v << std::endl;
                std::exit(1);
            }
        };

        handlers["cam="] = [](const std::string& v, RenderConfig& c) {
            if (v == "std") c.camera = CameraType::Parallel;
            else if (v == "nest") {
                // c.camera = CameraType::Nested;
                // we preserve backward compatibility
                c.evaluatorType = EvaluatorType::Nested;
                c.camera = CameraType::Parallel;
            }
            else if (v == "nest4") {
                c.camera = CameraType::Nested4;
            }
            else if (v == "cvnest") {
                // c.camera = CameraType::ClippedVoronoiNested;
                // we preserve backward compatibility
                c.evaluatorType = EvaluatorType::ClippedVoronoiNested;
                c.camera = CameraType::Parallel;
            } else {
                static const std::regex pixelCam(R"(pixel-([0-9]+),([0-9]+))");
                static const std::regex oneCam(R"(one-([0-9]+),([0-9]+))");
                std::smatch matches;

                if (std::regex_match(v, matches, oneCam)) {
                    c.camera = CameraType::Single;
                    c.pixelCoords = {std::stoi(matches[1]), std::stoi(matches[2])};
                }
                else {
                    std::cerr << "Unknown camera: " << v << std::endl;
                    std::exit(1);
                }
            }
        };

        handlers["source="] = [](const std::string& v, RenderConfig& c) {
            c.sourcePath = v;
            if (c.sourcePath.extension() == ".ypt") {
                c.sceneFormatType = SceneFormatType::YAPT;
            } else {
                std::cerr << "Unknown source format: " << v << std::endl;
                std::exit(1);
            }
        };

        handlers["eval="] = [](const std::string& v, RenderConfig& c) {
            if (v == "nest") {
                c.evaluatorType = EvaluatorType::Nested;
            } else if (v == "cvnest") {
                c.evaluatorType = EvaluatorType::ClippedVoronoiNested;
            } else if (v == "dcvnest") {
                c.evaluatorType = EvaluatorType::ClippedVoronoiNestedDebug;
            } else if (v == "normals") {
                c.evaluatorType = EvaluatorType::Normals;
            } else if (v == "standard") {
                c.evaluatorType = EvaluatorType::Standard;
            } else {
                std::cerr << "Unknown evaluator type: " << v << std::endl;
                std::exit(1);
            }
        };
    }
};

class RenderFactory {
public:
    using SamplerCreator = std::function<std::shared_ptr<SamplerFactory>(const RenderConfig&)>;
    using AggregatorCreator = std::function<std::shared_ptr<AggregatorFactory>(const RenderConfig&, std::shared_ptr<SamplerFactory>&)>;
    using CameraCreator = std::function<std::shared_ptr<Camera>(const RenderConfig&)>;
    using SceneCreator = std::function<std::shared_ptr<ContentDescription>(const RenderConfig&)>;
    using SamplingStrategyCreator = std::function<std::shared_ptr<ScatteringStrategy>(const RenderConfig&)>;
    using EvaluatorTypeCreator = std::function<std::shared_ptr<RayEvaluator>(const RenderConfig&, const shared_ptr<ScatteringStrategy>&)>;

    static void finalize_camera(const RenderConfig& cfg, const std::shared_ptr<Camera>& camera);

    static void init();

    static std::shared_ptr<SamplerFactory> createSampler(const RenderConfig& cfg);

    static std::shared_ptr<AggregatorFactory> createAggregator(const RenderConfig& cfg, std::shared_ptr<SamplerFactory> &samplerFactory);

    static std::shared_ptr<Camera> createCamera(const RenderConfig& cfg);

    static std::shared_ptr<ContentDescription> createContent(const RenderConfig& cfg);

    static std::shared_ptr<RayEvaluator> createEvaluator(const RenderConfig& cfg, const std::shared_ptr<ScatteringStrategy>&);

private:
    inline static std::map<SamplerType, SamplerCreator> samplerRegistry;
    inline static std::map<AggregatorType, AggregatorCreator> aggregatorRegistry;
    inline static std::map<CameraType, CameraCreator> cameraRegistry;
    inline static std::map<SceneFormatType, SceneCreator> sceneRegistry;
    inline static std::map<SamplingStrategyType, SamplingStrategyCreator> samplingStrategyRegistry;
    inline static std::map<EvaluatorType, EvaluatorTypeCreator> evaluatorRegistry;
};

inline void export_image(RenderConfig& cfg) {

}

class OutputManager {
protected:
    std::chrono::time_point<std::chrono::system_clock> start;
    std::chrono::time_point<std::chrono::system_clock> end;
    std::chrono::duration<long, std::ratio<1, 1000>> render_time;

public:
    explicit OutputManager(const RenderConfig& config)  {
        initialize(config);
    }
    void initialize(const RenderConfig& config) {
        sampler_descriptions[SamplerType::Uniform] = "rnd";
        sampler_descriptions[SamplerType::Stratified] = "strat";
        sampler_descriptions[SamplerType::SkewedPPP] = "sppp";

        aggregator_descriptions[AggregatorType::MonteCarlo] = "mc";
        aggregator_descriptions[AggregatorType::Voronoi] = "vor";
        aggregator_descriptions[AggregatorType::FilteringVoronoi] = "mc";
        aggregator_descriptions[AggregatorType::NicoVoronoi] = "nvor";
        aggregator_descriptions[AggregatorType::ClippedVoronoi] = "cvor";

        camera_descriptions[CameraType::Parallel] = "";
        camera_descriptions[CameraType::Single] = "single";
        camera_descriptions[CameraType::Nested] = "nest";
        camera_descriptions[CameraType::Nested4] = "nest4";
        camera_descriptions[CameraType::ClippedVoronoiNested] = "cvnest";

        evaluator_descriptions[EvaluatorType::Standard] = "";
        evaluator_descriptions[EvaluatorType::Normals] = "normals";
        evaluator_descriptions[EvaluatorType::Nested] = "nest";
        evaluator_descriptions[EvaluatorType::ClippedVoronoiNested] = "cvnest";
        evaluator_descriptions[EvaluatorType::ClippedVoronoiNestedDebug] = "cvnest";
    }

    void start_timer() {
        start = std::chrono::system_clock::now();
    }

    void stop_timer() {
        end = std::chrono::high_resolution_clock::now();
        render_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    }

    bool export_image(RenderConfig& config, const std::shared_ptr<Camera> &camera) {
        if (config.outputPath.empty()) {
            if (!config.outputDir.empty())
                config.outputPath = config.outputDir;

            std::filesystem::path filename;

            filename += config.sourcePath.stem();
            filename += "-";

            const std::string with_nee = (config.sampling_strategy_type == SamplingStrategyType::NextEventEstimation) ? "-nee" : "";
            std::string cam_tag;
            if (config.camera == CameraType::ClippedVoronoiNested || config.camera == CameraType::Nested || config.camera == CameraType::Nested4) {
                cam_tag += "-" + camera_descriptions[config.camera] + "-" + std::to_string(config.nested_sample_size);
            } else if (config.camera == CameraType::Single) {
                cam_tag += "-" + camera_descriptions[config.camera] + "(" + std::to_string(config.pixelCoords.first) + "," + std::to_string(config.pixelCoords.second) + ")";
            }

            std::string eval_tag;
            if (config.evaluatorType == EvaluatorType::ClippedVoronoiNested || config.evaluatorType == EvaluatorType::Nested) {
                eval_tag += "-" + evaluator_descriptions[config.evaluatorType] + "-" + std::to_string(config.nested_sample_size);
            } else if (config.evaluatorType == EvaluatorType::Normals) {
                eval_tag += "-" + evaluator_descriptions[config.evaluatorType];
            }

            filename +=
                aggregator_descriptions[config.aggregator] + "-" +
                sampler_descriptions[config.sampler] +
                "-spp-" + std::to_string(config.spp) +
                cam_tag +
                eval_tag +
                "-w-" + std::to_string(config.width) +
                "-d-" + std::to_string(config.maxDepth)
                + with_nee;

            filename += "-seed-";
            filename += std::to_string(config.seed);

            filename += ".exr";
            config.outputPath /= filename;
        }

        std::shared_ptr<ImageExporter> exporter;
        const std::string destination_extension = config.outputPath.extension();

        if (destination_extension == ".exr") {
            exporter = make_shared<EXRImageExporter>(camera->data());
        } else if (destination_extension == ".png") {
            exporter = make_shared<PNGImageExporter>(camera->data());
        }

        if (!exporter) {
            std::cerr << "Unrecognized destination extension: \"" << destination_extension << "\"." << std::endl << "Terminating." << std::endl;
            std::exit(1);
        }

        exporter->set_render_time(static_cast<size_t>(render_time.count()));
        std::cout << "Rendering duration: " << static_cast<double>(render_time.count()) / 1000. << " s" << std::endl;
        exporter->write(config.outputPath);

        std::cout << "Image saved to: " << config.outputPath << std::endl;

        return true;
    }

private:
    std::map<SamplerType, std::string> sampler_descriptions;
    std::map<AggregatorType, std::string> aggregator_descriptions;
    std::map<CameraType, std::string> camera_descriptions;
    std::map<EvaluatorType, std::string> evaluator_descriptions;
};

#endif //PARSER_H