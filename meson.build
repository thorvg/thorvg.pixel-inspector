project('thorvg.pixel-inspector',
  'cpp',
  version: '1.0',
  default_options: ['buildtype=debugoptimized', 'cpp_std=c++17', 'optimization=3'],
  license : 'MIT')

pixel_inspector_dir = meson.current_source_dir()
if host_machine.system() == 'windows'
  pixel_inspector_dir = pixel_inspector_dir.replace('\\', '/')
endif

add_project_arguments('-DRESOURCE_DIR="@0@/res"'.format(pixel_inspector_dir), language : 'cpp')
add_project_arguments('-DTARGET_RESOURCE_DIR="@0@/res/default"'.format(pixel_inspector_dir), language : 'cpp')
add_project_arguments('-DEXAMPLE_DIR="@0@/res/default/basic"'.format(pixel_inspector_dir), language : 'cpp')
add_project_arguments('-DOUTPUT_DIR="@0@/output"'.format(pixel_inspector_dir), language : 'cpp')

all_backends = get_option('backends').contains('all')
cpu_backend = all_backends or get_option('backends').contains('cpu')
gl_backend = all_backends or get_option('backends').contains('gl')
wg_backend = all_backends or get_option('backends').contains('wg')

if not cpu_backend and not gl_backend and not wg_backend
  error('At least one Pixel Inspector backend is required.')
endif

subdir('src')
