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
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

#include "exampleTest.h"

namespace tvgexam
{

void Example::scandir(const char* path)
{
    std::error_code error;
    std::vector<std::string> entries;
    auto it = std::filesystem::directory_iterator(path, error);
    auto end = std::filesystem::directory_iterator();
    while (it != end && !error) {
        if (it->is_regular_file(error)) entries.push_back(it->path().string());
        it.increment(error);
    }

    std::sort(entries.begin(), entries.end());
    for (const auto& entry : entries) populate(entry.c_str());
}

}  // namespace tvgexam

namespace tvgexample
{

uint32_t ExampleTestEntry::elapsed(uint32_t frame) const
{
    auto duration = static_cast<uint32_t>(std::lround(durationInSec * 1000.0f));
    if (frame + 1 == FrameCount) return duration;
    return duration * frame / (FrameCount - 1);
}

void ExampleTestRegistry::add(const char* name, uint32_t width, uint32_t height, float durationInSec, ExampleTestFactory factory)
{
    entries().push_back({name, width, height, durationInSec, factory});
}

std::vector<ExampleTestEntry>& ExampleTestRegistry::entries()
{
    static std::vector<ExampleTestEntry> entries;
    return entries;
}

ExampleTestRegistrar::ExampleTestRegistrar(const char* name, uint32_t width, uint32_t height, float durationInSec, ExampleTestFactory factory)
{
    ExampleTestRegistry::add(name, width, height, durationInSec, factory);
}

}  // namespace tvgexample
