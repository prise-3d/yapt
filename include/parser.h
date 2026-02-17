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

void display_help(const RenderConfig& config);

#include <map>
#include <functional>
#include <memory>
#include <iostream>
#include <cmath>

class CommandLineParser {
    using ArgumentConsumer = std::function<void(const std::string&, RenderConfig&)>;
    std::map<std::string, ArgumentConsumer> handlers;

public:
    CommandLineParser();

    RenderConfig parse(int argc, char* argv[]);

    void registerHandlers();
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

class OutputManager {
protected:
    std::chrono::time_point<std::chrono::system_clock> start;
    std::chrono::time_point<std::chrono::system_clock> end;
    std::chrono::duration<long, std::ratio<1, 1000>> render_time{};

public:
    explicit OutputManager(const RenderConfig& config);
    void initialize(const RenderConfig& config);
    void start_timer();
    void stop_timer();
    bool export_image(RenderConfig& config, const std::shared_ptr<Camera> &camera);

private:
    std::map<SamplerType, std::string> sampler_descriptions;
    std::map<AggregatorType, std::string> aggregator_descriptions;
    std::map<CameraType, std::string> camera_descriptions;
    std::map<EvaluatorType, std::string> evaluator_descriptions;
};

#endif //PARSER_H