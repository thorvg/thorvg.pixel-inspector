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
add_project_arguments('-DTARGET_RESOURCE_DIR="@0@/res/target"'.format(pixel_inspector_dir), language : 'cpp')
add_project_arguments('-DEXAMPLE_DIR="@0@/res/target"'.format(pixel_inspector_dir), language : 'cpp')
add_project_arguments('-DARTIFACTS_DIR="@0@/artifacts"'.format(pixel_inspector_dir), language : 'cpp')

subdir('src')
