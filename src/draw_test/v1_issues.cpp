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

// issue-4344: GL rejected gradients with fewer than two stops (stopCnt < 2),
// so a single-stop fill rendered as nothing. main lowered the guard to < 1.
// v1.0.0 (gl): empty cell. main (gl): solid color. sw/wg unaffected.
DRAW_TEST(issue_4344_single_stop_gradient, 400, 400, canvas)
{
    auto bg = tvg::Shape::gen();
    bg->appendRect(0, 0, 400, 400);
    bg->fill(40, 40, 40);
    canvas->add(bg);

    auto shape = tvg::Shape::gen();
    shape->appendCircle(200, 200, 150, 150);

    auto fill = tvg::LinearGradient::gen();
    fill->linear(50, 50, 350, 350);

    tvg::Fill::ColorStop stops[1];
    stops[0] = {0.5f, 255, 150, 0, 255};
    fill->colorStops(stops, 1);

    shape->fill(fill);
    canvas->add(shape);

    return true;
}

// issue-4344 (stroke variant): the same "< 2 stops" guard also dropped gradient
// STROKES on GL. main accepts single-stop gradient strokes. v1.0.0 (gl): the
// ring is not stroked (only the dark fill, == background). main: bright ring.
DRAW_TEST(issue_4344_single_stop_gradient_stroke, 400, 400, canvas)
{
    auto bg = tvg::Shape::gen();
    bg->appendRect(0, 0, 400, 400);
    bg->fill(35, 35, 35);
    canvas->add(bg);

    auto ring = tvg::Shape::gen();
    ring->appendCircle(200, 200, 140, 140);
    ring->fill(35, 35, 35);  // same as bg, so only the stroke is visible

    auto g = tvg::LinearGradient::gen();
    g->linear(60, 60, 340, 340);
    tvg::Fill::ColorStop stops[1];
    stops[0] = {0.5f, 90, 200, 255, 255};
    g->colorStops(stops, 1);
    ring->strokeFill(g);
    ring->strokeWidth(28);
    canvas->add(ring);

    return true;
}

// issue-4190: GL/WG used non-W3C formulas for the non-separable blend modes
// (Hue/Saturation/Color/Luminosity) and for SoftLight. main aligned the shaders
// with the W3C compositing spec. v1.0.0 (gl/wg): wrong tints. sw unaffected.
DRAW_TEST(issue_4190_nonseparable_blend, 600, 420, canvas)
{
    // Colorful backdrop so non-separable blends have hue/sat/luma to work with.
    auto backdrop = tvg::Shape::gen();
    backdrop->appendRect(0, 0, 600, 420);
    auto grad = tvg::LinearGradient::gen();
    grad->linear(0, 0, 600, 420);
    tvg::Fill::ColorStop bgStops[4];
    bgStops[0] = {0.0f, 255, 0, 0, 255};
    bgStops[1] = {0.33f, 0, 200, 60, 255};
    bgStops[2] = {0.66f, 40, 80, 255, 255};
    bgStops[3] = {1.0f, 255, 220, 0, 255};
    grad->colorStops(bgStops, 4);
    backdrop->fill(grad);
    canvas->add(backdrop);

    const tvg::BlendMethod modes[5] = {
        tvg::BlendMethod::SoftLight,
        tvg::BlendMethod::Hue,
        tvg::BlendMethod::Saturation,
        tvg::BlendMethod::Color,
        tvg::BlendMethod::Luminosity,
    };

    for (int i = 0; i < 5; ++i) {
        auto patch = tvg::Shape::gen();
        patch->appendRect(20 + i * 116, 60, 96, 300, 12, 12);
        patch->fill(170, 120, 210);
        patch->blend(modes[i]);
        canvas->add(patch);
    }

    return true;
}

// issue: the GPU backends now build strokes in LOCAL coordinates and transform
// the generated outline, so a thick miter stroke under a sheared / non-uniform
// matrix keeps correct widths and join spikes. v1.0.0 generated the stroke in
// already-transformed space -> distorted joins. gl/wg differ; sw was correct.
// Refs: gl 19f030fc, wg 5a12e52a ("draw strokes in local coordinates").
DRAW_TEST(strokes_local_coordinates, 480, 480, canvas)
{
    auto bg = tvg::Shape::gen();
    bg->appendRect(0, 0, 480, 480);
    bg->fill(248, 248, 248);
    canvas->add(bg);

    auto zig = tvg::Shape::gen();
    zig->moveTo(-150, -64);
    zig->lineTo(-75, 64);
    zig->lineTo(0, -64);
    zig->lineTo(75, 64);
    zig->lineTo(150, -64);
    zig->strokeWidth(36);
    zig->strokeFill(30, 60, 120);
    zig->strokeJoin(tvg::StrokeJoin::Miter);
    zig->strokeCap(tvg::StrokeCap::Butt);
    // shear + non-uniform scale + translate to center
    tvg::Matrix m = {1.7f, 0.9f, 240.0f, -0.6f, 1.3f, 250.0f, 0.0f, 0.0f, 1.0f};
    zig->transform(m);
    canvas->add(zig);

    return true;
}

// issue-4245: the GPU thin-fill fallback kicked in for sub-quantum geometry and
// left vertical seam artifacts. main suppresses the fallback. v1.0.0 (gl/wg):
// stray lines along very thin shapes. A ramp of shrinking bars exercises it.
DRAW_TEST(issue_4245_thin_fill, 480, 320, canvas)
{
    auto bg = tvg::Shape::gen();
    bg->appendRect(0, 0, 480, 320);
    bg->fill(255, 255, 255);
    canvas->add(bg);

    const float widths[8] = {6.0f, 4.0f, 2.5f, 1.5f, 1.0f, 0.75f, 0.5f, 0.3f};
    float x = 30.0f;
    for (int i = 0; i < 8; ++i) {
        auto bar = tvg::Shape::gen();
        bar->appendRect(x, 30, widths[i], 260);
        bar->fill(30, 30, 30);
        canvas->add(bar);
        x += widths[i] + 50.0f;

        auto thin = tvg::Shape::gen();
        thin->appendRect(x, 30 + i * 6, 380.0f, widths[i]);
        thin->fill(200, 40, 120);
        canvas->add(thin);
    }

    return true;
}
