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

static std::string _trim(const char* begin, const char* end)
{
    while (begin < end && std::isspace(static_cast<unsigned char>(*begin))) ++begin;
    while (begin < end && std::isspace(static_cast<unsigned char>(*(end - 1)))) --end;
    return std::string(begin, end - begin);
}

static bool _append(std::vector<std::string>* backends, const std::vector<std::string>& supported, const char* value)
{
    if (*value == '\0') return false;

    const char* begin = value;
    while (true) {
        auto end = begin;
        while (*end != '\0' && *end != ',') ++end;

        auto backend = _trim(begin, end);
        if (backend.empty() || std::find(supported.begin(), supported.end(), backend) == supported.end()) return false;
        if (std::find(backends->begin(), backends->end(), backend) == backends->end()) {
            backends->push_back(backend);
        }

        if (*end == '\0') break;
        begin = (*end == ',') ? end + 1 : end;
    }

    return !backends->empty();
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
    std::printf("  --backend <list>               render backend list\n");
    std::printf("  --resource <dir>               resource directory (default: TARGET_RESOURCE_DIR)\n");
    std::printf("  --output <dir>                 output directory (default: OUTPUT_DIR)\n");
    std::printf("  --max-width <px>               PNG fit cell width (default: %u)\n", DEFAULT_MAX_WIDTH);
    std::printf("  --shard-index <index>          zero-based asset shard index (default: 0)\n");
    std::printf("  --shard-count <count>          total asset shard count (default: 1)\n");
    std::printf("  --max-channel-distance-threshold <value>  Max-channel distance threshold (default: %u)\n", DEFAULT_THRESHOLD_MAX_CHANNEL_DISTANCE);
    std::printf("  --diff-ratio-threshold <value>  Diff ratio threshold (default: %.3g)\n", DEFAULT_THRESHOLD_DIFF_RATIO);
    std::printf("  --skip-draw-tests              skip registered C++ draw tests\n");
    std::printf("  --update-golden               update golden images\n");
    std::printf("  --help                         print this message\n");
}

static bool _parse(int argc, char** argv, TestConfig* config, bool* done)
{
    const std::vector<std::string> supported = {
#if defined(TVGTEST_SDL_GL_SUPPORTED) || defined(TVGTEST_GL_SUPPORTED) || defined(TVGTEST_GLES_SUPPORTED)
        "gl",
#endif
#if defined(TVGTEST_WG_SUPPORTED) || defined(TVGTEST_WGPU_SUPPORTED)
        "wg",
#endif
#if defined(TVGTEST_CPU_SUPPORTED)
        "cpu",
#endif
    };

    for (auto i = 1; i < argc; ++i) {
        const char* value = nullptr;

        if (_next(argc, argv, &i, "--backend", &value)) {
            if (!_append(&config->backends, supported, value)) return false;
        } else if (_next(argc, argv, &i, "--resource", &value)) {
            if (*value == '\0') return false;
            config->resourceTargetDir = value;
        } else if (_next(argc, argv, &i, "--output", &value)) {
            if (*value == '\0') return false;
            config->outputDir = value;
        } else if (_next(argc, argv, &i, "--max-width", &value)) {
            if (!_uint32(value, &config->maxWidth)) return false;
            if (config->maxWidth == 0) return false;
        } else if (_next(argc, argv, &i, "--shard-index", &value)) {
            if (!_uint32(value, &config->shardIndex)) return false;
        } else if (_next(argc, argv, &i, "--shard-count", &value)) {
            if (!_uint32(value, &config->shardCount)) return false;
        } else if (_next(argc, argv, &i, "--max-channel-distance-threshold", &value) ||
                   _next(argc, argv, &i, "--max_channel_distance_threshold", &value)) {
            if (!_uint32(value, &config->threshold.maxChannelDistance)) return false;
            if (config->threshold.maxChannelDistance > 255) return false;
        } else if (_next(argc, argv, &i, "--diff-ratio-threshold", &value) ||
                   _next(argc, argv, &i, "--diff_ratio_threshold", &value)) {
            if (!_float(value, &config->threshold.diffRatio)) return false;
            if (config->threshold.diffRatio > 1.0f) return false;
        } else if (_equal(argv[i], "--update-golden")) {
            config->updateGolden = true;
        } else if (_equal(argv[i], "--skip-draw-tests")) {
            config->drawTests = false;
        } else if (_equal(argv[i], "--help") || _equal(argv[i], "-h")) {
            _help(argv[0]);
            *done = true;
            return false;
        } else return false;
    }

    if (config->shardCount == 0 || config->shardIndex >= config->shardCount) return false;
    if (config->backends.empty()) config->backends = supported;
    // Keep CPU last because SwCanvas::target() updates ThorVG's global ImageLoader color space.
    auto cpu = std::find(config->backends.begin(), config->backends.end(), "cpu");
    if (cpu != config->backends.end()) std::rotate(cpu, cpu + 1, config->backends.end());
    return !config->backends.empty();
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

#if defined(TVGTEST_LOG_ENABLED)
    const auto start = std::chrono::steady_clock::now();
#endif
    const auto passed = Runner(config).run();
#if defined(TVGTEST_LOG_ENABLED)
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    LOG("MAIN", "Elapsed: %.3f seconds", elapsed);
#endif

    return passed ? 0 : 1;
}
