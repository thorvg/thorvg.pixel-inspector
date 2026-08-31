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
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "common.h"
#include "exampleTest.h"
#include "engine.h"
#include "lodepng.h"
#include "pngSaver.h"

using namespace tvg;

namespace
{

struct Size
{
    uint32_t w = 0;
    uint32_t h = 0;
};

bool _encode(const char* filename, const uint8_t* buffer, uint32_t w, uint32_t h, LodePNGColorType colorType)
{
    std::error_code filesystemError;
    const auto parent = std::filesystem::path(filename).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, filesystemError);
        if (filesystemError) return false;
    }

    LodePNGState state;
    lodepng_state_init(&state);
    state.encoder.auto_convert = 0;
    state.info_raw.colortype = colorType;
    state.info_raw.bitdepth = 8;
    state.info_png.color.colortype = colorType;
    state.info_png.color.bitdepth = 8;

    uint8_t* png = nullptr;
    size_t pngSize = 0;
    auto pngError = lodepng_encode(&png, &pngSize, buffer, w, h, &state);
    if (!pngError) pngError = lodepng_save_file(png, pngSize, filename);

    lodepng_state_cleanup(&state);
    std::free(png);
    return pngError == 0;
}

bool _resize(TestCanvas* canvas, Picture* picture, Size maxSize)
{
    float w = 0.0f, h = 0.0f;
    picture->size(&w, &h);

    // Fit size
    const auto scaleW = static_cast<double>(maxSize.w) / w;
    const auto scaleH = static_cast<double>(maxSize.h) / h;
    const auto scale = (scaleW < scaleH) ? scaleW : scaleH;
    const Size size = {
        std::max<uint32_t>(1, static_cast<uint32_t>(std::floor(w * scale))),
        std::max<uint32_t>(1, static_cast<uint32_t>(std::floor(h * scale)))
    };

    if (picture->size(size.w, size.h) != Result::Success) return false;
    return canvas->resize(size.w, size.h);
}

void _copyFrame(std::vector<uint8_t>& dst, uint32_t dstW, Size cell, const uint8_t* src, Size size, uint32_t index)
{
    const auto columns = dstW / cell.w;
    const auto col = index % columns, row = index / columns;
    const auto dx = col * cell.w + (cell.w - size.w) / 2;
    const auto dy = row * cell.h + (cell.h - size.h) / 2;
    const auto srcStride = size.w * 4, dstStride = dstW * 4;
    for (uint32_t y = 0; y < size.h; ++y) {
        std::memcpy(dst.data() + (dy + y) * dstStride + dx * 4, src + y * srcStride, srcStride);
    }
}

}

bool PngSaver::save(TestCanvas* canvas, const char* asset, const char* filename)
{
    bool saved = false;
    const auto ext = std::filesystem::path(asset).extension();
    if (ext == ".svg") {
        const auto picture = Picture::gen();

        if (picture->load(asset) != Result::Success) {
            Picture::rel(picture);
            return false;
        }

        saved = save(canvas, picture, filename);
    } else {
        std::unique_ptr<Animation> animation(Animation::gen());

        const auto picture = animation->picture();
        if (picture->load(asset) != Result::Success) return false;

        saved = save(canvas, animation.get(), filename);
    }

    if (saved) LOG("PNG", "Saved \"%s\" to \"%s\"", asset, filename);
    return saved;
}

bool PngSaver::save(TestCanvas* canvas, Picture* picture, const char* filename)
{
    if (!_resize(canvas, picture, {maxWidth, maxWidth})) {
        Picture::rel(picture);
        return false;
    }

    if (!canvas->clear()) {
        Picture::rel(picture);
        return false;
    }

    if (canvas->ptr()->add(picture) != Result::Success) {
        Picture::rel(picture);
        return false;
    }

    if (!canvas->render()) {
        canvas->clear();
        return false;
    }

    if (!_encode(filename, canvas->buffer(), canvas->width, canvas->height, LCT_RGBA)) {
        canvas->clear();
        return false;
    }

    return canvas->clear();
}

bool PngSaver::save(TestCanvas* canvas, const char* filename)
{
    if (!canvas->render()) return false;
    return _encode(filename, canvas->buffer(), canvas->width, canvas->height, LCT_RGBA);
}

bool PngSaver::save(TestCanvas* canvas, const tvgexample::ExampleTestEntry& entry, const char* filename)
{
    auto example = entry.factory();
    if (!example) return false;

    if (!canvas->resize(entry.width, entry.height)) return false;
    if (!example->content(canvas->ptr(), entry.width, entry.height)) {
        canvas->clear();
        return false;
    }

    if (entry.durationInSec <= 0.0f) {
        auto saved = save(canvas, filename);
        auto cleared = canvas->clear();
        return saved && cleared;
    }

    // Some examples require an initial sync before their viewport or paints can be updated.
    if (!canvas->render()) {
        canvas->clear();
        return false;
    }

    constexpr auto columns = 2u;
    constexpr auto rows = tvgexample::ExampleTestEntry::FrameCount / columns;
    const auto outputW = entry.width * columns;
    const auto outputH = entry.height * rows;
    const auto size = Size{entry.width, entry.height};
    std::vector<uint8_t> buffer(static_cast<size_t>(outputW) * outputH * 4, 0);

    for (uint32_t i = 0; i < tvgexample::ExampleTestEntry::FrameCount; ++i) {
        example->elapsed = entry.elapsed(i);
        example->update(canvas->ptr(), example->elapsed);
        if (!canvas->render()) {
            canvas->clear();
            return false;
        }

        const auto src = canvas->buffer();
        if (!src) {
            canvas->clear();
            return false;
        }

        _copyFrame(buffer, outputW, size, src, size, i);
    }

    auto saved = _encode(filename, buffer.data(), outputW, outputH, LCT_RGBA);
    auto cleared = canvas->clear();
    return saved && cleared;
}

bool PngSaver::save(TestCanvas* canvas, Animation* animation, const char* filename)
{
    const auto picture = animation->picture();
    const auto totalFrame = animation->totalFrame();
    const auto grid = static_cast<uint32_t>(std::clamp(std::sqrt(totalFrame), 1.0f, 5.0f));
    const auto output = maxWidth * grid;
    if (!_resize(canvas, picture, {maxWidth, maxWidth})) return false;
    const Size size = {canvas->width, canvas->height};

    // Draw each frame to a grid image. 
    std::vector<uint8_t> buffer(static_cast<size_t>(output) * output * 4,  0);
    if (!canvas->clear()) return false;
    if (canvas->ptr()->add(picture) != Result::Success) return false;

    const auto count = grid * grid;
    for (uint32_t i = 0; i < count; ++i) {
        const auto progress = (i + 1 == count) ? 1.0f : static_cast<float>(i) / count;
        animation->frame((totalFrame - 1.0f) * progress);
        const auto rendered = canvas->render();
        const auto src = canvas->buffer();
        if (!rendered || !src) {
            canvas->ptr()->remove(picture);
            return false;
        }

        _copyFrame(buffer, output, {maxWidth, maxWidth}, src, size, i);
    }

    if (canvas->ptr()->remove(picture) != Result::Success) return false;
    return _encode(filename, buffer.data(), output, output, LCT_RGBA);
}
