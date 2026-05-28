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

#ifndef _TVG_PIXEL_INSPECTOR_EVALUATOR_H_
#define _TVG_PIXEL_INSPECTOR_EVALUATOR_H_

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "common.h"

struct EvaluationTask
{
    std::string backend;
    std::string asset;
    std::string relative;
    std::string reference;
    std::string test;
    std::string diff;
    bool rendered = false;
};

class Evaluator
{
public:
    explicit Evaluator(const TestConfig& config);
    ~Evaluator();

    Evaluator(const Evaluator&) = delete;
    Evaluator& operator=(const Evaluator&) = delete;

    void push(EvaluationTask&& task);
    TestResult sync();

private:
    struct ImageDiff
    {
        bool ok = false;
        bool different = false;
        std::vector<float> metricValues;
    };

    void finish();
    void run();

    const std::vector<TestResult::Metric>& metrics() const;
    ImageDiff evaluate(const char* reference, const char* test);
    bool saveResult(const char* filename) const;

    std::mutex mutex;
    std::condition_variable condition;
    std::queue<EvaluationTask> tasks;
    std::thread worker; // TODO: Use a thread pool for parallel evaluation in the future
    TestResult result;
    bool done = false;

    std::vector<uint8_t> diff;
    uint32_t width = 0;
    uint32_t height = 0;
};

#endif
