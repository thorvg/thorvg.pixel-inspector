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
#include <cerrno>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include "common.h"
#include "runner.h"

static bool _equal(const char* a, const char* b)
{
    return std::strcmp(a, b) == 0;
}

static bool _read(const char* arg, const char* name, const char** value)
{
    auto len = std::strlen(name);
    if (std::strncmp(arg, name, len) != 0 || arg[len] != '=') return false;
    *value = arg + len + 1;
    return true;
}

static bool _next(int argc, char** argv, int* i, const char* name, const char** value)
{
    if (_read(argv[*i], name, value)) return true;
    if (!_equal(argv[*i], name)) return false;
    if (*i + 1 >= argc) return false;
    *value = argv[++(*i)];
    return true;
}

static bool _support(const char* backend)
{
    return _equal(backend, "sw") || _equal(backend, "gl") || _equal(backend, "wg");
}

static bool _format(const char* format)
{
    return _equal(format, DEFAULT_REPORT_FORMAT) || _equal(format, "md");
}

static bool _evaluatorType(const char* value, EvaluatorType* evaluatorType)
{
    if (_equal(value, "flip")) {
        *evaluatorType = EvaluatorType::Flip;
        return true;
    }
    if (_equal(value, "pixel")) {
        *evaluatorType = EvaluatorType::Pixel;
        return true;
    }
    return false;
}

static std::string _trim(const char* begin, const char* end)
{
    while (begin < end && std::isspace(static_cast<unsigned char>(*begin))) ++begin;
    while (begin < end && std::isspace(static_cast<unsigned char>(*(end - 1)))) --end;
    return std::string(begin, end - begin);
}

static bool _append(std::vector<std::string>* backends, const char* value)
{
    if (*value == '\0') return false;

    const char* begin = value;
    while (true) {
        auto end = begin;
        while (*end != '\0' && *end != ',') ++end;

        auto backend = _trim(begin, end);
        if (backend.empty() || !_support(backend.c_str())) return false;
        if (std::find(backends->begin(), backends->end(), backend) == backends->end()) {
            backends->push_back(backend);
        }

        if (*end == '\0') break;
        begin = (*end == ',') ? end + 1 : end;
    }

    return !backends->empty();
}

static void _normalizeBackends(std::vector<std::string>* backends)
{
    // Keep SW last because SwCanvas::target() updates ThorVG's global ImageLoader color space.
    auto sw = std::find(backends->begin(), backends->end(), "sw");
    if (sw == backends->end()) return;

    backends->erase(sw);
    backends->push_back("sw");
}

static bool _uint32(const char* value, uint32_t* result)
{
    char* end = nullptr;
    errno = 0;
    auto ret = std::strtoul(value, &end, 10);
    if (*value == '\0' || *end != '\0' || errno != 0 || ret > std::numeric_limits<uint32_t>::max()) return false;
    *result = static_cast<uint32_t>(ret);
    return true;
}

static bool _float(const char* value, float* result)
{
    char* end = nullptr;
    errno = 0;
    auto ret = std::strtof(value, &end);
    if (*value == '\0' || *end != '\0' || errno != 0 || !std::isfinite(ret) || ret < 0.0f) return false;
    *result = ret;
    return true;
}

static void _help(const char* name)
{
    std::printf("Usage: %s [options]\n", name);
    std::printf("\n");
    std::printf("Options:\n");
    std::printf("  --backend <sw|gl|wg>[,...]     render backend list (default: %s)\n", DEFAULT_BACKENDS_TEXT);
    std::printf("  --resource <dir>               resource directory (default: TARGET_RESOURCE_DIR)\n");
    std::printf("  --reference <dir>              reference directory (default: REFERENCE_DIR)\n");
    std::printf("  --test <dir>                   test directory (default: TEST_DIR)\n");
    std::printf("  --report <dir>                 report directory (default: REPORT_DIR)\n");
    std::printf("  --report-format <html|md>      report format (default: %s)\n", DEFAULT_REPORT_FORMAT);
    std::printf("  --evaluator <flip|pixel>       image evaluator strategy (default: %s)\n", DEFAULT_EVALUATOR_TEXT);
    std::printf("  --max-width <px>               PNG fit cell width (default: %u)\n", DEFAULT_MAX_WIDTH);
    std::printf("  --flip-surface-mean-threshold <value>         Alpha-surface FLIP mean threshold (default: %.3g)\n", DEFAULT_FLIP_SURFACE_MEAN_THRESHOLD);
    std::printf("  --flip-surface-error-floor-threshold <value>  Ignore low FLIP error contribution threshold (default: %.3g)\n", DEFAULT_FLIP_SURFACE_ERROR_FLOOR_THRESHOLD);
    std::printf("  --pixel-channel-threshold <value>        RGBA Chebyshev distance threshold (default: %u)\n", DEFAULT_PIXEL_CHANNEL_THRESHOLD);
    std::printf("  --pixel-surface-diff-ratio-threshold <value>  Weighted surface diff ratio threshold (default: %.3g)\n", DEFAULT_PIXEL_SURFACE_DIFF_RATIO_THRESHOLD);
    std::printf("  --pixel-background-ratio <value>        Background color detection ratio (default: %.3g)\n", DEFAULT_PIXEL_BACKGROUND_RATIO);
    std::printf("  --update-reference             update references\n");
    std::printf("  --help                         print this message\n");
}

