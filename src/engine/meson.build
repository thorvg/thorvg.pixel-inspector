source_file = [
   'engine.cpp',
   'engine.h',
]

engine_backend_dep = []

# GL or GLES
if host_machine.system() == 'darwin' or host_machine.system() == 'windows'
    sdl2_dep = dependency('sdl2', required: false)
    gl_dep = dependency('gl', required: false)
    if sdl2_dep.found() and gl_dep.found()
        engine_backend_dep += [sdl2_dep, gl_dep]
        compiler_flags += ['-DTVGTEST_SDL_GL_SUPPORTED']
    else
        message('SDL2 with OpenGL was not found. Skipping GL tests.')
    endif
else
    egl_dep = dependency('egl', required: false)
    gl_dep = dependency('gl', required: false)
    gles_dep = dependency('glesv2', required: false)
    if egl_dep.found() and gl_dep.found()
        engine_backend_dep += [egl_dep, gl_dep]
        compiler_flags += ['-DTVGTEST_GL_SUPPORTED']
    elif egl_dep.found() and gles_dep.found()
        engine_backend_dep += [egl_dep, gles_dep]
        compiler_flags += ['-DTVGTEST_GLES_SUPPORTED']
    else
        message('EGL with OpenGL or OpenGL ES was not found. Skipping GL tests.')
    endif
endif

# WebGPU
wgpu_dep = dependency('wgpu_native', required: false)

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

engine_dep = declare_dependency(
    dependencies : engine_backend_dep,
    include_directories : include_directories('.'),
    sources : source_file)
