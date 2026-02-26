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

#include "parser.h"


void RenderFactory::finalize_camera(const RenderConfig &cfg, const std::shared_ptr<Camera> &camera) {
    std::shared_ptr<SamplerFactory> sampler_factory = createSampler(cfg);
    camera->numThreads = cfg.numThreads;
    camera->maxDepth = cfg.maxDepth;
    camera->aggregator_factory = createAggregator(cfg, sampler_factory);
    camera->sampler_factory = sampler_factory;
    camera->imageWidth = cfg.width;

    camera->aspect_ratio   = 1.0;
    camera->background     = Color(0, 0, 0);
    camera->vfov           = 40;
    camera->lookFrom       = Point3(278, 278, -800);
    camera->lookAt         = Point3(278, 278, 0);
    camera->vup            = Vec3(0, 1, 0);
    camera->defocusAngle   = 0;
    camera->seed           = cfg.seed;
    camera->scattering_strategy = samplingStrategyRegistry[cfg.sampling_strategy_type](cfg);;
}

void RenderFactory::init() {
    samplerRegistry[SamplerType::Uniform] = [](const RenderConfig& cfg) {
        return std::make_shared<TrivialSamplerFactory>(cfg.spp);
    };
    samplerRegistry[SamplerType::SkewedPPP] = [](const RenderConfig &cfg) {
        return std::make_shared<SkewedPPPSamplerFactory>(cfg.spp, cfg.confidence);
    };
    samplerRegistry[SamplerType::Stratified] = [](const RenderConfig &cfg) {
        return std::make_shared<StratifiedSamplerFactory>(cfg.spp);
    };

    aggregatorRegistry[AggregatorType::MonteCarlo] = [](const RenderConfig&, std::shared_ptr<SamplerFactory>& samplerFactory) {
        return std::make_shared<MCAggregatorFactory>();
    };
    aggregatorRegistry[AggregatorType::Voronoi] = [](const RenderConfig&, std::shared_ptr<SamplerFactory>& samplerFactory) {
        return std::make_shared<VoronoiAggregatorFactory>();
    };
    aggregatorRegistry[AggregatorType::ClippedVoronoi] = [](const RenderConfig&, std::shared_ptr<SamplerFactory>& samplerFactory) {
        return std::make_shared<ClippedVoronoiAggregatorFactory>();
    };
    aggregatorRegistry[AggregatorType::FilteringVoronoi] = [](const RenderConfig&, std::shared_ptr<SamplerFactory>& samplerFactory) {
        return std::make_shared<FilteringVoronoiAggregatorFactory>();
    };
    aggregatorRegistry[AggregatorType::NicoVoronoi] = [](const RenderConfig&, std::shared_ptr<SamplerFactory>& samplerFactory) {
        return std::make_shared<NicoVoronoiAggregatorFactory>();
    };

    cameraRegistry[CameraType::Parallel] = [](const RenderConfig& cfg) {
        auto camera = std::make_shared<ForwardParallelCamera>();
        finalize_camera(cfg, camera);
        // camera->scattering_strategy = std::make_shared<SimpleRayEvaluator>(camera->samplingStrategy);
        camera->ray_evaluator = createEvaluator(cfg, camera->scattering_strategy);
        return camera;
    };
    cameraRegistry[CameraType::RL] = [](const RenderConfig& cfg)
    {
        auto camera = std::make_shared<RLCamera>(cfg.warmup_phases, cfg.exploitation_phases, cfg.voxel_grid_resolution);
        camera->record_warmup = cfg.record_warmup;
        finalize_camera(cfg, camera);
        return camera;
    };
    cameraRegistry[CameraType::Nested] = [](const RenderConfig& cfg) {
        auto camera = std::make_shared<ForwardParallelCamera>();
        finalize_camera(cfg, camera);

        camera->ray_evaluator = std::make_shared<NestedRayEvaluator>(
            camera->scattering_strategy,
            make_shared<SimpleRayEvaluator>(camera->scattering_strategy),
            cfg.nested_sample_size
        );

        return camera;
    };
    cameraRegistry[CameraType::Nested4] = [](const RenderConfig& cfg) {
        auto camera = std::make_shared<ForwardParallelCamera>();
        finalize_camera(cfg, camera);

        camera->ray_evaluator = std::make_shared<NestedRayEvaluator>(
            camera->scattering_strategy,
            make_shared<NestedRayEvaluator>(
                camera->scattering_strategy,
                std::make_shared<NestedRayEvaluator>(
                    camera->scattering_strategy,
                    std::make_shared<NestedRayEvaluator>(
                        camera->scattering_strategy,
                        make_shared<SimpleRayEvaluator>(camera->scattering_strategy),
                        cfg.nested_sample_size),
                    cfg.nested_sample_size),
                cfg.nested_sample_size
            ),
            cfg.nested_sample_size
        );

        return camera;
    };
    cameraRegistry[CameraType::ClippedVoronoiNested] = [](const RenderConfig& cfg) {
        auto camera = std::make_shared<ForwardParallelCamera>();
        finalize_camera(cfg, camera);

        camera->ray_evaluator = std::make_shared<CVorNestedRayEvaluator>(
            camera->scattering_strategy,
            make_shared<SimpleRayEvaluator>(camera->scattering_strategy),
            cfg.nested_sample_size
        );
        return camera;
    };
    cameraRegistry[CameraType::Pixel] = [](const RenderConfig& cfg) {
        auto camera = std::make_shared<CartographyCamera>(cfg.pixelCoords.first, cfg.pixelCoords.second);
        finalize_camera(cfg, camera);
        camera->ray_evaluator = std::make_shared<SimpleRayEvaluator>(camera->scattering_strategy);
        return camera;
    };
    cameraRegistry[CameraType::Single] = [](const RenderConfig& cfg) {
        auto camera = std::make_shared<SinglePixelCamera>(cfg.pixelCoords.first, cfg.pixelCoords.second);
        finalize_camera(cfg, camera);
        camera->ray_evaluator = std::make_shared<SimpleRayEvaluator>(camera->scattering_strategy);
        return camera;
    };

    evaluatorRegistry[EvaluatorType::Nested] = [](const RenderConfig &cfg, const shared_ptr<ScatteringStrategy>& sampling_strategy) {
        return std::make_shared<NestedRayEvaluator>(
            sampling_strategy,
            make_shared<SimpleRayEvaluator>(sampling_strategy),
            cfg.nested_sample_size
        );
    };
    evaluatorRegistry[EvaluatorType::ClippedVoronoiNested] = [](const RenderConfig &cfg, const shared_ptr<ScatteringStrategy>& sampling_strategy) {
        return std::make_shared<CVorNestedRayEvaluator>(
            sampling_strategy,
            make_shared<SimpleRayEvaluator>(sampling_strategy),
            cfg.nested_sample_size
        );
    };
    evaluatorRegistry[EvaluatorType::ClippedVoronoiNestedDebug] = [](const RenderConfig &cfg, const shared_ptr<ScatteringStrategy>& sampling_strategy) {
        return std::make_shared<CVorNestedRayEvaluator>(
            sampling_strategy,
            make_shared<SimpleRayEvaluator>(sampling_strategy),
            cfg.nested_sample_size
        );
    };
    evaluatorRegistry[EvaluatorType::Normals] = [](const RenderConfig &, const shared_ptr<ScatteringStrategy>&) {
        return std::make_shared<NormalRayEvaluator>();
    };
    evaluatorRegistry[EvaluatorType::Standard] = [](const RenderConfig &, const shared_ptr<ScatteringStrategy>& sampling_strategy) {
        return std::make_shared<SimpleRayEvaluator>(sampling_strategy);
    };

    samplingStrategyRegistry[SamplingStrategyType::MixturePDF] = [](const RenderConfig &) {
        return std::make_shared<MixtureScatteringStrategy>();
    };
    samplingStrategyRegistry[SamplingStrategyType::NextEventEstimation] = [](const RenderConfig &) {
        return std::make_shared<NEEScatteringStrategy>();
    };

    sceneRegistry[SceneFormatType::YAPT] = [](const RenderConfig& cfg) {
        const auto camera = createCamera(cfg);
        YaptSceneLoader loader;
        const auto geometry = std::make_shared<HittableList>();
        const auto lights = std::make_shared<HittableList>();
        loader.load(cfg.sourcePath, geometry, lights, camera);

        return std::make_shared<ContentDescription>(
            Scene(*geometry, *lights, Color(0, 0, 0)),
            camera
        );
    };
}

