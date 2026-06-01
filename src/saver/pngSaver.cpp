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

        // Copy
        const auto col = i % grid, row = i / grid;
        const auto dx = col * maxWidth + (maxWidth - size.w) / 2, dy = row * maxWidth + (maxWidth - size.h) / 2;
        const auto sx = dx * 4, sy = dy;
        const auto srcStride = size.w * 4, dstStride = output * 4;
        for (uint32_t y = 0; y < size.h; ++y) {
            std::memcpy(buffer.data() + (sy + y) * dstStride + sx, src + y * srcStride, srcStride);
        }
    }

    if (canvas->ptr()->remove(picture) != Result::Success) return false;
    return _encode(filename, buffer.data(), output, output, LCT_RGBA);
}
