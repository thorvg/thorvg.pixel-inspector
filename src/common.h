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
#define DEFAULT_ARTIFACTS_DIR ARTIFACTS_DIR
#define DEFAULT_BACKENDS_TEXT "gl,wg,sw"
#define DEFAULT_MAX_WIDTH 200
#define DEFAULT_THRESHOLD_MAX_CHANNEL_DISTANCE 12
#define DEFAULT_THRESHOLD_EFFECTIVE_DIFF_RATIO 0.03f
#define DEFAULT_PIXEL_BACKGROUND_RATIO 0.20f

#define DEFAULT_PIXEL_OUTLIER_DISTANCE_THRESHOLD 125
#define DEFAULT_PIXEL_OUTLIER_RATIO_THRESHOLD 0.80f
#define DEFAULT_OUTLIER_GATE_SCALE 0.5f


struct TestConfig
{
    struct Threshold
    {
        uint32_t maxChannelDistance = DEFAULT_THRESHOLD_MAX_CHANNEL_DISTANCE;       // per-pixel: defines "different"
        uint32_t outlierDistance = DEFAULT_PIXEL_OUTLIER_DISTANCE_THRESHOLD;        // per-pixel: defines "outlier"
        float effectiveDiffRatio = DEFAULT_THRESHOLD_EFFECTIVE_DIFF_RATIO;          // per-image: pass/fail
        float outlierRatio = DEFAULT_PIXEL_OUTLIER_RATIO_THRESHOLD;                 // per-image: pass/fail
    };

    std::string resourceTargetDir = DEFAULT_RESOURCE_TARGET_DIR;
    std::string artifactsDir = DEFAULT_ARTIFACTS_DIR;
    std::vector<std::string> backends;
    uint32_t maxWidth = DEFAULT_MAX_WIDTH;
    Threshold threshold;
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
    std::vector<BackendResult> backends;
};

#endif
