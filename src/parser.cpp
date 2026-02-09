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
    camera->samplerAggregator = createAggregator(cfg, sampler_factory);
    camera->pixelSamplerFactory = sampler_factory;
    camera->imageWidth = cfg.width;

    camera->aspect_ratio   = 1.0;
    camera->background     = Color(0, 0, 0);
    camera->vfov           = 40;
    camera->lookFrom       = Point3(278, 278, -800);
    camera->lookAt         = Point3(278, 278, 0);
    camera->vup            = Vec3(0, 1, 0);
    camera->defocusAngle   = 0;
    camera->seed           = cfg.seed;
    camera->samplingStrategy = samplingStrategyRegistry[cfg.sampling_strategy_type](cfg);;
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
        camera->scattering_strategy = std::make_shared<SimpleRayEvaluator>(camera->samplingStrategy);
        return camera;
    };
    cameraRegistry[CameraType::Nested] = [](const RenderConfig& cfg) {
        auto camera = std::make_shared<ForwardParallelCamera>();
        finalize_camera(cfg, camera);

        camera->scattering_strategy = std::make_shared<NestedRayEvaluator>(
            camera->samplingStrategy,
            make_shared<SimpleRayEvaluator>(camera->samplingStrategy),
            cfg.nested_sample_size
        );

        return camera;
    };
    cameraRegistry[CameraType::Nested4] = [](const RenderConfig& cfg) {
        auto camera = std::make_shared<ForwardParallelCamera>();
        finalize_camera(cfg, camera);

        camera->scattering_strategy = std::make_shared<NestedRayEvaluator>(
            camera->samplingStrategy,
            make_shared<NestedRayEvaluator>(
                camera->samplingStrategy,
                std::make_shared<NestedRayEvaluator>(
                    camera->samplingStrategy,
                    std::make_shared<NestedRayEvaluator>(
                        camera->samplingStrategy,
                        make_shared<SimpleRayEvaluator>(camera->samplingStrategy),
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

        camera->scattering_strategy = std::make_shared<CVorNestedRayEvaluator>(
            camera->samplingStrategy,
            make_shared<SimpleRayEvaluator>(camera->samplingStrategy),
            cfg.nested_sample_size
        );
        return camera;
    };
    cameraRegistry[CameraType::Pixel] = [](const RenderConfig& cfg) {
        auto camera = std::make_shared<CartographyCamera>(cfg.pixelCoords.first, cfg.pixelCoords.second);
        finalize_camera(cfg, camera);
        camera->scattering_strategy = std::make_shared<SimpleRayEvaluator>(camera->samplingStrategy);
        return camera;
    };
    cameraRegistry[CameraType::Single] = [](const RenderConfig& cfg) {
        auto camera = std::make_shared<SinglePixelCamera>(cfg.pixelCoords.first, cfg.pixelCoords.second);
        finalize_camera(cfg, camera);
        camera->scattering_strategy = std::make_shared<SimpleRayEvaluator>(camera->samplingStrategy);
        return camera;
    };

    samplingStrategyRegistry[SamplingStrategyType::MixturePDF] = [](const RenderConfig &) {
        return std::make_shared<MixtureSamplingStrategy>();
    };
    samplingStrategyRegistry[SamplingStrategyType::NextEventEstimation] = [](const RenderConfig &) {
        return std::make_shared<NEESamplingStrategy>();
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



