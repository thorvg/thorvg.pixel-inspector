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
#include <filesystem>
#include <utility>

#include <thorvg.h>

#include "runner.h"
#include "engine.h"
#include "evaluationQueue.h"
#include "htmlSaver.h"
#include "mdSaver.h"
#include "pngSaver.h"

static std::filesystem::path _path(const std::string& resourceTargetDir, const std::string& outputDir, const std::string& backend, const std::string& asset)
{
    return std::filesystem::path(outputDir) / backend / std::filesystem::path(asset)
                .lexically_relative(resourceTargetDir)
                .replace_extension(".png");
}

static void _loadFonts()
{
    auto fontDir = std::filesystem::path(RESOURCE_DIR) / "font";
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(fontDir, error)) {
        auto ext = entry.path().extension();
        if (!entry.is_regular_file(error) || (ext != ".ttf" && ext != ".otf")) continue;
        if (tvg::Text::load(entry.path().string().c_str()) != tvg::Result::Success) {
            LOGERR("RUNNER", "Failed to load font: %s", entry.path().string().c_str());
        }
    }
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

    // LOG options
    LOG("RUNNER", "Target resource directory: %s", config.resourceTargetDir.c_str());
    LOG("RUNNER", "Reference directory: %s", config.referenceDir.c_str());
    LOG("RUNNER", "Test directory: %s", config.testDir.c_str());
    LOG("RUNNER", "Report directory: %s", config.reportDir.c_str());
    LOG("RUNNER", "Report format: %s", config.reportFormat.c_str());
    LOG("RUNNER", "Evaluator: %s", evaluatorName(config.evaluatorType));
    LOG("RUNNER", "PNG max width: %u", config.maxWidth);
    switch (config.evaluatorType) {
        case EvaluatorType::Pixel: {
            LOG("RUNNER", "Pixel channel threshold: %u", config.pixel.channelThreshold);
            LOG("RUNNER", "Pixel surface diff ratio threshold: %.6f", config.pixel.surfaceDiffRatioThreshold);
            LOG("RUNNER", "Pixel background ratio: %.6f", config.pixel.backgroundRatio);
            break;
        }
        case EvaluatorType::Flip: {
            LOG("RUNNER", "FLIP surface mean threshold: %.6f", config.flip.surfaceMeanThreshold);
            LOG("RUNNER", "FLIP surface error floor threshold: %.6f", config.flip.surfaceErrorFloorThreshold);
            break;
        }
    }
    LOG("RUNNER", "Assets: %zu", assets.size());

    if (config.updateReference) LOG("RUNNER", "Update reference mode enabled.");
}

void Runner::run()
{
    auto savePngAndEval = [this](EvaluationQueue* evaluatorQueue) {
        for (const auto& backend : config.backends) {
            LOG("RUNNER", "Backend: %s", backend.c_str());
            TestCanvas canvas(backend.c_str());
            if (!canvas.ptr()) {
                LOGERR("RUNNER", "Skipping backend: %s", backend.c_str());
                continue;
            }
            _loadFonts();
            PngSaver saver(config.maxWidth);
            for (const auto& asset : assets) {
                auto target = _path(config.resourceTargetDir, config.testDir, backend, asset);
                const auto rendered = saver.save(&canvas, asset.c_str(), target.string().c_str());
                if (!rendered) LOGERR("RUNNER", "Failed: %s", asset.c_str());

                if (!evaluatorQueue) continue;

                auto reference = _path(config.resourceTargetDir, config.referenceDir, backend, asset);
                auto diff = _path(config.resourceTargetDir, (std::filesystem::path(config.reportDir) / "diff").string(), backend, asset);
                evaluatorQueue->push({
                        backend,
                        asset,
                        std::filesystem::path(asset).lexically_relative(config.resourceTargetDir).string(),
                        reference.string(),
                        target.string(),
                        diff.string(),
                        rendered
                });
            }
        }
    };

    auto saveReport = [this](const TestResult& result) {
        const auto reportPath = std::filesystem::path(config.reportDir) / (config.reportFormat == "md" ? "reporter.md" : "reporter.html");
        const auto saved = config.reportFormat == "md" ? MdSaver().save(result, config.reportDir) : HtmlSaver().save(result, config.reportDir);
        if (saved) {
            LOG("RUNNER", "Report: %s", reportPath.string().c_str());
        } else {
            LOGERR("RUNNER", "Failed to create report: %s", config.reportDir.c_str());
        }
    };

    if (config.updateReference) {
        LOG("RUNNER", "Updating references...");
        std::error_code error;
        std::filesystem::remove_all(config.referenceDir, error);
        if (error) {
            LOGERR("RUNNER", "Failed to remove reference directory: %s", config.referenceDir.c_str());
            return;
        }
        config.testDir = config.referenceDir;
        savePngAndEval(nullptr);
        LOG("RUNNER", "References updated.");
        return;
    }

    LOG("RUNNER", "Starting tests...");
    std::error_code error;
    std::filesystem::remove_all(std::filesystem::path(config.reportDir) / "diff", error);

    EvaluationQueue evaluatorQueue(config);
    savePngAndEval(&evaluatorQueue);
    saveReport(evaluatorQueue.sync());
    LOG("RUNNER", "Tests completed.");
}
