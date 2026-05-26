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
#include <filesystem>
#include <fstream>

#include "mdSaver.h"
#include "jsonSaver.h"
#include "common.h"

static std::string _link(const std::filesystem::path& from, const std::filesystem::path& to)
{
    return to.lexically_relative(from.parent_path()).string();
}

static float _metricValue(const TestResult::Comparison& comparison, size_t index)
{
    return index < comparison.metricValues.size() ? comparison.metricValues[index] : 0.0f;
}

static const char* _reportTitle(EvaluatorType evaluatorType)
{
    switch (evaluatorType) {
        case EvaluatorType::Pixel: {
            return "ThorVG Pixel Test Pixel Report";
        }
        case EvaluatorType::Flip: {
            return "ThorVG Pixel Test FLIP Report";
        }
    }
    return "ThorVG Pixel Test FLIP Report";
}

bool MdSaver::save(const TestResult& result, const std::string& reportDir)
{
    std::error_code error;
    std::filesystem::create_directories(reportDir, error);
    if (error) return false;

    auto reportPath = std::filesystem::path(reportDir) / "reporter.md";
    std::ofstream report(reportPath);
    if (!report.is_open()) return false;
    const auto primaryMetric = result.primaryMetric;

    report << "# " << _reportTitle(result.config.evaluatorType) << "\n\n";
    report << "- Resource: `" << result.config.resourceTargetDir << "`\n";
    report << "- Reference: `" << result.config.referenceDir << "`\n";
    report << "- Test: `" << result.config.testDir << "`\n";
    report << "- Evaluator: `" << evaluatorName(result.config.evaluatorType) << "`\n";
    switch (result.config.evaluatorType) {
        case EvaluatorType::Pixel: {
            report << "- Pixel channel threshold: `" << result.config.pixel.channelThreshold << "`\n";
            report << "- Pixel surface diff ratio threshold: `" << result.config.pixel.surfaceDiffRatioThreshold << "`\n";
            report << "- Pixel background ratio: `" << result.config.pixel.backgroundRatio << "`\n";
            break;
        }
        case EvaluatorType::Flip: {
            report << "- FLIP surface mean threshold: `" << result.config.flip.surfaceMeanThreshold << "`\n";
            report << "- FLIP surface error floor threshold: `" << result.config.flip.surfaceErrorFloorThreshold << "`\n";
            break;
        }
    }
    report << "\n";

    for (const auto& backend : result.backends) {
        report << "## " << backend.name << "\n\n";
        report << "| compared | different | failed |\n";
        report << "| ---: | ---: | ---: |\n";
        report << "| " << backend.summary.compared
               << " | " << backend.summary.different
               << " | " << backend.summary.failed << " |\n\n";
        report << "<details>\n";
        report << "<summary>Comparisons (" << backend.comparisons.size() << ")</summary>\n\n";
        report << "| status | asset | reference | test | diff";
        for (const auto& metric : result.metrics) report << " | " << metric.label;
        report << " |\n";
        report << "| --- | --- | --- | --- | ---";
        for (size_t i = 0; i < result.metrics.size(); ++i) report << " | ---:";
        report << " |\n";
        auto comparisons = backend.comparisons;
        std::sort(comparisons.begin(), comparisons.end(), [primaryMetric](const auto& a, const auto& b) {
            return _metricValue(a, primaryMetric) > _metricValue(b, primaryMetric);
        });
        for (const auto& comparison : comparisons) {
            report << "| " << (comparison.different ? "different" : "pass")
                   << " | `" << comparison.asset
                   << "` | ![](" << _link(reportPath, comparison.reference)
                   << ") | ![](" << _link(reportPath, comparison.test)
                   << ") | ![](" << _link(reportPath, comparison.diff)
                   << ")";
            for (size_t i = 0; i < result.metrics.size(); ++i) report << " | " << _metricValue(comparison, i);
            report << " |\n";
        }
        report << "\n</details>\n";
        if (!backend.failed.empty()) {
            report << "\nFailed:\n";
            for (const auto& failure : backend.failed) report << "- `" << failure << "`\n";
        }
        report << "\n";
    }

    return saveReportJson(result, reportDir);
}
