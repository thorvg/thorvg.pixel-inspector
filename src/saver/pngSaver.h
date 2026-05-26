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

#ifndef _TVG_PIXEL_INSPECTOR_PNG_SAVER_H_
#define _TVG_PIXEL_INSPECTOR_PNG_SAVER_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace tvg
{
struct Animation;
struct Picture;
}

struct TestCanvas;

struct PngImage
{
    uint32_t w = 0;
    uint32_t h = 0;
    std::vector<uint8_t> pixels;
};

std::vector<uint8_t> toRGB8(const uint8_t* rgba, uint32_t w, uint32_t h);
std::vector<float> normalizeRGB(const std::vector<uint8_t>& rgb8);
bool loadRGBA(const char* filename, PngImage* image);
bool saveRGBA(const char* filename, const uint8_t* pixels, uint32_t w, uint32_t h);

struct PngSaver
{
    explicit PngSaver(uint32_t maxWidth) : maxWidth(maxWidth) {}

    bool save(TestCanvas* canvas, const char* asset, const char* filename);
    bool save(TestCanvas* canvas, tvg::Picture* picture, const char* filename);
    bool save(TestCanvas* canvas, const char* filename);
    bool save(TestCanvas* canvas, tvg::Animation* animation, const char* filename);

    const uint32_t maxWidth;
};

#endif