static bool _parse(int argc, char** argv, TestConfig* config, bool* done)
{
    for (auto i = 1; i < argc; ++i) {
        const char* value = nullptr;

        if (_next(argc, argv, &i, "--backend", &value)) {
            if (!_append(&config->backends, value)) return false;
        } else if (_next(argc, argv, &i, "--resource", &value)) {
            if (*value == '\0') return false;
            config->resourceTargetDir = value;
        } else if (_next(argc, argv, &i, "--reference", &value)) {
            if (*value == '\0') return false;
            config->referenceDir = value;
        } else if (_next(argc, argv, &i, "--test", &value)) {
            if (*value == '\0') return false;
            config->testDir = value;
        } else if (_next(argc, argv, &i, "--report", &value)) {
            if (*value == '\0') return false;
            config->reportDir = value;
        } else if (_next(argc, argv, &i, "--report-format", &value)) {
            if (!_format(value)) return false;
            config->reportFormat = value;
        } else if (_next(argc, argv, &i, "--evaluator", &value)) {
            if (!_evaluatorType(value, &config->evaluatorType)) return false;
        } else if (_next(argc, argv, &i, "--max-width", &value)) {
            if (!_uint32(value, &config->maxWidth)) return false;
            if (config->maxWidth == 0) return false;
        } else if (_next(argc, argv, &i, "--pixel-channel-threshold", &value) || _next(argc, argv, &i, "--pixel_channel_threshold", &value)) {
            if (!_uint32(value, &config->pixel.channelThreshold)) return false;
            if (config->pixel.channelThreshold > 255) return false;
        } else if (_next(argc, argv, &i, "--pixel-surface-diff-ratio-threshold", &value) || _next(argc, argv, &i, "--pixel_surface_diff_ratio_threshold", &value)) {
            if (!_float(value, &config->pixel.surfaceDiffRatioThreshold)) return false;
            if (config->pixel.surfaceDiffRatioThreshold > 1.0f) return false;
        } else if (_next(argc, argv, &i, "--pixel-background-ratio", &value) || _next(argc, argv, &i, "--pixel_background_ratio", &value)) {
            if (!_float(value, &config->pixel.backgroundRatio)) return false;
            if (config->pixel.backgroundRatio > 1.0f) return false;
        } else if (_next(argc, argv, &i, "--flip-surface-mean-threshold", &value) || _next(argc, argv, &i, "--flip_surface_mean_threshold", &value)
                   || _next(argc, argv, &i, "--surface-mean-threshold", &value) || _next(argc, argv, &i, "--surface_mean_threshold", &value)) {
            if (!_float(value, &config->flip.surfaceMeanThreshold)) return false;
        } else if (_next(argc, argv, &i, "--flip-surface-error-floor-threshold", &value) || _next(argc, argv, &i, "--flip_surface_error_floor_threshold", &value)
                   || _next(argc, argv, &i, "--surface-error-floor-threshold", &value) || _next(argc, argv, &i, "--surface_error_floor_threshold", &value)) {
            if (!_float(value, &config->flip.surfaceErrorFloorThreshold)) return false;
        } else if (_equal(argv[i], "--update-reference")) {
            config->updateReference = true;
        } else if (_equal(argv[i], "--help") || _equal(argv[i], "-h")) {
            _help(argv[0]);
            *done = true;
            return false;
        } else return false;
    }

    if (config->backends.empty()) config->backends = {"gl", "wg", "sw"};
    else _normalizeBackends(&config->backends);
    return true;
}

int main(int argc, char** argv)
{
    TestConfig config;
    auto done = false;
    if (!_parse(argc, argv, &config, &done)) {
        if (done) return 0;
        std::fprintf(stderr, "Invalid arguments. Try --help.\n");
        return 1;
    }

    const auto start = std::chrono::steady_clock::now();
    Runner(config).run();
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    LOG("MAIN", "Elapsed: %.3f seconds", elapsed);

    return 0;
}
