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

/* ----------------------------- bug fixes ------------------------------ */

// issue #2920 / #3990 / #3991 (wg_engine: handle degenerate paths as strokes)
// Problem: a degenerate (zero-length) sub-path that is only stroked rendered
// nothing (or crashed on GPU) instead of a cap-shaped dot.
DRAW_TEST(v0_2920_degenerate_stroke_dots, 100, 100, canvas)
{
    auto bg = tvg::Shape::gen();
    bg->appendRect(0, 0, 100, 100);
    bg->fill(245, 245, 245);
    canvas->add(bg);

    auto dots = tvg::Shape::gen();  // zero-length sub-paths -> round dots
    dots->moveTo(28, 34); dots->lineTo(28, 34);
    dots->moveTo(50, 34); dots->lineTo(50, 34);
    dots->moveTo(72, 34); dots->lineTo(72, 34);
    dots->strokeWidth(15);
    dots->strokeFill(220, 60, 90);
    dots->strokeCap(tvg::StrokeCap::Round);
    canvas->add(dots);

    auto line = tvg::Shape::gen();  // collinear path, square cap
    line->moveTo(22, 70); line->lineTo(78, 70);
    line->strokeWidth(13);
    line->strokeFill(30, 120, 200);
    line->strokeCap(tvg::StrokeCap::Square);
    canvas->add(line);

    return true;
}

// issue #3231 (gl_engine: fix line join while dashing)
// Problem: a stroke that was both dashed and had sharp corners rendered the
// join incorrectly at dash boundaries on the GPU backends.
DRAW_TEST(v0_3231_dash_line_join, 100, 100, canvas)
{
    auto bg = tvg::Shape::gen();
    bg->appendRect(0, 0, 100, 100);
    bg->fill(245, 245, 245);
    canvas->add(bg);

    auto zig = tvg::Shape::gen();
    zig->moveTo(12, 78);
    zig->lineTo(35, 20);
    zig->lineTo(58, 78);
    zig->lineTo(81, 20);
    zig->lineTo(92, 60);
    zig->strokeWidth(8);
    zig->strokeFill(40, 60, 160);
    zig->strokeJoin(tvg::StrokeJoin::Miter);
    float dash[2] = {12.0f, 7.0f};
    zig->strokeDash(dash, 2);
    canvas->add(zig);

    return true;
}

// issue #3205 (wg_engine: handle properly odd numbers of dashes/gaps)
// Problem: an odd-length dash array {a,b,c} must repeat to an even pattern
// (a,b,c,a,b,c,...). An odd count was mishandled.
DRAW_TEST(v0_3205_odd_dash_array, 100, 100, canvas)
{
    auto bg = tvg::Shape::gen();
    bg->appendRect(0, 0, 100, 100);
    bg->fill(245, 245, 245);
    canvas->add(bg);

    auto ring = tvg::Shape::gen();
    ring->appendCircle(50, 50, 38, 38);
    ring->strokeWidth(8);
    ring->strokeFill(200, 80, 30);
    float dash[3] = {16.0f, 6.0f, 3.0f};  // odd count
    ring->strokeDash(dash, 3);
    canvas->add(ring);

    return true;
}

// issue #3217 / #3223 / #3191 (gl_engine: add offset support in dashed strokes)
// Problem: the dash 'offset' (phase) was ignored on the GPU backends. The two
// lines share a pattern but differ only by offset.
DRAW_TEST(v0_3217_dash_offset, 100, 80, canvas)
{
    auto bg = tvg::Shape::gen();
    bg->appendRect(0, 0, 100, 80);
    bg->fill(245, 245, 245);
    canvas->add(bg);

    float dash[2] = {14.0f, 9.0f};
    auto a = tvg::Shape::gen();
    a->moveTo(12, 28); a->lineTo(88, 28);
    a->strokeWidth(8); a->strokeFill(40, 120, 200);
    a->strokeDash(dash, 2, 0.0f);
    canvas->add(a);

    auto b = tvg::Shape::gen();
    b->moveTo(12, 54); b->lineTo(88, 54);
    b->strokeWidth(8); b->strokeFill(200, 60, 90);
    b->strokeDash(dash, 2, 11.0f);  // phase-shifted
    canvas->add(b);

    return true;
}

