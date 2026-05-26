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

#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <vector>

#if defined(TVGTEST_SDL_GL_SUPPORTED)
    #define SDL_MAIN_HANDLED
    #include <SDL.h>
    #include <SDL_opengl.h>
#elif defined(TVGTEST_GL_SUPPORTED) || defined(TVGTEST_GLES_SUPPORTED)
    #include <EGL/egl.h>
    #include <EGL/eglext.h>
#if defined(TVGTEST_GLES_SUPPORTED)
    #include <GLES3/gl3.h>
#elif defined(__APPLE__)
    #include <OpenGL/gl3.h>
#else
    #include <GL/gl.h>
#endif
#endif

#if defined(TVGTEST_WG_SUPPORTED) || defined(TVGTEST_WGPU_SUPPORTED)
    #include <webgpu/webgpu.h>
#endif

#include "common.h"
#include "engine.h"

#define THREAD_COUNT 3

using namespace tvg;

struct TestEngine
{
    uint32_t width;
    uint32_t height;
    ColorSpace colorSpace;

    TestEngine(uint32_t w = 100, uint32_t h = 100, ColorSpace cs = ColorSpace::ABGR8888S) :
        width(w),
        height(h),
        colorSpace(cs)
    {
    }

    virtual ~TestEngine() {}

    virtual Canvas* init() = 0;
    virtual Result resize(Canvas* canvas, uint32_t w, uint32_t h) = 0;
    virtual const uint8_t* output(uint32_t w, uint32_t h) = 0;
};

struct TestSwEngine : TestEngine
{
    TestSwEngine(uint32_t w = 100, uint32_t h = 100, ColorSpace cs = ColorSpace::ABGR8888S) :
        TestEngine(w, h, cs)
    {}

    TestSwEngine(const TestSwEngine&) = delete;
    TestSwEngine& operator=(const TestSwEngine&) = delete;

    ~TestSwEngine() override
    {
        std::free(pixels);
        Initializer::term();
    }

    Canvas* init() override
    {
        Initializer::init(THREAD_COUNT);
        return SwCanvas::gen(EngineOption::Default);
    }

    Result resize(Canvas* canvas, uint32_t w, uint32_t h) override
    {
        if (!canvas || w == 0 || h == 0) return Result::InvalidArguments;
        auto buffer = static_cast<uint32_t*>(std::malloc(static_cast<size_t>(w) * h * sizeof(uint32_t)));
        if (!buffer) return Result::FailedAllocation;

        auto ret = static_cast<SwCanvas*>(canvas)->target(buffer, w, w, h, colorSpace);
        if (ret != Result::Success) {
            std::free(buffer);
            return ret;
        }

        std::free(pixels);
        pixels = buffer;
        width = w;
        height = h;
        return ret;
    }

    const uint8_t* output(uint32_t w, uint32_t h) override
    {
        if (w != width || h != height) return nullptr;
        return reinterpret_cast<const uint8_t*>(pixels);
    }

private:
    uint32_t* pixels = nullptr;
};

#if defined(TVGTEST_SDL_GL_SUPPORTED)

struct TestGLEngine : TestEngine
{
    SDL_Window* window = nullptr;
    SDL_GLContext context = nullptr;
    bool videoInitialized = false;

