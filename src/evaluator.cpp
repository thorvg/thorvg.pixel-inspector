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
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <sstream>
#include <utility>
#include <vector>

#include "evaluator.h"
#include "lodepng.h"

namespace
{

constexpr auto EvaluatorName = "pixel";

std::string _metricLog(const std::vector<TestResult::Metric>& schema, const std::vector<float>& values)
{
    std::ostringstream stream;
    for (size_t i = 0; i < schema.size() && i < values.size(); ++i) {
        if (i > 0) stream << ' ';
        stream << schema[i].key << '=' << values[i];
    }
    return stream.str();
}

struct PngImage
{
    uint32_t w = 0;
    uint32_t h = 0;
    std::vector<uint8_t> pixels;
};

bool _loadRGBA(const char* filename, PngImage* image)
{
    uint8_t* buffer = nullptr;
    unsigned w = 0;
    unsigned h = 0;
    if (lodepng_decode32_file(&buffer, &w, &h, filename) != 0) return false;

    image->w = w;
    image->h = h;
    image->pixels.assign(buffer, buffer + static_cast<size_t>(w) * h * 4);

    std::free(buffer);
    return true;
}

uint32_t _rgba(const uint8_t* pixel)
{
    return (static_cast<uint32_t>(pixel[0]) << 24) |
           (static_cast<uint32_t>(pixel[1]) << 16) |
           (static_cast<uint32_t>(pixel[2]) << 8) |
           static_cast<uint32_t>(pixel[3]);
}

uint32_t _rgbaChebyshevDistance(const uint8_t* a, const uint8_t* b)
{
    const auto r = std::abs(static_cast<int>(a[0]) - static_cast<int>(b[0]));
    const auto g = std::abs(static_cast<int>(a[1]) - static_cast<int>(b[1]));
    const auto bdiff = std::abs(static_cast<int>(a[2]) - static_cast<int>(b[2]));
    const auto alpha = std::abs(static_cast<int>(a[3]) - static_cast<int>(b[3]));
    return static_cast<uint32_t>(std::max({r, g, bdiff, alpha}));
}

std::optional<uint32_t> _mostVisibleColor(const PngImage& image, float ratio)
{
    std::vector<uint32_t> colors;
    colors.reserve(static_cast<size_t>(image.w) * image.h);

    // Collect non-transparent pixels
    for (size_t i = 0, n = static_cast<size_t>(image.w) * image.h; i < n; ++i) {
        const auto pixel = image.pixels.data() + i * 4;
        if (pixel[3] == 0) continue;
        colors.push_back(_rgba(pixel));
    }
    if (colors.empty()) return std::nullopt;

    // Find the most common visible color and accept it as background only if it covers the required ratio.
    std::sort(colors.begin(), colors.end());

    uint32_t mostColor = colors[0];
    size_t mostCount = 0;
    for (auto it = colors.begin(); it != colors.end();) {
        const auto upper = std::upper_bound(it, colors.end(), *it);
        const auto count = static_cast<size_t>(upper - it);
        if (count > mostCount) {
            mostColor = *it;
            mostCount = count;
        }
        it = upper;
    }

    const auto required = static_cast<size_t>(std::max(1.0f, std::ceil(colors.size() * ratio)));
    return mostCount >= required ? std::optional<uint32_t>(mostColor) : std::nullopt;
}

}

Evaluator::~Evaluator()
{
    finish();
}

Evaluator::Evaluator(const TestConfig& config)
{
    result.config = config;
    result.metrics = metrics();
    for (const auto& backend : config.backends)
        result.backends.push_back({backend});
    worker = std::thread(&Evaluator::run, this);
}

void Evaluator::push(EvaluationTask&& task)
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        tasks.push(std::move(task));
    }
    condition.notify_one();
}

TestResult Evaluator::sync()
{
    finish();
    return std::move(result);
}

void Evaluator::finish()
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
    }
    condition.notify_one();
    if (worker.joinable()) worker.join();
}

void Evaluator::run()
{
    while (true) {
        EvaluationTask task;
        {
            std::unique_lock<std::mutex> lock(mutex);
            condition.wait(lock, [this]() { return done || !tasks.empty(); });
            if (tasks.empty()) {
                if (done) break;
                continue;
            }
            task = std::move(tasks.front());
            tasks.pop();
        }

        auto backendResult = std::find_if(result.backends.begin(), result.backends.end(), [&task](const auto& backend) { return backend.name == task.backend; });

        // Failed Cases: missing render
        if (!task.rendered) {
            ++backendResult->summary.failed;
            backendResult->failed.push_back(task.relative);
            LOGERR("EVALUATOR", "Failed to render: %s", task.asset.c_str());
            continue;
        }

        // Failed Cases: missing comparison target
        if (!std::filesystem::exists(task.reference) || !std::filesystem::exists(task.test)) {
            ++backendResult->summary.failed;
            backendResult->failed.push_back(task.relative);
            LOGERR("EVALUATOR", "Missing comparison target: %s", task.asset.c_str());
            continue;
        }

        // Compare and save diff
        auto imageDiff = evaluate(task.reference.c_str(), task.test.c_str());
        if (!imageDiff.ok) {
            ++backendResult->summary.failed;
            backendResult->failed.push_back(task.relative);
            LOGERR("EVALUATOR", "Failed to compare: %s", task.asset.c_str());
            continue;
        }
        ++backendResult->summary.compared;

        if (imageDiff.different) {
            ++backendResult->summary.different;
        }

        if (!saveResult(task.diff.c_str())) {
            ++backendResult->summary.failed;
            LOGERR("EVALUATOR", "Failed to create diff image: %s", task.asset.c_str());
            continue;
        }

        LOG("EVALUATOR", "%s evaluator=%s %s", task.relative.c_str(), EvaluatorName, _metricLog(result.metrics, imageDiff.metricValues).c_str());
        backendResult->comparisons.push_back({task.relative, task.reference, task.test, task.diff, std::move(imageDiff.metricValues), imageDiff.different});
    }

    // Log summary
    for (const auto& backendResult : result.backends) {
        LOG("EVALUATOR", "%s %s: compared=%u different=%u failed=%u",
            EvaluatorName, backendResult.name.c_str(), backendResult.summary.compared, backendResult.summary.different, backendResult.summary.failed);
    }
}

