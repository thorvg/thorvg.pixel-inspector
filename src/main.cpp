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
    std::printf("  --artifacts <dir>              artifacts directory (default: ARTIFACTS_DIR)\n");
    std::printf("  --max-width <px>               PNG fit cell width (default: %u)\n", DEFAULT_MAX_WIDTH);
    std::printf("  --max-channel-distance-threshold <value>  Max-channel distance threshold (default: %u)\n", DEFAULT_THRESHOLD_MAX_CHANNEL_DISTANCE);
    std::printf("  --effective-diff-ratio-threshold <value>  Effective difference ratio threshold (default: %.3g)\n", DEFAULT_THRESHOLD_EFFECTIVE_DIFF_RATIO);
    std::printf("  --outlier-distance-threshold <value>  Outlier distance threshold (default: %u)\n", DEFAULT_PIXEL_OUTLIER_DISTANCE_THRESHOLD);
    std::printf("  --outlier-ratio-threshold <value>  Clustered outlier ratio threshold (default: %.3g)\n", DEFAULT_PIXEL_OUTLIER_RATIO_THRESHOLD);
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
        } else if (_next(argc, argv, &i, "--artifacts", &value)) {
            if (*value == '\0') return false;
            config->artifactsDir = value;
        } else if (_next(argc, argv, &i, "--max-width", &value)) {
            if (!_uint32(value, &config->maxWidth)) return false;
            if (config->maxWidth == 0) return false;
        } else if (_next(argc, argv, &i, "--max-channel-distance-threshold", &value) ||
                   _next(argc, argv, &i, "--max_channel_distance_threshold", &value)) {
            if (!_uint32(value, &config->threshold.maxChannelDistance)) return false;
            if (config->threshold.maxChannelDistance > 255) return false;
        } else if (_next(argc, argv, &i, "--effective-diff-ratio-threshold", &value) ||
                   _next(argc, argv, &i, "--effective_diff_ratio_threshold", &value)) {
            if (!_float(value, &config->threshold.effectiveDiffRatio)) return false;
            if (config->threshold.effectiveDiffRatio > 1.0f) return false;
        } else if (_next(argc, argv, &i, "--outlier-distance-threshold", &value) ||
                   _next(argc, argv, &i, "--outlier_distance_threshold", &value)) {
            if (!_uint32(value, &config->threshold.outlierDistance)) return false;
            if (config->threshold.outlierDistance > 255) return false;
        } else if (_next(argc, argv, &i, "--outlier-ratio-threshold", &value) || _next(argc, argv, &i, "--outlier_ratio_threshold", &value)) {
            if (!_float(value, &config->threshold.outlierRatio)) return false;
            if (config->threshold.outlierRatio > 1.0f) return false;
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
    const auto passed = Runner(config).run();
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    LOG("MAIN", "Elapsed: %.3f seconds", elapsed);

    return passed ? 0 : 1;
}