    TestGLEngine(uint32_t w = 100, uint32_t h = 100, ColorSpace cs = ColorSpace::ABGR8888S) :
        TestEngine(w, h, cs)
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) return;
        videoInitialized = true;

        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

        window = SDL_CreateWindow("thorvg-gl-test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, static_cast<int>(width), static_cast<int>(height), SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        if (!window) return;

        context = SDL_GL_CreateContext(window);
        if (!context) return;

        if (!makeCurrent()) return;
        SDL_GL_SetSwapInterval(0);
    }

    ~TestGLEngine()
    {
        Initializer::term();
        if (context) {
            SDL_GL_MakeCurrent(nullptr, nullptr);
            SDL_GL_DeleteContext(context);
        }
        if (window) SDL_DestroyWindow(window);
        if (videoInitialized) SDL_Quit();
    }

    Canvas* init() override
    {
        if (!window || !context || !makeCurrent()) return nullptr;
        Initializer::init(THREAD_COUNT);
        return GlCanvas::gen(EngineOption::Default);
    }

    Result resize(Canvas* canvas, uint32_t w, uint32_t h) override
    {
        if (!canvas || !makeCurrent() || w == 0 || h == 0) return Result::InvalidArguments;
        if (width != w || height != h) {
            SDL_SetWindowSize(window, static_cast<int>(w), static_cast<int>(h));
            width = w;
            height = h;
        }
        return static_cast<GlCanvas*>(canvas)->target(nullptr, nullptr, context, 0, width, height, colorSpace);
    }

    const uint8_t* output(uint32_t w, uint32_t h) override
    {
        if (w != width || h != height || !makeCurrent()) return nullptr;
        pixels.resize(static_cast<size_t>(width) * height * 4);
        glFinish();
        glReadPixels(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        auto stride = static_cast<size_t>(width) * 4;
        std::vector<uint8_t> row(stride);
        for (uint32_t y = 0; y < height / 2; ++y) {
            auto top = pixels.data() + static_cast<size_t>(y) * stride;
            auto bottom = pixels.data() + static_cast<size_t>(height - y - 1) * stride;
            std::memcpy(row.data(), top, stride);
            std::memcpy(top, bottom, stride);
            std::memcpy(bottom, row.data(), stride);
        }
        return pixels.data();
    }

private:
    std::vector<uint8_t> pixels;

    bool makeCurrent()
    {
        return window && context && SDL_GL_MakeCurrent(window, context) == 0;
    }
};

#elif defined(TVGTEST_GL_SUPPORTED) || defined(TVGTEST_GLES_SUPPORTED)

struct TestGLEngine : TestEngine
{
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    EGLConfig config = nullptr;

    TestGLEngine(uint32_t w = 100, uint32_t h = 100, ColorSpace cs = ColorSpace::ABGR8888S) :
        TestEngine(w, h, cs)
    {
        if (!initDisplay()) return;
#if defined(TVGTEST_GLES_SUPPORTED)
        if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) return;
#else
        if (eglBindAPI(EGL_OPENGL_API) != EGL_TRUE) return;
#endif

        // Choose config
        const EGLint configAttrs[] = {
#if defined(TVGTEST_GLES_SUPPORTED)
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
#else
            EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
#endif
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_STENCIL_SIZE, 8,
            EGL_NONE};
        EGLint count = 0;

        if (eglChooseConfig(display, configAttrs, &config, 1, &count) != EGL_TRUE || count == 0) return;

        // Create context
        const EGLint contextAttrs[] = {
#if defined(TVGTEST_GLES_SUPPORTED)
            EGL_CONTEXT_CLIENT_VERSION, 3,
#else
            EGL_CONTEXT_MAJOR_VERSION, 3,
            EGL_CONTEXT_MINOR_VERSION, 3,
            EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
#endif
            EGL_NONE};