std::shared_ptr<SamplerFactory> RenderFactory::createSampler(const RenderConfig &cfg) {
    try {
        return samplerRegistry[cfg.sampler](cfg);
    } catch (const std::bad_function_call& e) {
        std::cerr << "Cannot instantiate Sampler " << e.what() << std::endl;
        std::exit(1);
    }
}

std::shared_ptr<AggregatorFactory> RenderFactory::createAggregator(const RenderConfig &cfg, std::shared_ptr<SamplerFactory> &samplerFactory) {
    try {
        return aggregatorRegistry[cfg.aggregator](cfg, samplerFactory);
    } catch (const std::bad_function_call& e) {
        std::cerr << "Cannot instantiate Aggregator " << e.what() << std::endl;
        std::exit(1);
    }
}

std::shared_ptr<Camera> RenderFactory::createCamera(const RenderConfig &cfg) {
    try {
        return cameraRegistry[cfg.camera](cfg);
    } catch (const std::bad_function_call& e) {
        std::cerr << "Cannot instantiate Camera " << e.what() << std::endl;
        std::exit(1);
    }
}

std::shared_ptr<ContentDescription> RenderFactory::createContent(const RenderConfig &cfg) {
    try {
        return sceneRegistry[cfg.sceneFormatType](cfg);
    } catch (const std::bad_function_call& e) {
        std::cerr << "Cannot instantiate Content Description from " << cfg.sourcePath << std::endl;
        std::exit(1);
    }
}

