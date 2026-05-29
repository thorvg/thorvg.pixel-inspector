# v1_issue

Minimal repros for ThorVG fixes/features landed between v1.0.0 and main. Rendered
by the Pixel Inspector; each renders one way on v1.0.0 and the corrected way on
main, so the inspector flags a pixel diff. Filenames: `issue-<num>-<keyword>.<ext>`.

Assets:

- `issue-2681-pucker-bloat.json` — #2681 — Lottie pucker/bloat (`pb`) modifier; ignored on v1.0.0.
- `issue-4158-text-range-selector.json` — #4158 — Lottie text range selector factor / world-space transform.
- `issue-4325-posteffect-stride.json` — #4325 — Lottie post-effect fill stride.
- `issue-4269-word-wrap.json` — #4269 — Lottie text word-boundary line wrapping.
- `issue-3801-bezier-color.json` — #3801 — Lottie solid-fill color with bezier easing.
- `issue-3896-tofixed-rounding.json` — #3896 — Lottie expression `toFixed(0)` rounding.
- `issue-4255-mix-blend-mode.svg` — #4255 — SVG `mix-blend-mode`; ignored on v1.0.0.
- `issue-2713-em-units.svg` — #2713 — SVG `em`/`ex` length units in text.
- `issue-1331-userspaceonuse-gradient.svg` — #1331 — SVG userSpaceOnUse gradient without a viewport.
- `issue-svg-tspan.svg` — (no issue) — SVG `<tspan>` element support.
- `issue-svg-dxdy.svg` — (no issue) — SVG `dx`/`dy` on text/tspan.
- `issue-svg-text-anchor.svg` — (no issue) — SVG `text-anchor` attribute.
- `issue-4320-url-font-*.json` (carriage, sample, test, textdoc) — #4320 — Lottie URL-font text layout.
- `issue-4408-iso-rotation-sweep.json`, `issue-4408-repro-stroke-scale.json` — #4408 — stroke scale under rotation.

Related C++ draw tests: `src/draw_test/issue-v1_0_0-main.cpp` (single-stop gradient #4344, non-separable blend #4190, thin-fill #4245, local-coordinate strokes).

Crash-class case kept separate (segfaults v1.0.0): `res/crash-cases/` (#4315).
