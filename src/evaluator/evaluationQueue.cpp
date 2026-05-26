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
#include <sstream>
#include <utility>

#include "evaluationQueue.h"

namespace
{

std::string _metricLog(const std::vector<TestResult::Metric>& schema, const std::vector<float>& values)
{
    std::ostringstream stream;
    for (size_t i = 0; i < schema.size() && i < values.size(); ++i) {
        if (i > 0) stream << ' ';
        stream << schema[i].key << '=' << values[i];
    }
    return stream.str();
}

}

EvaluationQueue::~EvaluationQueue()
{
    finish();
}

EvaluationQueue::EvaluationQueue(const TestConfig& config)
{
    evaluator = makeEvaluator(config);
    result.config = config;
    result.metrics = evaluator->metrics();
    result.primaryMetric = evaluator->primaryMetric();
    for (const auto& backend : config.backends)
        result.backends.push_back({backend});
    worker = std::thread(&EvaluationQueue::run, this);
}

void EvaluationQueue::push(EvaluationTask&& task)
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        tasks.push(std::move(task));
    }
    condition.notify_one();
}

TestResult EvaluationQueue::sync()
{
    finish();
    return std::move(result);
}

void EvaluationQueue::finish()
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
    }
    condition.notify_one();
    if (worker.joinable()) worker.join();
}

void EvaluationQueue::run()
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
        auto imageDiff = evaluator->evaluate(task.reference.c_str(), task.test.c_str());
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

        if (!evaluator->saveResult(task.diff.c_str())) {
            ++backendResult->summary.failed;
            LOGERR("EVALUATOR", "Failed to create diff image: %s", task.asset.c_str());
            continue;
        }

        LOG("EVALUATOR", "%s evaluator=%s %s", task.relative.c_str(), evaluator->name(), _metricLog(result.metrics, imageDiff.metricValues).c_str());
        backendResult->comparisons.push_back({task.relative, task.reference, task.test, task.diff, std::move(imageDiff.metricValues), imageDiff.different});
    }

    // Log summary
    for (const auto& backendResult : result.backends) {
        LOG("EVALUATOR", "%s %s: compared=%u different=%u failed=%u",
            evaluator->name(), backendResult.name.c_str(), backendResult.summary.compared, backendResult.summary.different, backendResult.summary.failed);
    }
}