        context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttrs);
        if (context == EGL_NO_CONTEXT) return;

        createSurface(width, height);
    }

    ~TestGLEngine()
    {
        Initializer::term();
        if (display != EGL_NO_DISPLAY) {
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
            if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
            eglTerminate(display);
        }
        eglReleaseThread();
    }

    Canvas* init() override
    {
        if (display == EGL_NO_DISPLAY || surface == EGL_NO_SURFACE || context == EGL_NO_CONTEXT) return nullptr;
        if (eglMakeCurrent(display, surface, surface, context) != EGL_TRUE) return nullptr;
        Initializer::init(THREAD_COUNT);
        return GlCanvas::gen(EngineOption::Default);
    }

    Result resize(Canvas* canvas, uint32_t w, uint32_t h) override
    {
        if (!canvas || context == EGL_NO_CONTEXT || !createSurface(w, h)) return Result::InvalidArguments;
        if (eglMakeCurrent(display, surface, surface, context) != EGL_TRUE) return Result::InvalidArguments;
        width = w;
        height = h;
        return static_cast<GlCanvas*>(canvas)->target(display, surface, context, 0, width, height, colorSpace);
    }

    const uint8_t* output(uint32_t w, uint32_t h) override
    {
        if (w != width || h != height) return nullptr;
        pixels.resize(static_cast<size_t>(width) * height * 4);
        if (eglMakeCurrent(display, surface, surface, context) != EGL_TRUE) return nullptr;
        glFinish();
        glReadPixels(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        auto stride = static_cast<size_t>(width) * 4;
        std::vector<uint8_t> row(stride);
        for (uint32_t y = 0; y < height / 2; ++y) {
            auto top = pixels.data() + static_cast<size_t>(y) * stride;
            auto bottom = pixels.data() + static_cast<size_t>(height - y - 1) * stride;
            std::memcpy(row.data(), top, stride);
            std::memcpy(top, bottom, stride);
            std::memcpy(bottom, row.data(), stride);
        }
        return pixels.data();
    }

private:
    std::vector<uint8_t> pixels;

    bool createSurface(uint32_t w, uint32_t h)
    {
        if (display == EGL_NO_DISPLAY || config == nullptr || w == 0 || h == 0) return false;
        if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);

        const EGLint attrs[] = {
            EGL_WIDTH, static_cast<EGLint>(w),
            EGL_HEIGHT, static_cast<EGLint>(h),
            EGL_NONE};

        surface = eglCreatePbufferSurface(display, config, attrs);
        return surface != EGL_NO_SURFACE;
    }

    bool initDisplay()
    {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) return false;
        if (eglInitialize(display, nullptr, nullptr) == EGL_TRUE) return true;

        display = EGL_NO_DISPLAY;

        auto queryDevicesExt = reinterpret_cast<PFNEGLQUERYDEVICESEXTPROC>(eglGetProcAddress("eglQueryDevicesEXT"));
        if (!queryDevicesExt) return false;

        auto getPlatformDisplayExt = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
        if (!getPlatformDisplayExt) getPlatformDisplayExt = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplay"));

        if (!getPlatformDisplayExt) return false;

        EGLDeviceEXT device = nullptr;
        EGLint numDevices = 0;
        if (queryDevicesExt(1, &device, &numDevices) != EGL_TRUE || numDevices <= 0) return false;

        display = getPlatformDisplayExt(EGL_PLATFORM_DEVICE_EXT, device, nullptr);
        if (display == EGL_NO_DISPLAY) return false;
        if (eglInitialize(display, nullptr, nullptr) != EGL_TRUE) {
            display = EGL_NO_DISPLAY;
            return false;
        }

        return true;
    }
};

#endif

#if defined(TVGTEST_WG_SUPPORTED) || defined(TVGTEST_WGPU_SUPPORTED)

class WgTextureReadback
{
public:
    WgTextureReadback(WGPUDevice device, WGPUTexture texture, uint32_t width, uint32_t height) :
        stride((width * 4 + 255) & ~255u),
        size(static_cast<size_t>(stride) * height)
    {
        buffer = copyTextureToBuffer(device, texture, width, height, stride);
    }

    ~WgTextureReadback()
    {
        wgpuBufferUnmap(buffer);
        wgpuBufferDestroy(buffer);
        wgpuBufferRelease(buffer);
    }

    WgTextureReadback(const WgTextureReadback&) = delete;
    WgTextureReadback& operator=(const WgTextureReadback&) = delete;

    const uint8_t* map(WGPUInstance instance)
    {
        data = mapBuffer(instance);
        return data;
    }

    uint32_t stride = 0;

private:
    WGPUBuffer copyTextureToBuffer(WGPUDevice device, WGPUTexture texture, uint32_t width, uint32_t height, uint32_t stride)
    {
        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
        bufferDesc.size = static_cast<uint64_t>(stride) * height;

        auto readback = wgpuDeviceCreateBuffer(device, &bufferDesc);

        WGPUCommandEncoderDescriptor encoderDesc = {};
        auto encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

        WGPUTexelCopyTextureInfo src = {};
        src.texture = texture;
        src.aspect = WGPUTextureAspect_All;

        WGPUTexelCopyBufferInfo dst = {};
        dst.layout.bytesPerRow = stride;
        dst.layout.rowsPerImage = height;
        dst.buffer = readback;

        WGPUExtent3D copySize = {};
        copySize.width = width;
        copySize.height = height;
        copySize.depthOrArrayLayers = 1;

        wgpuCommandEncoderCopyTextureToBuffer(encoder, &src, &dst, &copySize);

        WGPUCommandBufferDescriptor commandBufferDesc = {};
        auto commands = wgpuCommandEncoderFinish(encoder, &commandBufferDesc);
        wgpuCommandEncoderRelease(encoder);

        auto queue = wgpuDeviceGetQueue(device);

        wgpuQueueSubmit(queue, 1, &commands);
        wgpuQueueRelease(queue);
        wgpuCommandBufferRelease(commands);

        return readback;
    }