const std::vector<TestResult::Metric>& Evaluator::metrics() const
{
    // The first metric is the primary report metric used for thresholding and sorting.
    static const std::vector<TestResult::Metric> metrics = {
        {"effectiveDiffRatio", "Effective Diff Ratio"},
        {"clusteredOutlierRatio", "Clustered Outlier Ratio"}
    };
    return metrics;
}

Evaluator::ImageDiff Evaluator::evaluate(const char* reference, const char* testFile)
{
    width = 0;
    height = 0;

    PngImage ref;
    PngImage test;
    if (!_loadRGBA(reference, &ref) || !_loadRGBA(testFile, &test)) return {};
    if (ref.w != test.w || ref.h != test.h || ref.w == 0 || ref.h == 0) return {};

    const auto refBackground = _mostVisibleColor(ref, DEFAULT_PIXEL_BACKGROUND_RATIO);
    const auto testBackground = _mostVisibleColor(test, DEFAULT_PIXEL_BACKGROUND_RATIO);
    std::optional<uint32_t> background;
    if (refBackground && testBackground && *refBackground == *testBackground) background = *refBackground;

    width = ref.w;
    height = ref.h;
    diff.assign(static_cast<size_t>(width) * height * 4, 0);

    uint64_t effectivePixels = 0;
    double weightedDiffSum = 0.0;
    const auto pixelCount = static_cast<size_t>(width) * height;
    double outlierPixels = 0.0;
    const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    for (size_t i = 0; i < pixelCount; ++i) {
        const auto offset = i * 4;
        const auto refPixel = ref.pixels.data() + offset;
        const auto testPixel = test.pixels.data() + offset;
        uint32_t distance = 0;

        // Ignore invisible transparent pixels so their RGBA payload does not affect comparison.
        const auto transparent = refPixel[3] == 0 && testPixel[3] == 0;
        const auto backgroundPixel = background && _rgba(refPixel) == *background && _rgba(testPixel) == *background;
        if (!transparent && !backgroundPixel) {
            // Measure effective pixels with RGBA Chebyshev distance: the largest absolute channel delta.
            distance = _rgbaChebyshevDistance(refPixel, testPixel);
            ++effectivePixels;
            if (distance > result.config.threshold.maxChannelDistance) weightedDiffSum += static_cast<double>(distance) / 255.0;
            if (distance > result.config.threshold.outlierDistance) {
                auto x = i % width;
                auto y = i / width;
                int outlierCnt = 0;
                for (size_t j = 0; j < 8; ++j) {
                    auto nx = static_cast<int>(x) + dx[j];
                    auto ny = static_cast<int>(y) + dy[j];
                    if (nx < 0 || nx >= static_cast<int>(width) || ny < 0 || ny >= static_cast<int>(height)) continue;
                    auto neighborOffset = (static_cast<size_t>(ny) * width + nx) * 4;
                    auto neighborRefPixel = ref.pixels.data() + neighborOffset;
                    auto neighborTestPixel = test.pixels.data() + neighborOffset;
                    auto neighborDistance = _rgbaChebyshevDistance(neighborRefPixel, neighborTestPixel);
                    if (neighborDistance > result.config.threshold.outlierDistance) {
                        outlierCnt++;
                    }
                }
                if (outlierCnt >= 2)
                    outlierPixels += static_cast<double>(distance) / 255.0;
            }
        }

        // Store the distance in the red channel of the diff image for visualization.
        auto diffPixel = diff.data() + offset;
        diffPixel[0] = static_cast<uint8_t>(distance);
        diffPixel[3] = 255;
    }
    const auto effectiveDiffRatio = effectivePixels == 0 ? 0.0f : static_cast<float>(weightedDiffSum / effectivePixels);
    const auto outlierRatio =  (weightedDiffSum == 0 ? 0.0f : static_cast<float>(outlierPixels / weightedDiffSum));
    const int minDiffPixels = static_cast<int>(std::sqrt(effectivePixels) * DEFAULT_OUTLIER_GATE_SCALE);

    return {
        true,
        effectiveDiffRatio >= result.config.threshold.effectiveDiffRatio 
          || (outlierRatio > result.config.threshold.outlierRatio && minDiffPixels < weightedDiffSum),
        {effectiveDiffRatio, outlierRatio}
    };
}

bool Evaluator::saveResult(const char* filename) const
{
    if (diff.empty()) return false;

    std::error_code error;
    const auto parent = std::filesystem::path(filename).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, error);
        if (error) return false;
    }

    return lodepng_encode32_file(filename, diff.data(), width, height) == 0;
}
