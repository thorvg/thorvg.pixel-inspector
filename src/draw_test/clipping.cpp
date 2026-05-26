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

void _star(tvg::Shape* shape)
{
    shape->moveTo(199, 34);
    shape->lineTo(253, 143);
    shape->lineTo(374, 160);
    shape->lineTo(287, 244);
    shape->lineTo(307, 365);
    shape->lineTo(199, 309);
    shape->lineTo(97, 365);
    shape->lineTo(112, 245);
    shape->lineTo(26, 161);
    shape->lineTo(146, 143);
    shape->close();
}

}  // namespace

DRAW_TEST(clipping_svg, 800, 760, canvas)
{
    {
        auto scene = tvg::Scene::gen();

        auto star1 = tvg::Shape::gen();
        _star(star1);
        star1->fill(255, 255, 0);
        star1->strokeFill(255, 0, 0);
        star1->strokeWidth(10);

        // Move Star1
        star1->translate(-10, -10);

        // color/alpha/opacity are ignored for a clip object - no need to set them
        auto clipStar = tvg::Shape::gen();
        clipStar->appendCircle(200, 230, 110, 110);
        clipStar->translate(10, 10);

        star1->clip((clipStar));

        auto star2 = tvg::Shape::gen();
        _star(star2);
        star2->fill(0, 255, 255);
        star2->strokeFill(0, 255, 0);
        star2->strokeWidth(10);
        star2->opacity(100);

        // Move Star2
        star2->translate(10, 40);

        // color/alpha/opacity are ignored for a clip object - no need to set them
        auto clip = tvg::Shape::gen();
        clip->appendCircle(200, 230, 130, 130);
        clip->translate(10, 10);

        scene->add(star1);
        scene->add(star2);

        // Clipping scene to shape
        scene->clip(clip);

        canvas->add(scene);
    }

    {
        auto star3 = tvg::Shape::gen();
        _star(star3);

        // Fill Gradient
        auto fill = tvg::LinearGradient::gen();
        fill->linear(100, 100, 300, 300);
        tvg::Fill::ColorStop colorStops[2];
        colorStops[0] = {0, 0, 0, 0, 255};
        colorStops[1] = {1, 255, 255, 255, 255};
        fill->colorStops(colorStops, 2);
        star3->fill(fill);

        star3->strokeFill(255, 0, 0);
        star3->strokeWidth(10);
        star3->translate(400, 0);

        // color/alpha/opacity are ignored for a clip object - no need to set them
        auto clipRect = tvg::Shape::gen();
        clipRect->appendRect(500, 120, 200, 200);  // x, y, w, h
        clipRect->translate(20, 20);

        // Clipping scene to rect(shape)
        star3->clip(clipRect);

        canvas->add(star3);
    }

    {
        auto picture = tvg::Picture::gen();
        if (!tvgexam::verify(picture->load(EXAMPLE_DIR "/svg/cartman.svg"))) return false;

        picture->scale(3);
        picture->translate(50, 400);

        // color/alpha/opacity are ignored for a clip object - no need to set them
        auto clipPath = tvg::Shape::gen();
        clipPath->appendCircle(200, 510, 50, 50);  // x, y, w, h, rx, ry
        clipPath->appendCircle(200, 650, 50, 50);  // x, y, w, h, rx, ry
        clipPath->translate(20, 20);

        // Clipping picture to path
        picture->clip(clipPath);

        canvas->add(picture);
    }

    {
        auto shape1 = tvg::Shape::gen();
        shape1->appendRect(500, 420, 250, 250, 20, 20);
        shape1->fill(255, 0, 255, 160);

        // color/alpha/opacity are ignored for a clip object - no need to set them
        auto clipShape = tvg::Shape::gen();
        clipShape->appendCircle(600, 550, 150, 150);
        clipShape->strokeWidth(20);

        // Clipping shape1 to clipShape
        shape1->clip(clipShape);

        canvas->add(shape1);
    }

    return true;
}
