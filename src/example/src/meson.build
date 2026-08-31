# C API, saver-only, multi-canvas, media, random, and input-driven examples are
# intentionally excluded.

# [<test name>, <copied source>, <width>, <height>, <duration in seconds>]
#   - duration = 0: renders one static frame.
#   - duration > 0: samples ExampleTestEntry::FrameCount frames evenly from 0 to
#     duration and combines them into a grid (currently 4 frames in a 2x2 grid).
#   Each grid cell keeps the declared width and height, so the current grid
#   produces an image twice as wide and twice as high.
example_tests = [
    ['accessor', 'Accessor.cpp', '800', '800', '0.0f'],
    ['animation', 'Animation.cpp', '1024', '1024', '6.003f'],
    ['blending', 'Blending.cpp', '1800', '1380', '0.0f'],
    ['bounding_box', 'BoundingBox.cpp', '900', '900', '0.0f'],
    ['clipping_svg', 'Clipping.cpp', '800', '760', '0.0f'],
    ['custom_transform', 'CustomTransform.cpp', '960', '960', '2.0f'],
    ['data_load', 'DataLoad.cpp', '800', '800', '0.0f'],
    ['direct_update', 'DirectUpdate.cpp', '960', '960', '2.0f'],
    ['duplicate', 'Duplicate.cpp', '800', '800', '0.0f'],
    ['effect_drop_shadow', 'EffectDropShadow.cpp', '800', '800', '2.5f'],
    ['fill_rule', 'FillRule.cpp', '760', '700', '0.0f'],
    ['fill_spread', 'FillSpread.cpp', '800', '760', '0.0f'],
    ['linear_gradient', 'LinearGradient.cpp', '760', '760', '0.0f'],
    ['radial_gradient', 'RadialGradient.cpp', '760', '760', '0.0f'],
    ['gradient_stroke', 'GradientStroke.cpp', '800', '820', '2.0f'],
    ['gradient_masking', 'GradientMasking.cpp', '800', '800', '0.0f'],
    ['gradient_transform', 'GradientTransform.cpp', '960', '960', '2.0f'],
    ['lottie', 'Lottie.cpp', '1280', '1280', '5.0f'],
    ['lottie_asset_resolver', 'LottieAssetResolver.cpp', '1024', '512', '2.0f'],
    ['lottie_expressions', 'LottieExpressions.cpp', '1024', '1024', '5.0f'],
    ['masking', 'Masking.cpp', '1200', '1100', '0.0f'],
    ['masking_methods', 'MaskingMethods.cpp', '1500', '1024', '0.0f'],
    ['opacity', 'Opacity.cpp', '800', '800', '0.0f'],
    ['path_commands', 'Path.cpp', '800', '700', '0.0f'],
    ['picture', 'Picture.cpp', '1024', '1024', '0.0f'],
    ['picture_jpg', 'PictureJpg.cpp', '800', '800', '0.0f'],
    ['picture_png', 'PicturePng.cpp', '800', '800', '0.0f'],
    ['picture_raw', 'PictureRaw.cpp', '800', '800', '0.0f'],
    ['picture_svg', 'PictureSvg.cpp', '960', '960', '0.0f'],
    ['picture_transform', 'PictureTransform.cpp', '1024', '1024', '3.0f'],
    ['picture_webp', 'PictureWebp.cpp', '800', '800', '0.0f'],
    ['retaining', 'Retaining.cpp', '960', '960', '1.0f'],
    ['scene', 'Scene.cpp', '760', '760', '0.0f'],
    ['scene_blending', 'SceneBlending.cpp', '800', '800', '0.0f'],
    ['scene_effects', 'SceneEffects.cpp', '1200', '800', '2.5f'],
    ['scene_transform', 'SceneTransform.cpp', '960', '960', '2.0f'],
    ['shapes', 'Shapes.cpp', '760', '760', '0.0f'],
    ['stroke', 'Stroke.cpp', '800', '800', '0.0f'],
    ['stroke_line', 'StrokeLine.cpp', '800', '960', '0.0f'],
    ['stroke_miterlimit', 'StrokeMiterlimit.cpp', '800', '800', '0.0f'],
    ['svg', 'Svg.cpp', '1280', '1280', '0.0f'],
    ['text_layout', 'TextLayout.cpp', '1100', '800', '0.0f'],
    ['text_effects', 'TextEffects.cpp', '600', '600', '2.0f'],
    ['text_line_wrap', 'TextLineWrap.cpp', '800', '800', '0.0f'],
    ['text_otf', 'TextOtf.cpp', '1024', '1024', '0.0f'],
    ['text_ttf', 'TextTtf.cpp', '1024', '1024', '0.0f'],
    ['transform', 'Transform.cpp', '960', '960', '2.0f'],
    ['trim_path', 'TrimPath.cpp', '700', '700', '0.0f'],
    ['viewport', 'Viewport.cpp', '1024', '1024', '2.0f'],
]

# Time-remap slots used by LottieSlot.cpp are supported since ThorVG 1.1.1.
if thorvg_dep.version().version_compare('>=1.1.1')
    example_tests += [['lottie_slot', 'LottieSlot.cpp', '1024', '1024', '5.0f']]
endif
