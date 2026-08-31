subdir('src')

source_file = [
    'exampleTest.h',
    'exampleTest.cpp',
    'src/Example.h',
]

foreach test : example_tests
    config = configuration_data()
    config.set('NAME', test[0])
    config.set('NAMESPACE', test[0])
    config.set('SOURCE', test[1])
    config.set('WIDTH', test[2])
    config.set('HEIGHT', test[3])
    config.set('DURATION', test[4])
    source_file += configure_file(
        input : 'exampleAdapter.cpp.in',
        output : '@0@Adapter.cpp'.format(test[0]),
        configuration : config)
endforeach

example_dep = declare_dependency(
    include_directories : [include_directories('.')],
    dependencies : [thorvg_dep],
    sources : source_file)