// issue #3954 / #4014 (wg_engine: broken / regressed radial gradient rendering)
// Problem: radial gradients with an offset focal point rendered incorrectly on
// the WebGPU backend.
DRAW_TEST(v0_3954_radial_gradient_focal, 100, 100, canvas)
{
    auto shape = tvg::Shape::gen();
    shape->appendRect(0, 0, 100, 100);

    auto fill = tvg::RadialGradient::gen();
    fill->radial(50, 50, 48, 30, 30, 0);  // cx,cy,r, fx,fy,fr (offset focal)

    tvg::Fill::ColorStop stops[3];
    stops[0] = {0.0f, 255, 255, 255, 255};
    stops[1] = {0.5f, 255, 150, 0, 255};
    stops[2] = {1.0f, 20, 0, 80, 255};
    fill->colorStops(stops, 3);

    shape->fill(fill);
    canvas->add(shape);

    return true;
}

/* --------------------------- feature work ----------------------------- */

// issue #3118 (sw/gl/wg: support trimmed FILL)  [feature]
// Added: trimpath() now also clips the filled region, not just the stroke.
DRAW_TEST(v0_3118_trimmed_fill, 100, 100, canvas)
{
    auto bg = tvg::Shape::gen();
    bg->appendRect(0, 0, 100, 100);
    bg->fill(245, 245, 245);
    canvas->add(bg);

    auto pie = tvg::Shape::gen();
    pie->appendCircle(50, 50, 40, 40);
    pie->fill(120, 80, 220);
    pie->strokeWidth(4);
    pie->strokeFill(40, 20, 90);
    pie->trimpath(0.0f, 0.6f, true);
    canvas->add(pie);

    return true;
}

// issue #2153 / #2718 (lottie/renderer: drop shadow / layer effects)  [feature]
// Added: the DropShadow scene post-effect (used by Lottie drop-shadow layers).
DRAW_TEST(v0_2153_drop_shadow, 100, 100, canvas)
{
    auto bg = tvg::Shape::gen();
    bg->appendRect(0, 0, 100, 100);
    bg->fill(250, 250, 250);
    canvas->add(bg);

    auto scene = tvg::Scene::gen();
    auto card = tvg::Shape::gen();
    card->appendRect(24, 22, 46, 46, 8, 8);
    card->fill(255, 180, 0);
    scene->add(card);
    scene->add(tvg::SceneEffect::DropShadow, 0, 0, 0, 160, 135.0, 7.0, 3.0, 100);
    canvas->add(scene);

    return true;
}

// issue #1367 / #3054 (svg feGaussianBlur / gl fill effect)  [feature]
// Added: the GaussianBlur scene post-effect (backs SVG feGaussianBlur).
DRAW_TEST(v0_1367_gaussian_blur, 100, 100, canvas)
{
    auto bg = tvg::Shape::gen();
    bg->appendRect(0, 0, 100, 100);
    bg->fill(255, 255, 255);
    canvas->add(bg);

    auto scene = tvg::Scene::gen();
    auto a = tvg::Shape::gen();
    a->appendRect(20, 20, 36, 36);
    a->fill(230, 60, 60);
    scene->add(a);
    auto b = tvg::Shape::gen();
    b->appendCircle(64, 60, 22, 22);
    b->fill(40, 110, 220);
    scene->add(b);
    scene->add(tvg::SceneEffect::GaussianBlur, 4.0, 0, 0, 100);  // sigma, dir, border, quality
    canvas->add(scene);

    return true;
}

// issue #2718 (lottie: layer effects - Tint / Tritone / Fill)  [feature]
// Added: the Tritone scene post-effect (Lottie tritone layer effect).
DRAW_TEST(v0_2718_tritone, 100, 100, canvas)
{
    auto bg = tvg::Shape::gen();
    bg->appendRect(0, 0, 100, 100);
    bg->fill(255, 255, 255);
    canvas->add(bg);

    auto scene = tvg::Scene::gen();
    auto shape = tvg::Shape::gen();
    shape->appendRect(8, 8, 84, 84, 10, 10);
    auto grad = tvg::LinearGradient::gen();
    grad->linear(8, 8, 92, 92);
    tvg::Fill::ColorStop stops[2];
    stops[0] = {0.0f, 0, 0, 0, 255};
    stops[1] = {1.0f, 255, 255, 255, 255};
    grad->colorStops(stops, 2);
    shape->fill(grad);
    scene->add(shape);
    scene->add(tvg::SceneEffect::Tritone, 30, 0, 70, 200, 110, 36, 255, 250, 210, 0);
    canvas->add(scene);

    return true;
}
