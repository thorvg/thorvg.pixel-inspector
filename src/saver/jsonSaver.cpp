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

#include <filesystem>
#include <fstream>

#include "jsonSaver.h"
#include "common.h"

namespace
{

std::string _json(const std::string& text)
{
    std::string ret;
    for (auto c : text) {
        if (c == '\\') ret += "\\\\";
        else if (c == '"') ret += "\\\"";
        else if (c == '\n') ret += "\\n";
        else if (c == '\r') ret += "\\r";
        else if (c == '\t') ret += "\\t";
        else ret += c;
    }
    return ret;
}

std::string _link(const std::filesystem::path& from, const std::filesystem::path& to)
{
    return to.lexically_relative(from.parent_path()).string();
}

}

bool saveReportJson(const TestResult& result, const std::string& reportDir)
{
    std::error_code error;
    std::filesystem::create_directories(reportDir, error);
    if (error) return false;

    const auto dataPath = std::filesystem::path(reportDir) / "data.json";
    std::ofstream data(dataPath);
    if (!data.is_open()) return false;

    TestResult::Summary total;
    for (const auto& backend : result.backends) {
        total.compared += backend.summary.compared;
        total.different += backend.summary.different;
        total.failed += backend.summary.failed;
    }

    data << "{";
    data << "\"evaluator\":\"" << _json(evaluatorName(result.config.evaluatorType)) << "\",";
    data << "\"flip\":{";
    data << "\"surfaceMeanThreshold\":" << result.config.flip.surfaceMeanThreshold << ",";
    data << "\"surfaceErrorFloorThreshold\":" << result.config.flip.surfaceErrorFloorThreshold << "},";
    data << "\"pixel\":{";
    data << "\"channelThreshold\":" << result.config.pixel.channelThreshold << ",";
    data << "\"surfaceDiffRatioThreshold\":" << result.config.pixel.surfaceDiffRatioThreshold << ",";
    data << "\"backgroundRatio\":" << result.config.pixel.backgroundRatio << "},";
    data << "\"resource\":\"" << _json(result.config.resourceTargetDir) << "\",";
    data << "\"reference\":\"" << _json(result.config.referenceDir) << "\",";
    data << "\"test\":\"" << _json(result.config.testDir) << "\",";
    data << "\"metrics\":[";
    for (size_t i = 0; i < result.metrics.size(); ++i) {
        const auto& metric = result.metrics[i];
        if (i > 0) data << ",";
        data << "{";
        data << "\"key\":\"" << _json(metric.key) << "\",";
        data << "\"label\":\"" << _json(metric.label) << "\"";
        data << "}";
    }
    data << "],";
    data << "\"primaryMetric\":" << result.primaryMetric << ",";
    data << "\"summary\":{";
    data << "\"compared\":" << total.compared << ",";
    data << "\"different\":" << total.different << ",";
    data << "\"failed\":" << total.failed << "},";
    data << "\"backends\":[";
    for (size_t i = 0; i < result.backends.size(); ++i) {
        const auto& backend = result.backends[i];
        if (i > 0) data << ",";
        data << "{";
        data << "\"name\":\"" << _json(backend.name) << "\",";
        data << "\"summary\":{";
        data << "\"compared\":" << backend.summary.compared << ",";
        data << "\"different\":" << backend.summary.different << ",";
        data << "\"failed\":" << backend.summary.failed << "},";
        data << "\"comparisons\":[";
        for (size_t j = 0; j < backend.comparisons.size(); ++j) {
            const auto& comparison = backend.comparisons[j];
            if (j > 0) data << ",";
            data << "{";
            data << "\"asset\":\"" << _json(comparison.asset) << "\",";
            data << "\"reference\":\"" << _json(_link(dataPath, comparison.reference)) << "\",";
            data << "\"test\":\"" << _json(_link(dataPath, comparison.test)) << "\",";
            data << "\"diff\":\"" << _json(_link(dataPath, comparison.diff)) << "\",";
            data << "\"metrics\":{";
            for (size_t k = 0; k < result.metrics.size() && k < comparison.metricValues.size(); ++k) {
                if (k > 0) data << ",";
                data << "\"" << _json(result.metrics[k].key) << "\":" << comparison.metricValues[k];
            }
            data << "},";
            data << "\"different\":" << (comparison.different ? "true" : "false");
            data << "}";
        }
        data << "],\"failed\":[";
        for (size_t j = 0; j < backend.failed.size(); ++j) {
            if (j > 0) data << ",";
            data << "\"" << _json(backend.failed[j]) << "\"";
        }
        data << "]}";
    }
    data << "]}";

    return true;
}
