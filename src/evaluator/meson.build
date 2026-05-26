source_file = [
   'evaluator.h',
   'evaluator.cpp',
   'evaluationQueue.h',
   'evaluationQueue.cpp',
   'flipEvaluator.h',
   'flipEvaluator.cpp',
   'pixelEvaluator.h',
   'pixelEvaluator.cpp',
]

evaluator_dep = declare_dependency(
    include_directories : [include_directories('.')],
    dependencies : [common_dep, external_dep, saver_dep],
    sources : source_file)
