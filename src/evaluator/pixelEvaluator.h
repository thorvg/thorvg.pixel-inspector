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

#ifndef _TVG_PIXEL_INSPECTOR_PIXEL_EVALUATOR_H_
#define _TVG_PIXEL_INSPECTOR_PIXEL_EVALUATOR_H_

#include <cstdint>
#include <vector>

#include "evaluator.h"

class PixelEvaluator : public Evaluator
{
public:
    explicit PixelEvaluator(const TestConfig::Pixel& config);

    const char* name() const override;
    const std::vector<TestResult::Metric>& metrics() const override;
    size_t primaryMetric() const override;
    ImageDiff evaluate(const char* reference, const char* test) override;
    bool saveResult(const char* filename) const override;

private:
    uint32_t channelThreshold = 0;
    float surfaceDiffRatioThreshold = 0.0f;
    float backgroundRatio = 0.0f;
    std::vector<uint8_t> diff;
    uint32_t width = 0;
    uint32_t height = 0;
};

#endif
