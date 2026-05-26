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
#include <optional>
#include <vector>

#include "pixelEvaluator.h"
#include "pngSaver.h"

namespace
{

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

}  // namespace

PixelEvaluator::PixelEvaluator(const TestConfig::Pixel& config) :
    channelThreshold(config.channelThreshold),
    surfaceDiffRatioThreshold(config.surfaceDiffRatioThreshold),
    backgroundRatio(config.backgroundRatio)
{
}

const char* PixelEvaluator::name() const
{
    return "pixel";
}

const std::vector<TestResult::Metric>& PixelEvaluator::metrics() const
{
    static const std::vector<TestResult::Metric> metrics = {
        {"weightedSurfaceDiffRatio", "weighted surface diff ratio"},
        {"weightedFullImageDiffRatio", "weighted full image diff ratio"},
        {"surfacePixels", "surface pixels"}
    };
    return metrics;
}

size_t PixelEvaluator::primaryMetric() const
{
    return 0;
}

ImageDiff PixelEvaluator::evaluate(const char* reference, const char* testFile)
{
    width = 0;
    height = 0;

    PngImage ref;
    PngImage test;
    if (!loadRGBA(reference, &ref) || !loadRGBA(testFile, &test)) return {};
    if (ref.w != test.w || ref.h != test.h || ref.w == 0 || ref.h == 0) return {};

    const auto refBackground = _mostVisibleColor(ref, backgroundRatio);
    const auto testBackground = _mostVisibleColor(test, backgroundRatio);
    const auto background = (refBackground && testBackground && *refBackground == *testBackground) ? refBackground : std::optional<uint32_t>();

    width = ref.w;
    height = ref.h;
    diff.assign(static_cast<size_t>(width) * height * 4, 0);

    uint64_t surfacePixels = 0;
    double weightedDiffSum = 0.0;
    const auto pixelCount = static_cast<size_t>(width) * height;
    for (size_t i = 0; i < pixelCount; ++i) {
        const auto offset = i * 4;
        const auto refPixel = ref.pixels.data() + offset;
        const auto testPixel = test.pixels.data() + offset;
        uint32_t distance = 0;

        // Ignore invisible transparent pixels so their RGBA payload does not affect comparison.
        const auto transparent = refPixel[3] == 0 && testPixel[3] == 0;
        const auto backgroundPixel = background && _rgba(refPixel) == *background && _rgba(testPixel) == *background;
        if (!transparent && !backgroundPixel) {
            // Measure surface pixels with RGBA Chebyshev distance: the largest absolute channel delta.
            distance = _rgbaChebyshevDistance(refPixel, testPixel);
            ++surfacePixels;
            if(distance > channelThreshold) weightedDiffSum += static_cast<double>(distance) / 255.0;
        }

        // Store the distance in the red channel of the diff image for visualization.
        auto diffPixel = diff.data() + offset;
        diffPixel[0] = static_cast<uint8_t>(distance);
        diffPixel[3] = 255;
    }
    const auto ratio = surfacePixels == 0 ? 0.0f : static_cast<float>(weightedDiffSum / surfacePixels);
    const auto fullRatio = static_cast<float>(weightedDiffSum / pixelCount);

    ImageDiff imageDiff;
    imageDiff.ok = true;
    imageDiff.different = ratio >= surfaceDiffRatioThreshold;
    imageDiff.metricValues = {
        ratio,
        fullRatio,
        static_cast<float>(surfacePixels)
    };
    return imageDiff;
}

bool PixelEvaluator::saveResult(const char* filename) const
{
    if (diff.empty()) return false;
    return saveRGBA(filename, diff.data(), width, height);
}