std::shared_ptr<RayEvaluator> RenderFactory::createEvaluator(const RenderConfig &cfg, const std::shared_ptr<ScatteringStrategy> &sampling_strategy) {
    try {
        return evaluatorRegistry[cfg.evaluatorType](cfg, sampling_strategy);
    } catch (const std::bad_function_call& e) {
        std::cerr << "Cannot instantiate Aggregator " << e.what() << std::endl;
        std::exit(1);
    }
}

void display_help(const RenderConfig& config) {
    std::cout << "usage: yapt path=out/pic.png spp=1000 sampler=sppp aggregator=vor" << std::endl
              << " - path       => path to render output (optional)" << std::endl
              << " - spp        => samples per pixel (DEFAULT=500)" << std::endl
              << " - sampler    => pixel sampling method:" << std::endl
              << "                 - rnd    => pure random sampling" << std::endl
              << "                 - strat  => stratified sampling" << std::endl
              << "                 - sppp   => Skewed Poisson Point Process sampling with margin (DEFAULT)" << std::endl
              << " - aggregator => path aggregation method:" << std::endl
              << "                 - mc      => Monte Carlo integration" << std::endl
              << "                 - vor     => Voronoi aggregation (DEFAULT)" << std::endl
              << "                 - cvor    => Clipped Voronoi aggregation" << std::endl
              << "                 - fvor    => Filtering Voronoi aggregation" << std::endl
              << "                 - nvor    => Nico Voronoi aggregation" << std::endl
              << " - confidence => Voronoi aggregation confidence (DEFAULT=.999)" << std::endl
              << " - source     => Scene model to import" << std::endl
              << " - maxdepth   => maximum path depth (DEFAULT=25)" << std::endl
              << " - dir        => output directory (optional, ignored if path is specified)" << std::endl
              << " - threads    => number of threads used (DEFAULT=hardware_concurrency)" << std::endl
              << " - width      => force image width (DEFAULT=scene dependent)" << std::endl
              << " - cam        => camera type" << std::endl
              << "                 - std          => standard camera type (DEFAULT) " << std::endl
              << "                 - nest         => MC Nesting (DEPRECATED)" << std::endl
              << "                 - nest4        => MC Nesting (depth 4, DEPRECATED)" << std::endl
              << "                 - cvnest       => Clipped Voronoi Nesting (DEPRECATED)" << std::endl
              << "                 - normals      => renders normals to surfaces " << std::endl
              << "                 - one-x,y      => renders only one pixel @coords (x,y)" << std::endl
              << " - eval       => ray evaluation method" << std::endl
              << "                 - nest         => MC Nesting" << std::endl
              << "                 - cvnest       => Clipped Voronoi Nesting" << std::endl
              << " - seed       => RNG seed (DEFAULT = random seed)" << std::endl
              << " - nee        => Next Event Estimation (DEFAULT = false)" << std::endl
              << " - nestsamples => sample count for first bounce voronoi cameras (DEFAULT = 100)" << std::endl
              << " cam=rl parameters:" << std::endl
              << "                 - vox          => voxel slices (DEFAULT = 64)" << std::endl
              << "                 - warmup       => warmup phases count (DEFAULT = 4)" << std::endl
              << "                 - exploit      => exploitation phases count (DEFAULT = 4)" << std::endl
              << "                 - recordwarmup => record warmup contribution (DEFAULT = true)" << std::endl;
}