    const uint8_t* mapBuffer(WGPUInstance instance)
    {
        WGPUMapAsyncStatus mapStatus = WGPUMapAsyncStatus_Unknown;

        WGPUBufferMapCallbackInfo callback = {};
        callback.mode = WGPUCallbackMode_AllowProcessEvents;
        callback.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void* userdata1, void*) {
            *((WGPUMapAsyncStatus*)userdata1) = status;
        };
        callback.userdata1 = &mapStatus;

        wgpuBufferMapAsync(buffer, WGPUMapMode_Read, 0, size, callback);

        using clock = std::chrono::steady_clock;
        auto timeout = clock::now() + std::chrono::seconds(5);
        while (mapStatus == WGPUMapAsyncStatus_Unknown && clock::now() < timeout) {
            wgpuInstanceProcessEvents(instance);
            std::this_thread::yield();
        }

        return static_cast<const uint8_t*>(wgpuBufferGetConstMappedRange(buffer, 0, size));
    }

    size_t size = 0;
    WGPUBuffer buffer = nullptr;
    const uint8_t* data = nullptr;
};

struct TestWgEngine : TestEngine
{
    WGPUInstance instance = nullptr;
    WGPUDevice device = nullptr;
    WGPUTexture texture = nullptr;
    WGPUTextureFormat format = WGPUTextureFormat_BGRA8Unorm;

    TestWgEngine(uint32_t w = 100, uint32_t h = 100, ColorSpace cs = ColorSpace::ABGR8888S) :
        TestEngine(w, h, cs)
    {
        instance = wgpuCreateInstance(nullptr);
        if (!instance) return;

        WGPUAdapter adapter = nullptr;
        auto onAdapterRequestEnded = [](WGPURequestAdapterStatus, WGPUAdapter adapter, WGPUStringView, void* userdata1, void*) {
            *((WGPUAdapter*)userdata1) = adapter;
        };
        WGPURequestAdapterOptions requestAdapterOptions = {};
        requestAdapterOptions.featureLevel = WGPUFeatureLevel_Compatibility;
        requestAdapterOptions.powerPreference = WGPUPowerPreference_HighPerformance;

        WGPURequestAdapterCallbackInfo requestAdapterCallback = {};
        requestAdapterCallback.mode = WGPUCallbackMode_WaitAnyOnly;
        requestAdapterCallback.callback = onAdapterRequestEnded;
        requestAdapterCallback.userdata1 = &adapter;
        wgpuInstanceRequestAdapter(instance, &requestAdapterOptions, requestAdapterCallback);
        if (!adapter) return;

        auto onDeviceError = [](WGPUDevice const*, WGPUErrorType, WGPUStringView, void*, void*) {
        };
        auto onDeviceRequestEnded = [](WGPURequestDeviceStatus, WGPUDevice device, WGPUStringView, void* userdata1, void*) {
            *((WGPUDevice*)userdata1) = device;
        };
        WGPUDeviceDescriptor deviceDesc = {};
        deviceDesc.label.data = "ThorVG Test Device";
        deviceDesc.label.length = WGPU_STRLEN;
        deviceDesc.uncapturedErrorCallbackInfo.callback = onDeviceError;

        WGPURequestDeviceCallbackInfo requestDeviceCallback = {};
        requestDeviceCallback.mode = WGPUCallbackMode_WaitAnyOnly;
        requestDeviceCallback.callback = onDeviceRequestEnded;
        requestDeviceCallback.userdata1 = &device;
        wgpuAdapterRequestDevice(adapter, &deviceDesc, requestDeviceCallback);

        wgpuAdapterRelease(adapter);
        if (!device) return;

        createTexture(width, height);
    }

    ~TestWgEngine()
    {
        Initializer::term();
        if (texture) {
            wgpuTextureDestroy(texture);
            wgpuTextureRelease(texture);
        }
        if (device) {
            wgpuDeviceDestroy(device);
            wgpuDeviceRelease(device);
        }
        if (instance) wgpuInstanceRelease(instance);
    }

    Canvas* init() override
    {
        if (!instance || !device) return nullptr;
        Initializer::init(THREAD_COUNT);
        return WgCanvas::gen(EngineOption::Default);
    }

