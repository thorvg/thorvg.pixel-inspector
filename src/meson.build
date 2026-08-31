cc = meson.get_compiler('cpp')

compiler_flags = []
pixel_inspector_source_file = []

if get_option('log')
    compiler_flags += ['-DTVGTEST_LOG_ENABLED']
endif

thorvg_dep = dependency('thorvg-1', required: true)
pixel_inspector_dep = [thorvg_dep, ]

# Lottie Test
lottie_example = '''
    #include <thorvg_lottie.h>
    int main(int argc, char **argv) {
        return tvg::LottieAnimation::gen() ? 0 : 1;
    }
'''

if not cc.links(lottie_example, dependencies : thorvg_dep)
    error('ThorVG Lottie is required.')
endif

engine_backend_dep = []

if cpu_backend
    compiler_flags += ['-DTVGTEST_CPU_SUPPORTED']
endif

# GL or GLES
if gl_backend
    backend_required = not all_backends
    if host_machine.system() == 'darwin' or host_machine.system() == 'windows'
        sdl2_dep = dependency('sdl2', required: backend_required)
        gl_dep = dependency('gl', required: backend_required)
        if sdl2_dep.found() and gl_dep.found()
            engine_backend_dep += [sdl2_dep, gl_dep]
            compiler_flags += ['-DTVGTEST_SDL_GL_SUPPORTED']
        else
            message('SDL2 with OpenGL was not found. Skipping GL tests.')
        endif
    else
        egl_dep = dependency('egl', required: backend_required)
        gl_dep = dependency('gl', required: false)
        gles_dep = dependency('glesv2', required: false)
        if egl_dep.found() and gl_dep.found()
            engine_backend_dep += [egl_dep, gl_dep]
            compiler_flags += ['-DTVGTEST_GL_SUPPORTED']
        elif egl_dep.found() and gles_dep.found()
            engine_backend_dep += [egl_dep, gles_dep]
            compiler_flags += ['-DTVGTEST_GLES_SUPPORTED']
        else
            if backend_required
                error('EGL with OpenGL or OpenGL ES is required for the GL backend.')
            endif
            message('EGL with OpenGL or OpenGL ES was not found. Skipping GL tests.')
        endif
    endif
endif

# WebGPU
if wg_backend
    wgpu_dep = dependency('wgpu_native', required: not all_backends)
    if wgpu_dep.found()
        engine_backend_dep += wgpu_dep
        compiler_flags += ['-DTVGTEST_WGPU_SUPPORTED']
        if host_machine.system() == 'darwin'
            add_languages('objcpp', native: false)
            engine_backend_dep += declare_dependency(link_args: ['-lobjc', '-framework', 'Cocoa', '-framework', 'CoreVideo', '-framework', 'IOKit', '-framework', 'QuartzCore'])
            compiler_flags += ['-x', 'objective-c++']
        endif
    else
        message('wgpu_native not found, skipping WGPU tests.')
    endif
endif


subdir('saver')
subdir('example')

pixel_inspector_dep += engine_backend_dep
pixel_inspector_dep += saver_dep
pixel_inspector_dep += example_dep

pixel_inspector_source_file += [
    'common.h',
    'main.cpp',
    'runner.cpp',
    'runner.h',
    'evaluator.cpp',
    'evaluator.h',
    'engine.cpp',
    'engine.h',
]
executable('tvg-pixel-inspector', pixel_inspector_source_file,
    dependencies: pixel_inspector_dep,
    cpp_args : compiler_flags)
