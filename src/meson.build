cc = meson.get_compiler('cpp')

compiler_flags = []
pixel_inspector_source_file = []

thorvg_dep = dependency('thorvg-1', required: true)
thorvg_inc = include_directories(join_paths(thorvg_dep.get_variable(pkgconfig: 'includedir'), 'thorvg-1'))
pixel_inspector_dep = [thorvg_dep, ]

# Lottie Test
lottie_example = '''
    #include <thorvg_lottie.h>
    int main(int argc, char **argv) {
        return tvg::LottieAnimation::gen() ? 0 : 1;
    }
'''

if not cc.links(lottie_example, dependencies : thorvg_dep, include_directories : thorvg_inc)
    error('ThorVG Lottie is required.')
endif


subdir('external')
subdir('common')
subdir('engine')
subdir('saver')
subdir('evaluator')
subdir('draw_test')

pixel_inspector_dep += external_dep
pixel_inspector_dep += common_dep
pixel_inspector_dep += engine_dep
pixel_inspector_dep += saver_dep
pixel_inspector_dep += evaluator_dep
pixel_inspector_dep += draw_test_dep

pixel_inspector_source_file += [
    'main.cpp',
    'runner.cpp',
    'runner.h',
]
executable('tvg-pixel-inspector', pixel_inspector_source_file,
    dependencies: pixel_inspector_dep,
    cpp_args : compiler_flags)
