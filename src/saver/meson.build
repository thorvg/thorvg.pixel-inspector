source_file = [
   'htmlSaver.h',
   'htmlSaver.cpp',
   'jsonSaver.h',
   'jsonSaver.cpp',
   'mdSaver.h',
   'mdSaver.cpp',
   'pngSaver.h',
   'pngSaver.cpp',
]

saver_dep = declare_dependency(
    include_directories : include_directories('.'),
    dependencies : [common_dep, engine_dep, external_dep],
    sources : source_file)
