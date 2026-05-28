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

#ifndef _TVG_PIXEL_INSPECTOR_ENGINE_H_
#define _TVG_PIXEL_INSPECTOR_ENGINE_H_

#include <cstdint>

#include <thorvg.h>

struct TestEngine;

struct TestCanvas
{
    explicit TestCanvas(const char* engineType = "sw", uint32_t w = 100, uint32_t h = 100, tvg::ColorSpace cs = tvg::ColorSpace::ABGR8888S);

    TestCanvas(const TestCanvas&) = delete;
    TestCanvas& operator=(const TestCanvas&) = delete;

    ~TestCanvas();

    bool resize(uint32_t w, uint32_t h);
    bool clear();
    bool render();
    const uint8_t* buffer();
    tvg::Canvas* ptr();

    uint32_t width = 0;
    uint32_t height = 0;

private:
    tvg::Canvas* canvas = nullptr;
    TestEngine* engine = nullptr;
};

#endif