    Result resize(Canvas* canvas, uint32_t w, uint32_t h) override
    {
        if (!canvas || !createTexture(w, h)) return Result::InvalidArguments;
        auto ret = static_cast<WgCanvas*>(canvas)->target(device, instance, texture, w, h, colorSpace, 1);
        if (ret == Result::Success) {
            width = w;
            height = h;
        }
        return ret;
    }

    const uint8_t* output(uint32_t w, uint32_t h) override
    {
        if (!instance || !device || !texture || w != width || h != height) return nullptr;
        WgTextureReadback readback(device, texture, width, height);
        auto data = readback.map(instance);
        if (!data) return nullptr;

        pixels.resize(static_cast<size_t>(width) * height * 4);
        for (uint32_t y = 0; y < height; ++y) {
            auto src = data + static_cast<size_t>(y) * readback.stride;
            auto dst = pixels.data() + static_cast<size_t>(y) * width * 4;
            for (uint32_t x = 0; x < width; ++x) {
                dst[0] = src[2];
                dst[1] = src[1];
                dst[2] = src[0];
                dst[3] = src[3];
                src += 4;
                dst += 4;
            }
        }
        return pixels.data();
    }

private:
    std::vector<uint8_t> pixels;

    bool createTexture(uint32_t w, uint32_t h)
    {
        if (!device || w == 0 || h == 0) return false;
        if (texture) {
            wgpuTextureDestroy(texture);
            wgpuTextureRelease(texture);
        }

        WGPUTextureDescriptor textureDesc = {};
        textureDesc.usage = WGPUTextureUsage_CopySrc | WGPUTextureUsage_CopyDst | WGPUTextureUsage_RenderAttachment;
        textureDesc.dimension = WGPUTextureDimension_2D;
        textureDesc.size.width = w;
        textureDesc.size.height = h;
        textureDesc.size.depthOrArrayLayers = 1;
        textureDesc.format = format;
        textureDesc.mipLevelCount = 1;
        textureDesc.sampleCount = 1;

        texture = wgpuDeviceCreateTexture(device, &textureDesc);
        return texture != nullptr;
    }
};

#endif

static TestEngine* _engine(const char* type, uint32_t w, uint32_t h, ColorSpace cs)
{
    if (!type || std::strcmp(type, "sw") == 0) return new TestSwEngine(w, h, cs);
#if defined(TVGTEST_SDL_GL_SUPPORTED) || defined(TVGTEST_GL_SUPPORTED) || defined(TVGTEST_GLES_SUPPORTED)
    if (std::strcmp(type, "gl") == 0) return new TestGLEngine(w, h, cs);
#endif
#if defined(TVGTEST_WG_SUPPORTED) || defined(TVGTEST_WGPU_SUPPORTED)
    if (std::strcmp(type, "wg") == 0) return new TestWgEngine(w, h, cs);
#endif
    return nullptr;
}

TestCanvas::TestCanvas(const char* engineType, uint32_t w, uint32_t h, ColorSpace cs) :
    engine(_engine(engineType, w, h, cs))
{
    if (!engine) {
        LOGERR("ENGINE", "Invalid engine.");
        return;
    }

    canvas = engine->init();
    if (!canvas) {
        LOGERR("ENGINE", "Canvas initialization failed.");
        return;
    }

    if (w == 0) w = engine->width;
    if (h == 0) h = engine->height;
    resize(w, h);
}

TestCanvas::~TestCanvas()
{
    delete(canvas);
    delete(engine);
}

bool TestCanvas::resize(uint32_t w, uint32_t h)
{
    if (!canvas || !engine || w == 0 || h == 0) {
        LOGERR("ENGINE", "Invalid canvas resize request.");
        return false;
    }
    if (width == w && height == h) return true;

    canvas->sync();
    if (engine->resize(canvas, w, h) != Result::Success) {
        LOGERR("ENGINE", "Canvas resize failed.");
        return false;
    }

    width = w;
    height = h;
    return true;
}

bool TestCanvas::clear()
{
    if (!canvas) return false;
    return canvas->remove() == Result::Success;
}

bool TestCanvas::render()
{
    if (!canvas || !engine) return false;
    canvas->update();
    if (canvas->draw(true) != Result::Success) return false;
    return canvas->sync() == Result::Success;
}

const uint8_t* TestCanvas::buffer()
{
    if (!engine) return nullptr;
    return engine->output(width, height);
}

Canvas* TestCanvas::ptr()
{
    return canvas;
}
