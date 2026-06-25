# Artifact Issue Report

## Summary

`svg2009.svg` showed visible artifacts when rendered by this project and saved through the inspector pipeline, while ThorVG's standalone `tvg-svg2png` output looked correct.

The problem was not caused by PNG encoding. It was caused by global font loading before asset rendering.

## Why It Happened

The runner was loading all bundled fonts from `res/font` before rendering every asset.

That changed ThorVG's global text/font resolution state for the whole process. As a result:

- SVGs that do not explicitly depend on our bundled fonts could still render differently.
- `svg2009.svg` contains text content and font references.
- Once the bundled fonts were loaded, ThorVG used different fallback or matching behavior for that SVG.
- The text layout/raster result changed, which appeared as artifacts in the saved PNG.

The important point is that this was a render-state problem, not a `pngSaver` byte-order problem.

## Why I Set These Fonts Families

The bundled font families are loaded because this repository contains assets and draw tests that intentionally depend on them.

Current bundled families:

- `DMSans`
- `NOTO-SANS-KR`
- `NanumGothicCoding`
- `Pretendard`
- `PublicSans-Regular`
- `SentyCloud`

These families are needed for two main reasons:

1. Draw tests

- Several draw tests call `text->font(...)` with these exact family names.
- Without loading those fonts first, those tests would fail or render with unpredictable fallback fonts.

2. Resource assets

- Some SVG assets explicitly reference bundled families, for example `PublicSans-Regular`.
- Those assets need the matching font loaded to produce stable output.

So the font list exists to make text-related tests and assets deterministic.

## What Was Wrong With The Old Approach

The old approach loaded bundled fonts once and kept them active for every later asset.

That is too broad because:

- text/font registration is effectively global in the ThorVG process
- unrelated SVGs are affected by the presence of those extra fonts
- fallback behavior changes even when an asset does not actually need our bundled fonts

In short: the project loaded fonts for correctness in some assets, but that same global state corrupted rendering expectations for other assets.

## Fix

The runner now keeps resource asset rendering separate from draw tests:

1. Render all resource assets in a clean session without loading bundled fonts.
2. Run draw tests in a fresh session after loading bundled fonts, because many draw tests explicitly require those families.

This keeps draw-test text coverage intact without contaminating resource asset rendering.

## Result

- `svg2009.svg` no longer shows the artifact in the software backend path.
- Font-dependent draw tests still render with the intended bundled fonts.
- The issue is documented as a font-scope/render-state problem, not a PNG saver problem.
