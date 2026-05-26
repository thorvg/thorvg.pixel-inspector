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

#ifndef _TVG_PIXEL_INSPECTOR_COMMON_H_
#define _TVG_PIXEL_INSPECTOR_COMMON_H_

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

constexpr auto ErrorColor = "\033[31m";
constexpr auto ErrorBgColor = "\033[41m";
constexpr auto LogColor = "\033[32m";
constexpr auto LogBgColor = "\033[42m";
constexpr auto GreyColor = "\033[90m";
constexpr auto ResetColors = "\033[0m";

#define LOG(tag, fmt, ...) fprintf(stdout, "%s[L]%s %s" tag "%s (%s %d): %s" fmt "\n", LogBgColor, ResetColors, LogColor, GreyColor, __FILE__, __LINE__, ResetColors, ##__VA_ARGS__)
#define LOGERR(tag, fmt, ...) fprintf(stderr, "%s[E]%s %s" tag "%s (%s %d): %s" fmt "\n", ErrorBgColor, ResetColors, ErrorColor, GreyColor, __FILE__, __LINE__, ResetColors, ##__VA_ARGS__)

#define DEFAULT_RESOURCE_TARGET_DIR TARGET_RESOURCE_DIR
#define DEFAULT_REFERENCE_DIR REFERENCE_DIR
#define DEFAULT_TEST_DIR TEST_DIR
#define DEFAULT_REPORT_DIR REPORT_DIR
#define DEFAULT_REPORT_FORMAT "html"
#define DEFAULT_EVALUATOR_TEXT "pixel"
#define DEFAULT_BACKENDS_TEXT "gl,wg,sw"
#define DEFAULT_MAX_WIDTH 200
#define DEFAULT_FLIP_SURFACE_MEAN_THRESHOLD 0.025f
#define DEFAULT_FLIP_SURFACE_ERROR_FLOOR_THRESHOLD 0.25f
#define DEFAULT_PIXEL_CHANNEL_THRESHOLD 12
#define DEFAULT_PIXEL_SURFACE_DIFF_RATIO_THRESHOLD 0.03f
#define DEFAULT_PIXEL_BACKGROUND_RATIO 0.20f

enum class EvaluatorType
{
    Flip,
    Pixel
};

#define DEFAULT_EVALUATOR EvaluatorType::Pixel

inline const char* evaluatorName(EvaluatorType evaluatorType)
{
    switch (evaluatorType) {
        case EvaluatorType::Pixel: {
            return "pixel";
        }
        case EvaluatorType::Flip: {
            return "flip";
        }
    }
    return "flip";
}

struct TestConfig
{
    struct Flip
    {
        float surfaceMeanThreshold = DEFAULT_FLIP_SURFACE_MEAN_THRESHOLD;
        float surfaceErrorFloorThreshold = DEFAULT_FLIP_SURFACE_ERROR_FLOOR_THRESHOLD;
    };

    struct Pixel
    {
        uint32_t channelThreshold = DEFAULT_PIXEL_CHANNEL_THRESHOLD;
        float surfaceDiffRatioThreshold = DEFAULT_PIXEL_SURFACE_DIFF_RATIO_THRESHOLD;
        float backgroundRatio = DEFAULT_PIXEL_BACKGROUND_RATIO;
    };

    std::string resourceTargetDir = DEFAULT_RESOURCE_TARGET_DIR;
    std::string referenceDir = DEFAULT_REFERENCE_DIR;
    std::string testDir = DEFAULT_TEST_DIR;
    std::string reportDir = DEFAULT_REPORT_DIR;
    std::string reportFormat = DEFAULT_REPORT_FORMAT;
    EvaluatorType evaluatorType = DEFAULT_EVALUATOR;
    std::vector<std::string> backends;
    uint32_t maxWidth = DEFAULT_MAX_WIDTH;
    Flip flip;
    Pixel pixel;
    bool updateReference = false;
};

struct TestResult
{
    struct Metric
    {
        std::string key;
        std::string label;
    };

    struct Summary
    {
        uint32_t compared = 0;
        uint32_t different = 0;
        uint32_t failed = 0;
    };

    struct Comparison
    {
        std::string asset;
        std::string reference;
        std::string test;
        std::string diff;
        std::vector<float> metricValues;
        bool different = false;
    };

    struct BackendResult
    {
        std::string name;
        Summary summary;
        std::vector<Comparison> comparisons;
        std::vector<std::string> failed;
    };

    TestConfig config;
    std::vector<Metric> metrics;
    size_t primaryMetric = 0;
    std::vector<BackendResult> backends;
};

#endif
