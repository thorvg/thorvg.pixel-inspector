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

#ifndef _TVG_PIXEL_INSPECTOR_DRAW_TEST_H_
#define _TVG_PIXEL_INSPECTOR_DRAW_TEST_H_

#include <cstdint>
#include <cstdio>

#include <memory>
#include <vector>
#include <filesystem>
#include <fstream>
#include <istream>
#include <algorithm>

#include "common.h"

#include <thorvg.h>

namespace tvgdraw
{

class DrawTest
{
public:
    DrawTest(uint32_t width, uint32_t height) : width(width), height(height) {}
    virtual ~DrawTest() = default;

    DrawTest(const DrawTest&) = delete;
    DrawTest& operator=(const DrawTest&) = delete;

    virtual bool draw(tvg::Canvas* canvas) = 0;

    const uint32_t width;
    const uint32_t height;
};

using DrawTestFactory = std::unique_ptr<DrawTest> (*)();

struct DrawTestEntry
{
    const char* name;
    DrawTestFactory factory;
};

class DrawTestRegistry
{
public:
    static void add(const char* name, DrawTestFactory factory);
    static std::vector<DrawTestEntry>& entries();
};

class DrawTestRegistrar
{
public:
    DrawTestRegistrar(const char* name, DrawTestFactory factory);
};

} // namespace tvgdraw

namespace tvgexam
{

#ifndef TVG_DRAW_TEST_DEFAULT_UPDATE
#define TVG_DRAW_TEST_DEFAULT_UPDATE 1500
#endif

inline float progress(uint32_t elapsed, float durationInSec, bool rewind = false)
{
    auto duration = uint32_t(durationInSec * 1000.0f); //sec -> millisec.
    if (elapsed == 0 || duration == 0) return 0.0f;
    auto forward = ((elapsed / duration) % 2 == 0) ? true : false;
    if (elapsed % duration == 0) return forward ? 0.0f : 1.0f;
    auto progress = (float(elapsed % duration) / (float)duration);
    if (rewind) return forward ? progress : (1 - progress);
    return progress;
}

inline bool verify(tvg::Result result)
{
    return result == tvg::Result::Success;
}

} // namespace tvgexam

#define TVG_DRAW_TEST_CONCAT_IMPL(a, b) a##b
#define TVG_DRAW_TEST_CONCAT(a, b) TVG_DRAW_TEST_CONCAT_IMPL(a, b)

#define DRAW_TEST(NAME, WIDTH, HEIGHT, CANVAS)                                            \
    class TVG_DRAW_TEST_CONCAT(NAME, _DrawTest) final : public tvgdraw::DrawTest              \
    {                                                                                         \
    public:                                                                                   \
        TVG_DRAW_TEST_CONCAT(NAME, _DrawTest)() : tvgdraw::DrawTest(WIDTH, HEIGHT) {}         \
                                                                                              \
    private:                                                                                  \
        bool draw(tvg::Canvas* canvas) override;                                              \
    };                                                                                        \
                                                                                              \
    static std::unique_ptr<tvgdraw::DrawTest> TVG_DRAW_TEST_CONCAT(make_, NAME)()             \
    {                                                                                         \
        return std::make_unique<TVG_DRAW_TEST_CONCAT(NAME, _DrawTest)>();                     \
    }                                                                                         \
                                                                                              \
    static tvgdraw::DrawTestRegistrar TVG_DRAW_TEST_CONCAT(register_, NAME)(                  \
        #NAME, TVG_DRAW_TEST_CONCAT(make_, NAME));                                            \
                                                                                              \
    bool TVG_DRAW_TEST_CONCAT(NAME, _DrawTest)::draw(tvg::Canvas* CANVAS)

#endif
