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

#include <cstring>
#include <filesystem>

#include <thorvg.h>

#include "runner.h"
#include "engine.h"
#include "evaluator.h"
#include "drawTest.h"
#include "htmlSaver.h"
#include "pngSaver.h"

static std::filesystem::path _path(const std::string& resourceTargetDir, const std::string& artifactsDir, const std::string& backend, const std::string& asset, const char* role)
{
    auto relative = std::filesystem::path(asset).lexically_relative(resourceTargetDir);
    relative.replace_extension("." + backend + "." + role + ".png");
    return std::filesystem::path(artifactsDir) / relative;
}

static std::filesystem::path _drawTestPath(const std::string& artifactsDir, const std::string& backend, const char* name, const char* role)
{
    return std::filesystem::path(artifactsDir) / "draw_test" / (std::string(name) + "." + backend + "." + role + ".png");
}

static void _removeBySuffix(const std::string& artifactsDir, const char* suffix)
{
    std::error_code error;
    const auto root = std::filesystem::path(artifactsDir);
    if (!std::filesystem::exists(root, error)) return;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(root, std::filesystem::directory_options::skip_permission_denied, error)) {
        if (error) { error.clear(); continue; }
        if (!entry.is_regular_file(error)) continue;

        const auto filename = entry.path().filename().string();
        const auto len = std::strlen(suffix);
        if (filename.size() >= len && filename.compare(filename.size() - len, len, suffix) == 0) {
            std::filesystem::remove(entry.path(), error);
            if (error) error.clear();
        }
    }
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

static bool _saveDrawTest(TestCanvas* canvas, tvgdraw::DrawTest* drawTest, const char* filename)
{
    if (!canvas->clear()) return false;
    if (!canvas->resize(drawTest->width, drawTest->height)) return false;
    return drawTest->draw(canvas->ptr()) && PngSaver(drawTest->width).save(canvas, filename);
}

static bool _passed(const TestResult& result)
{
    for (const auto& backend : result.backends) {
        if (backend.summary.different > 0 || backend.summary.failed > 0) return false;
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

    // LOG options
    LOG("RUNNER", "Target resource directory: %s", config.resourceTargetDir.c_str());
    LOG("RUNNER", "Artifacts directory: %s", config.artifactsDir.c_str());
    LOG("RUNNER", "PNG max width: %u", config.maxWidth);
    LOG("RUNNER", "Max-channel distance threshold: %u", config.threshold.maxChannelDistance);
    LOG("RUNNER", "Effective difference ratio threshold: %.6f", config.threshold.effectiveDiffRatio);
    LOG("RUNNER", "Outlier distance threshold: %u", config.threshold.outlierDistance);
    LOG("RUNNER", "Outlier ratio threshold: %.6f", config.threshold.outlierRatio);
    LOG("RUNNER", "Assets: %zu", assets.size());
    LOG("RUNNER", "Draw tests: %zu", tvgdraw::DrawTestRegistry::entries().size());

    if (config.updateReference) LOG("RUNNER", "Update reference mode enabled.");
}

bool Runner::run()
{
    auto savePngAndEval = [this](const std::string& backend, TestCanvas* canvas, Evaluator* evaluatorQueue) {
        LOG("RUNNER", "Backend: %s", backend.c_str());
        PngSaver saver(config.maxWidth);
        for (const auto& asset : assets) {
            auto target = _path(config.resourceTargetDir, config.artifactsDir, backend, asset, evaluatorQueue ? "test" : "reference");
            const auto rendered = saver.save(canvas, asset.c_str(), target.string().c_str());
            if (!rendered) LOGERR("RUNNER", "Failed: %s", asset.c_str());

            if (!evaluatorQueue) continue;

            auto reference = _path(config.resourceTargetDir, config.artifactsDir, backend, asset, "reference");
            auto diff = _path(config.resourceTargetDir, config.artifactsDir, backend, asset, "diff");
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
    };

    auto saveDrawTestsAndEval = [this](const std::string& backend, TestCanvas* canvas, Evaluator* evaluatorQueue) {
        const auto& drawTests = tvgdraw::DrawTestRegistry::entries();
        if (drawTests.empty()) return;

        LOG("RUNNER", "Draw test backend: %s", backend.c_str());
        for (const auto& entry : drawTests) {
            auto drawTest = entry.factory();
            const auto target = _drawTestPath(config.artifactsDir, backend, entry.name, evaluatorQueue ? "test" : "reference");
            const auto rendered = drawTest && _saveDrawTest(canvas, drawTest.get(), target.string().c_str());
            if (!rendered) LOGERR("RUNNER", "Failed draw test: %s", entry.name);

            if (!evaluatorQueue) continue;

            const auto reference = _drawTestPath(config.artifactsDir, backend, entry.name, "reference");
            const auto diff = _drawTestPath(config.artifactsDir, backend, entry.name, "diff");
            const auto relative = (std::filesystem::path("draw_test") / entry.name).string();
            evaluatorQueue->push({
                    backend,
                    relative,
                    relative,
                    reference.string(),
                    target.string(),
                    diff.string(),
                    rendered
            });
        }
    };

    auto saveBackendAndEval = [&savePngAndEval, &saveDrawTestsAndEval](const std::string& backend, Evaluator* evaluatorQueue) {
        TestCanvas canvas(backend.c_str());
        if (!canvas.ptr()) {
            LOGERR("RUNNER", "Skipping backend: %s", backend.c_str());
            return;
        }
        _loadFonts();
        savePngAndEval(backend, &canvas, evaluatorQueue);
        saveDrawTestsAndEval(backend, &canvas, evaluatorQueue);
    };

    auto saveReport = [this](const TestResult& result) {
        const auto reportPath = std::filesystem::path(config.artifactsDir) / "reporter.html";
        const auto saved = HtmlSaver().save(result, config.artifactsDir);
        if (saved) {
            LOG("RUNNER", "Report: %s", reportPath.string().c_str());
        } else {
            LOGERR("RUNNER", "Failed to create report: %s", config.artifactsDir.c_str());
        }
        return saved;
    };

    if (config.updateReference) {
        LOG("RUNNER", "Updating references...");
        _removeBySuffix(config.artifactsDir, ".reference.png");
        _removeBySuffix(config.artifactsDir, ".test.png");
        _removeBySuffix(config.artifactsDir, ".diff.png");
        std::error_code error;
        std::filesystem::remove(std::filesystem::path(config.artifactsDir) / "reporter.html", error);
        for (const auto& backend : config.backends) saveBackendAndEval(backend, nullptr);
        LOG("RUNNER", "References updated.");
        return true;
    }

    LOG("RUNNER", "Starting tests...");
    std::error_code error;
    _removeBySuffix(config.artifactsDir, ".test.png");
    _removeBySuffix(config.artifactsDir, ".diff.png");
    std::filesystem::remove(std::filesystem::path(config.artifactsDir) / "reporter.html", error);

    Evaluator evaluatorQueue(config);
    for (const auto& backend : config.backends) saveBackendAndEval(backend, &evaluatorQueue);
    auto result = evaluatorQueue.sync();
    const auto reportSaved = saveReport(result);
    LOG("RUNNER", "Tests completed.");
    return reportSaved && _passed(result);
}
