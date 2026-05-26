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

namespace
{

bool _addText(tvg::Canvas* canvas, const char* label, float x, float y, float w, float h, float ax, float ay)
{
    auto text = tvg::Text::gen();
    if (!text) return false;
    if (text->font("NOTO-SANS-KR") != tvg::Result::Success) return false;
    text->translate(x, y);
    text->size(8.0f);
    text->align(ax, ay);
    text->layout(w, h);
    text->text(label);
    text->fill(0, 0, 0);
    canvas->add(text);
    return true;
}

}

DRAW_TEST(text_layout, 500, 400, canvas)
{
    const auto width = 320.0f;
    const auto height = 360.0f;
    const auto border = 60.0f;
    const auto layoutW = width - border * 2.0f;
    const auto layoutH = height - border * 2.0f;

    float dashPattern[2] = {10.0f, 10.0f};
    auto lines = tvg::Shape::gen();
    lines->strokeFill(100, 100, 100);
    lines->strokeWidth(1);
    lines->strokeDash(dashPattern, 2);
    lines->moveTo(width / 2.0f, 0);
    lines->lineTo(width / 2.0f, height);
    lines->moveTo(0, height / 2.0f);
    lines->lineTo(width, height / 2.0f);
    lines->moveTo(border, border);
    lines->lineTo(width - border, border);
    lines->lineTo(width - border, height - border);
    lines->lineTo(border, height - border);
    lines->close();
    lines->moveTo(370, 0);
    lines->lineTo(370, 400);
    canvas->add(lines);

    if (!_addText(canvas, "Top-Left", border, border, layoutW, layoutH, 0.0f, 0.0f)) return false;
    if (!_addText(canvas, "Top-Center", border, border, layoutW, layoutH, 0.5f, 0.0f)) return false;
    if (!_addText(canvas, "Top-End", border, border, layoutW, layoutH, 1.0f, 0.0f)) return false;
    if (!_addText(canvas, "Middle-Left", border, border, layoutW, layoutH, 0.0f, 0.5f)) return false;
    if (!_addText(canvas, "Middle-Center", border, border, layoutW, layoutH, 0.5f, 0.5f)) return false;
    if (!_addText(canvas, "Middle-End", border, border, layoutW, layoutH, 1.0f, 0.5f)) return false;
    if (!_addText(canvas, "Bottom-Left", border, border, layoutW, layoutH, 0.0f, 1.0f)) return false;
    if (!_addText(canvas, "Bottom-Center", border, border, layoutW, layoutH, 0.5f, 1.0f)) return false;
    if (!_addText(canvas, "Bottom-End", border, border, layoutW, layoutH, 1.0f, 1.0f)) return false;

    tvg::Point alignments[] = {{0.0f, 0.5f}, {0.25f, 0.5f}, {0.5f, 0.5f}, {0.75f, 0.5f}, {1.0f, 0.5f}};
    char label[64];
    for (int i = 0; i < 5; ++i) {
        std::snprintf(label, sizeof(label), "Align = %0.2f", 0.25 * static_cast<double>(i));
        if (!_addText(canvas, label, 380.0f, 80.0f + i * 60.0f, 0.0f, 0.0f, alignments[i].x, alignments[i].y)) return false;
    }

    return true;
}
