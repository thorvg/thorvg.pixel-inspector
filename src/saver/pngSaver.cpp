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
#include <cstdio>
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

bool _encodeFile(const char* filename, const uint8_t* buffer, uint32_t w, uint32_t h, LodePNGColorType colorType)
{
    LodePNGState state;
    lodepng_state_init(&state);
    state.encoder.auto_convert = 0;
    state.info_raw.colortype = colorType;
    state.info_raw.bitdepth = 8;
    state.info_png.color.colortype = colorType;
    state.info_png.color.bitdepth = 8;

    uint8_t* png = nullptr;
    size_t pngSize = 0;
    auto error = lodepng_encode(&png, &pngSize, buffer, w, h, &state);
    if (!error) error = lodepng_save_file(png, pngSize, filename);

    lodepng_state_cleanup(&state);
    std::free(png);
    return error == 0;
}

bool _encode(const char* filename, const uint8_t* buffer, uint32_t w, uint32_t h, LodePNGColorType colorType)
{
    std::error_code error;
    const auto parent = std::filesystem::path(filename).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, error);
        if (error) return false;
    }

    return _encodeFile(filename, buffer, w, h, colorType);
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

bool _renderFrame(TestCanvas* canvas, Picture* picture)
{
    if (!canvas->clear()) return false;

    if (canvas->ptr()->add(picture) != Result::Success) return false;
    return canvas->render();
}

}

std::vector<uint8_t> toRGB8(const uint8_t* rgba, uint32_t w, uint32_t h)
{
    std::vector<uint8_t> rgb(static_cast<size_t>(w) * h * 3);
    auto src = rgba;
    auto dst = rgb.data();
    const auto count = w * h;
    for (uint32_t i = 0; i < count; ++i) {
        const auto a = static_cast<uint32_t>(src[3]);
        dst[0] = static_cast<uint8_t>((static_cast<uint32_t>(src[0]) * a + 255u * (255u - a) + 127u) / 255u);
        dst[1] = static_cast<uint8_t>((static_cast<uint32_t>(src[1]) * a + 255u * (255u - a) + 127u) / 255u);
        dst[2] = static_cast<uint8_t>((static_cast<uint32_t>(src[2]) * a + 255u * (255u - a) + 127u) / 255u);
        src += 4;
        dst += 3;
    }
    return rgb;
}

std::vector<float> normalizeRGB(const std::vector<uint8_t>& rgb8)
{
    std::vector<float> rgb(rgb8.size());
    for (size_t i = 0, n = rgb8.size(); i < n; ++i) {
        rgb[i] = static_cast<float>(rgb8[i]) / 255.0f;
    }
    return rgb;
}

bool loadRGBA(const char* filename, PngImage* image)
{
    uint8_t* buffer = nullptr;
    unsigned w = 0;
    unsigned h = 0;
    if (lodepng_decode32_file(&buffer, &w, &h, filename) != 0) return false;

    image->w = w;
    image->h = h;
    image->pixels.assign(buffer, buffer + static_cast<size_t>(w) * h * 4);

    std::free(buffer);
    return true;
}

bool saveRGBA(const char* filename, const uint8_t* pixels, uint32_t w, uint32_t h)
{
    return _encode(filename, pixels, w, h, LCT_RGBA);
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

    picture->ref();
    const auto ret = _renderFrame(canvas, picture) && saveRGBA(filename, canvas->buffer(), canvas->width, canvas->height);
    const auto cleared = canvas->clear();
    picture->unref();
    return ret && cleared;
}

bool PngSaver::save(TestCanvas* canvas, const char* filename)
{
    if (!canvas->render()) return false;
    return saveRGBA(filename, canvas->buffer(), canvas->width, canvas->height);
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
    const auto count = grid * grid;
    for (uint32_t i = 0; i < count; ++i) {
        const auto progress = (i + 1 == count) ? 1.0f : static_cast<float>(i) / count;
        animation->frame((totalFrame - 1.0f) * progress);
        _renderFrame(canvas, picture);
        const auto src = canvas->buffer();

        // Copy
        const auto col = i % grid, row = i / grid;
        const auto dx = col * maxWidth + (maxWidth - size.w) / 2, dy = row * maxWidth + (maxWidth - size.h) / 2;
        const auto sx = dx * 4, sy = dy;
        const auto srcStride = size.w * 4, dstStride = output * 4;
        for (uint32_t y = 0; y < size.h; ++y) {
            std::memcpy(buffer.data() + (sy + y) * dstStride + sx, src + y * srcStride, srcStride);
        }
        canvas->clear();
    }

    return saveRGBA(filename, buffer.data(), output, output);
}