CommandLineParser::CommandLineParser() {
    registerHandlers();
}

RenderConfig CommandLineParser::parse(int argc, char* argv[]) {
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

void CommandLineParser::registerHandlers() {
    handlers["maxdepth="] = [](const std::string& v, RenderConfig& c) { c.maxDepth = std::stoi(v);};
    handlers["spp="] = [](const std::string& v, RenderConfig& c) { c.spp = std::stoi(v);};
    handlers["width="] = [](const std::string& v, RenderConfig& c) { c.width = std::stoi(v);};
    handlers["source="] = [](const std::string& v, RenderConfig& c) { c.sourcePath = v;};
    handlers["path="] = [](const std::string& v, RenderConfig& c) {c.outputPath = v; };
    handlers["seed="] = [](const std::string& v, RenderConfig& c) { c.seed = std::stoi(v); };
    handlers["nestsamples="] = [](const std::string& v, RenderConfig& c) { c.nested_sample_size = std::stoi(v); };
    handlers["threads="] = [](const std::string& v, RenderConfig& c) { c.numThreads = std::stoi(v); };
    handlers["nee="] = [](const std::string& v, RenderConfig& c) { c.sampling_strategy_type = (v=="true") ? SamplingStrategyType::NextEventEstimation : SamplingStrategyType::MixturePDF; };
    handlers["help"] = [](const std::string&, RenderConfig& c) { c.help = true; };
    handlers["vox="] = [](const std::string& v, RenderConfig& c) { c.voxel_grid_resolution = std::stoi(v); };
    handlers["exploit="] = [](const std::string& v, RenderConfig& c) { c.exploitation_phases = std::stoi(v); };
    handlers["warmup="] = [](const std::string& v, RenderConfig& c) { c.warmup_phases = std::stoi(v); };
    handlers["recordwarmup="] = [](const std::string& v, RenderConfig& c) { c.record_warmup = v=="true"; };

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
        else if (v == "rl")
        {
            c.camera = CameraType::RL;
        }
        else if (v == "nest") {
            c.evaluatorType = EvaluatorType::Nested;
            c.camera = CameraType::Parallel;
        }
        else if (v == "nest4") {
            c.camera = CameraType::Nested4;
        }
        else if (v == "cvnest") {
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

OutputManager::OutputManager(const RenderConfig& config)  {
    initialize(config);
}

void OutputManager::initialize(const RenderConfig& config) {
    sampler_descriptions[SamplerType::Uniform] = "rnd";
    sampler_descriptions[SamplerType::Stratified] = "strat";
    sampler_descriptions[SamplerType::SkewedPPP] = "sppp";

    aggregator_descriptions[AggregatorType::MonteCarlo] = "mc";
    aggregator_descriptions[AggregatorType::Voronoi] = "vor";
    aggregator_descriptions[AggregatorType::FilteringVoronoi] = "mc";
    aggregator_descriptions[AggregatorType::NicoVoronoi] = "nvor";
    aggregator_descriptions[AggregatorType::ClippedVoronoi] = "cvor";

    camera_descriptions[CameraType::Parallel] = "";
    camera_descriptions[CameraType::RL] = "rl";
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

void OutputManager::start_timer() {
    start = std::chrono::system_clock::now();
}

void OutputManager::stop_timer() {
    end = std::chrono::high_resolution_clock::now();
    render_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
}

bool OutputManager::export_image(RenderConfig& config, const std::shared_ptr<Camera>& camera) {
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
        } else if (config.camera == CameraType::RL) {
            cam_tag += "-" + camera_descriptions[config.camera];
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

