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

#include "drawTest.h"

static bool _addDst(tvg::Canvas* canvas, const char* path, float x, float y, float w, float h)
{
    auto picture = tvg::Picture::gen();
    if (picture->load(path) != tvg::Result::Success) {
        tvg::Paint::rel(picture);
        return false;
    }
    picture->size(w, h);
    picture->translate(x, y);
    canvas->add(picture);
    return true;
}

DRAW_TEST(dst_svg_scale, 360, 240, canvas)
{
    const auto path = (std::filesystem::path(EXAMPLE_DIR) / "svg" / "dst.svg").string();

    if (!_addDst(canvas, path.c_str(), 20.0f, 20.0f, 100.0f, 100.0f)) return false;
    if (!_addDst(canvas, path.c_str(), 140.0f, 20.0f, 200.0f, 200.0f)) return false;
    if (!_addDst(canvas, path.c_str(), 20.0f, 140.0f, 50.0f, 50.0f)) return false;

    return true;
}

// issue-4411: adjacent raw pictures can expose gap space between tiles.
DRAW_TEST(raw_picture_tile, 640, 480, canvas)
{
    constexpr auto TILE_PX = 16u;
    constexpr auto TILE_W = 32.0f;
    constexpr auto TILE_H = 32.0f;
    constexpr auto COLS = 20;
    constexpr auto ROWS = 15;

    auto bg = tvg::Shape::gen();
    bg->appendRect(0, 0, 640, 480);
    bg->fill(80, 80, 80);
    canvas->add(bg);

    const uint32_t colors[] = {
        0xffff0000, 0xff00aa00, 0xff0066ff, 0xffff00ff,
        0xffff8800, 0xff00bbbb, 0xff8844ff, 0xffdd3366
    };
    std::vector<uint32_t> tile(TILE_PX * TILE_PX);

    auto group = tvg::Scene::gen();
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            auto picture = tvg::Picture::gen();
            std::fill(tile.begin(), tile.end(), colors[(r * COLS + c) % (sizeof(colors) / sizeof(colors[0]))]);
            if (picture->load(tile.data(), TILE_PX, TILE_PX, tvg::ColorSpace::ARGB8888, true) != tvg::Result::Success) {
                tvg::Paint::rel(picture);
                tvg::Paint::rel(group);
                return false;
            }
            picture->size(TILE_W, TILE_H);
            picture->translate(c * TILE_W, r * TILE_H);
            group->add(picture);
        }
    }

    canvas->add(group);
    return true;
}
