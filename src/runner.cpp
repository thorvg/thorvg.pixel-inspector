/*
 * Copyright (c) 2026 ThorVG project. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <utility>

#include <thorvg.h>

#include "runner.h"
#include "csvSaver.h"
#include "engine.h"
#include "evaluator.h"
#include "exampleTest.h"
#include "htmlSaver.h"
#include "mdSaver.h"
#include "pngSaver.h"

static std::filesystem::path _path(const std::string& resourceTargetDir, const std::string& outputDir, const std::string& backend, const std::string& asset, const char* role)
{
    auto relative = std::filesystem::path(asset).lexically_relative(resourceTargetDir);
    relative.replace_extension("." + backend + "." + role + ".png");
    return std::filesystem::path(outputDir) / relative;
}

static std::filesystem::path _examplePath(const std::string& outputDir, const std::string& backend, const char* name, const char* role)
{
    return std::filesystem::path(outputDir) / "example" / (std::string(name) + "." + backend + "." + role + ".png");
}

static void _loadFonts()
{
    auto fontDir = std::filesystem::path(EXAMPLE_DIR) / "font";
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(fontDir, error)) {
        auto ext = entry.path().extension();
        if (!entry.is_regular_file(error) || (ext != ".ttf" && ext != ".otf")) continue;
        if (tvg::Text::load(entry.path().string().c_str()) != tvg::Result::Success) {
            LOGERR("RUNNER", "Failed to load font: %s", entry.path().string().c_str());
        }
    }
}

static bool _passed(const TestResult& result)
{
    for (const auto& backend : result.backends) {
        if (backend.summary.differences > 0 || backend.summary.errors > 0) return false;
    }
    return true;
}

Runner::Runner(const TestConfig& config) : config(config)
{
    std::error_code error;
    // Collect assets 
    for (const auto& entry : std::filesystem::recursive_directory_iterator(config.resourceTargetDir, std::filesystem::directory_options::skip_permission_denied, error)) {
        if (error) { error.clear(); continue; }
        auto ext = entry.path().extension();
        if (entry.is_regular_file(error) && (ext == ".json" || ext == ".svg")) assets.push_back(entry.path().string());
    }
    if (config.shardCount > 1) {
        std::sort(assets.begin(), assets.end());
        std::vector<std::string> shard;
        shard.reserve((assets.size() + config.shardCount - 1) / config.shardCount);
        size_t lottieIndex = 0;
        size_t svgIndex = 0;
        for (auto& asset : assets) {
            auto& index = std::filesystem::path(asset).extension() == ".json" ? lottieIndex : svgIndex;
            if (index++ % config.shardCount == config.shardIndex) shard.push_back(std::move(asset));
        }
        assets = std::move(shard);
    }

    // LOG options
    LOG("RUNNER", "Target resource directory: %s", config.resourceTargetDir.c_str());
    LOG("RUNNER", "Output directory: %s", config.outputDir.c_str());
    LOG("RUNNER", "PNG max width: %u", config.maxWidth);
    LOG("RUNNER", "Max-channel distance threshold: %u", config.threshold.maxChannelDistance);
    LOG("RUNNER", "Diff ratio threshold: %.6f", config.threshold.diffRatio);
    LOG("RUNNER", "Assets: %zu", assets.size());

    if (config.updateGolden) LOG("RUNNER", "Update golden mode enabled.");
}

bool Runner::run()
{
    auto examples = tvgexample::ExampleTestRegistry::entries();
    if (config.shardCount > 1) {
        std::sort(examples.begin(), examples.end(), [](const auto& a, const auto& b) { return std::strcmp(a.name, b.name) < 0; });
        std::vector<tvgexample::ExampleTestEntry> shard;
        shard.reserve((examples.size() + config.shardCount - 1) / config.shardCount);
        for (size_t i = config.shardIndex; i < examples.size(); i += config.shardCount) shard.push_back(examples[i]);
        examples = std::move(shard);
    }
    LOG("RUNNER", "Example tests: %zu", config.examples ? examples.size() : 0);

    auto savePngAndEval = [this](const std::string& backend, TestCanvas* canvas, Evaluator* evaluatorQueue) {
        LOG("RUNNER", "Backend: %s", backend.c_str());
        auto saver = PngSaver(config.maxWidth);
        for (const auto& asset : assets) {
            auto target = _path(config.resourceTargetDir, config.outputDir, backend, asset, evaluatorQueue ? "actual" : "golden");
            LOG("RUNNER", "Started: %s", asset.c_str());
            auto rendered = false;
            if (canvas) {
                if (backend == "wg" && !canvas->recreate()) {
                    LOGERR("RUNNER", "Failed to recreate asset canvas: %s", backend.c_str());
                } else {
                    rendered = saver.save(canvas, asset.c_str(), target.string().c_str());
                }
            } else {
                LOGERR("RUNNER", "Invalid asset canvas: %s", backend.c_str());
            }
            if (!rendered) LOGERR("RUNNER", "Failed: %s", asset.c_str());

            if (!evaluatorQueue) continue;

            auto golden = _path(config.resourceTargetDir, config.outputDir, backend, asset, "golden");
            auto diff = _path(config.resourceTargetDir, config.outputDir, backend, asset, "diff");
            evaluatorQueue->push({
                    backend,
                    asset,
                    std::filesystem::path(asset).lexically_relative(config.resourceTargetDir).string(),
                    golden.string(),
                    target.string(),
                    diff.string(),
                    rendered
            });
        }
    };

    auto saveExamplesAndEval = [this, &examples](const std::string& backend, TestCanvas* canvas, Evaluator* evaluatorQueue) {
        if (!config.examples) return;
        if (examples.empty()) return;

        LOG("RUNNER", "Example test backend: %s", backend.c_str());
        for (const auto& entry : examples) {
            const auto target = _examplePath(config.outputDir, backend, entry.name, evaluatorQueue ? "actual" : "golden");
            auto rendered = false;
            if (canvas && canvas->recreate()) {
                rendered = PngSaver(entry.width).save(canvas, entry, target.string().c_str());
            } else {
                LOGERR("RUNNER", "Failed to recreate example test canvas: %s", backend.c_str());
            }
            if (!rendered) LOGERR("RUNNER", "Failed example test: %s", entry.name);

            if (!evaluatorQueue) continue;

            const auto golden = _examplePath(config.outputDir, backend, entry.name, "golden");
            const auto diff = _examplePath(config.outputDir, backend, entry.name, "diff");
            const auto relative = (std::filesystem::path("example") / entry.name).string();
            evaluatorQueue->push({
                    backend,
                    relative,
                    relative,
                    golden.string(),
                    target.string(),
                    diff.string(),
                    rendered
            });
        }
    };

    auto saveBackendAndEval = [&savePngAndEval, &saveExamplesAndEval](const std::string& backend, Evaluator* evaluatorQueue) {
        auto colorSpace = backend == "wg" ? tvg::ColorSpace::ABGR8888 : tvg::ColorSpace::ABGR8888S;
        TestCanvas canvas(backend.c_str(), colorSpace);
        if (!canvas.ptr()) {
            LOGERR("RUNNER", "Skipping backend: %s", backend.c_str());
            return;
        }
        _loadFonts();
        savePngAndEval(backend, &canvas, evaluatorQueue);
        saveExamplesAndEval(backend, &canvas, evaluatorQueue);
    };

    auto saveReport = [this](const TestResult& result) {
        const auto htmlPath = std::filesystem::path(config.outputDir) / "reporter.html";
        const auto htmlSaved = HtmlSaver().save(result, config.outputDir);
        if (htmlSaved) {
            LOG("RUNNER", "Report: %s", htmlPath.string().c_str());
        } else {
            LOGERR("RUNNER", "Failed to create report: %s", config.outputDir.c_str());
        }

        const auto mdPath = std::filesystem::path(config.outputDir) / "reporter.md";
        const auto mdSaved = MdSaver().save(result, config.outputDir);
        if (mdSaved) {
            LOG("RUNNER", "Summary: %s", mdPath.string().c_str());
        } else {
            LOGERR("RUNNER", "Failed to create summary: %s", config.outputDir.c_str());
        }

        const auto csvPath = std::filesystem::path(config.outputDir) / "reporter.csv";
        const auto csvSaved = CsvSaver().save(result, config.outputDir);
        if (csvSaved) {
            LOG("RUNNER", "Summary data: %s", csvPath.string().c_str());
        } else {
            LOGERR("RUNNER", "Failed to create summary data: %s", config.outputDir.c_str());
        }
        return htmlSaved && mdSaved && csvSaved;
    };

    if (config.updateGolden) {
        LOG("RUNNER", "Updating golden images...");
        std::error_code error;
        std::filesystem::remove(std::filesystem::path(config.outputDir) / "reporter.html", error);
        std::filesystem::remove(std::filesystem::path(config.outputDir) / "reporter.md", error);
        std::filesystem::remove(std::filesystem::path(config.outputDir) / "reporter.csv", error);
        for (const auto& backend : config.backends) saveBackendAndEval(backend, nullptr);
        LOG("RUNNER", "Golden images updated.");
        return true;
    }

    LOG("RUNNER", "Starting tests...");
    std::error_code error;
    std::filesystem::remove(std::filesystem::path(config.outputDir) / "reporter.html", error);
    std::filesystem::remove(std::filesystem::path(config.outputDir) / "reporter.md", error);
    std::filesystem::remove(std::filesystem::path(config.outputDir) / "reporter.csv", error);

    auto expected = static_cast<uint32_t>(assets.size() + (config.examples ? examples.size() : 0));
    Evaluator evaluatorQueue(config, expected);
    for (const auto& backend : config.backends) saveBackendAndEval(backend, &evaluatorQueue);
    auto result = evaluatorQueue.sync();
    const auto reportSaved = saveReport(result);
    LOG("RUNNER", "Tests completed.");
    return reportSaved && _passed(result);
}
