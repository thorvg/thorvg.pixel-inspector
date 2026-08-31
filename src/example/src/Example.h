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

#ifndef _TVG_PIXEL_INSPECTOR_EXAMPLE_H_
#define _TVG_PIXEL_INSPECTOR_EXAMPLE_H_

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <thorvg_lottie.h>

#include "exampleTest.h"

using namespace std;

namespace tvgexam
{

inline float progress(uint32_t elapsed, float durationInSec, bool rewind = false)
{
    auto duration = uint32_t(durationInSec * 1000.0f); //sec -> millisec.
    if (elapsed == 0 || duration == 0) return 0.0f;
    auto forward = ((elapsed / duration) % 2 == 0) ? true : false;
    if (elapsed % duration == 0) return forward ? 0.0f : 1.0f;
    auto progress = (float(elapsed % duration) / (float)duration);
    if (rewind) return forward ? progress : (1 - progress);
    return progress;
}

inline bool verify(tvg::Result result)
{
    return result == tvg::Result::Success;
}

inline void input(int argc, char** argv, std::string& out)
{
    for (auto i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], "-i") != 0) continue;
        out = argv[i + 1];
        break;
    }
}

// Generated adapters instantiate UserExample directly; keep the copied entry point linkable.
inline int main(Example* example, int, char**, bool = false, uint32_t = 800, uint32_t = 800, uint32_t = 4, bool = false)
{
    delete (example);
    return 0;
}

}  // namespace tvgexam

#endif
