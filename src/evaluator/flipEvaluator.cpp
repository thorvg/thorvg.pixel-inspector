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
#include <memory>

#include "flipEvaluator.h"
#include "nv_flip.h"
#include "pngSaver.h"

FlipEvaluator::FlipEvaluator(const TestConfig::Flip& config) :
    surfaceMeanThreshold(config.surfaceMeanThreshold),
    surfaceErrorFloorThreshold(config.surfaceErrorFloorThreshold)
{
}

const char* FlipEvaluator::name() const
{
    return "flip";
}

const std::vector<TestResult::Metric>& FlipEvaluator::metrics() const
{
    static const std::vector<TestResult::Metric> metrics = {
        {"fullMean", "full mean"},
        {"surfaceMean", "surface mean"},
        {"surfaceAdjustedMean", "surface adjusted"},
        {"surfaceCoverage", "surface"}
    };
    return metrics;
}

size_t FlipEvaluator::primaryMetric() const
{
    return 2; // "surfaceAdjustedMean"
}

ImageDiff FlipEvaluator::evaluate(const char* reference, const char* testFile)
{
    width = 0;
    height = 0;

    PngImage ref;
    PngImage test;
    if (!loadRGBA(reference, &ref) || !loadRGBA(testFile, &test)) return {};
    if (ref.w != test.w || ref.h != test.h) return {};

    const auto count = static_cast<size_t>(ref.w) * ref.h;
    const auto refRGB = normalizeRGB(toRGB8(ref.pixels.data(), ref.w, ref.h));
    const auto testRGB = normalizeRGB(toRGB8(test.pixels.data(), test.w, test.h));

    ImageDiff imageDiff;
    FLIP::Parameters params;
    float* rawErrorMap = nullptr;
    float fullMean = 0.0f;
    FLIP::evaluate(refRGB.data(), testRGB.data(), ref.w, ref.h, false, params, false, true, fullMean, &rawErrorMap);
    std::unique_ptr<float[]> errors(rawErrorMap);
    if (!errors) return {};

    auto surfaceMean = 0.0f;
    auto surfaceAdjustedMean = 0.0f;
    auto surfaceCoverage = 0.0f;
    {
        // Measure FLIP only on visible alpha-surface pixels so transparent padding does not dilute the result.
        // The error floor removes low-level FLIP noise before computing the adjusted mean used for pass/fail.
        double surfaceSum = 0.0;
        double surfaceAdjustedSum = 0.0;
        uint64_t surfacePixels = 0;

        for (size_t i = 0; i < count; ++i) {
            const auto offset = i * 4;

            // Skip fully transparent pixels in both reference and test as they are not visible and can have FLIP noise.
            if (ref.pixels[offset + 3] == 0 && test.pixels[offset + 3] == 0) continue;
            const auto error = errors[i];
            surfaceSum += error;

             // Ignore low-level FLIP noise before computing the adjusted mean used for pass/fail.
            if (error > surfaceErrorFloorThreshold) surfaceAdjustedSum += error;
            ++surfacePixels;
        }
        if (surfacePixels > 0) {
            surfaceMean = static_cast<float>(surfaceSum / surfacePixels);
            surfaceAdjustedMean = static_cast<float>(surfaceAdjustedSum / surfacePixels);
            surfaceCoverage = static_cast<float>(static_cast<double>(surfacePixels) / count);
        }
    }

    diff.assign(count * 4, 0);
    for (size_t i = 0; i < count; ++i) {
        const auto value = static_cast<uint8_t>(std::clamp(errors[i], 0.0f, 1.0f) * 255.0f + 0.5f);
        diff[i * 4 + 0] = value;
        diff[i * 4 + 3] = 255;
    }

    width = ref.w;
    height = ref.h;
    imageDiff.metricValues = {fullMean, surfaceMean, surfaceAdjustedMean, surfaceCoverage};
    imageDiff.different = surfaceAdjustedMean >= surfaceMeanThreshold;
    imageDiff.ok = true;
    return imageDiff;
}

bool FlipEvaluator::saveResult(const char* filename) const
{
    if (diff.empty()) return false;
    return saveRGBA(filename, diff.data(), width, height);
}
