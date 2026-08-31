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

#ifndef _TVG_PIXEL_INSPECTOR_EXAMPLE_TEST_H_
#define _TVG_PIXEL_INSPECTOR_EXAMPLE_TEST_H_

#include <cstdint>
#include <memory>
#include <vector>

#include <thorvg.h>

namespace tvgexam
{
struct Example
{
    uint32_t elapsed = 0;

    virtual ~Example() = default;

    virtual bool content(tvg::Canvas* canvas, uint32_t w, uint32_t h) = 0;
    virtual bool update(tvg::Canvas*, uint32_t) { return false; }
    virtual void populate(const char*) {}

    void scandir(const char* path);
};
}  // namespace tvgexam

namespace tvgexample
{
using ExampleTestFactory = std::unique_ptr<tvgexam::Example> (*)();

struct ExampleTestEntry
{
    static constexpr uint32_t FrameCount = 4;

    const char* name;
    uint32_t width;
    uint32_t height;
    float durationInSec;
    ExampleTestFactory factory;

    uint32_t elapsed(uint32_t frame) const;
};

class ExampleTestRegistry
{
public:
    static void add(const char* name, uint32_t width, uint32_t height, float durationInSec, ExampleTestFactory factory);
    static std::vector<ExampleTestEntry>& entries();
};

class ExampleTestRegistrar
{
public:
    ExampleTestRegistrar(const char* name, uint32_t width, uint32_t height, float durationInSec, ExampleTestFactory factory);
};

}  // namespace tvgexample

#endif
