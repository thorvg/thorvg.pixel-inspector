# v0_issues

Repros for ThorVG work done before v1.0.0 (bug fixes and feature enhancements).
The Pixel Inspector runs on v1.0.0+, where all of these already landed, so they
are not expected to diff between two v1.0.0+ refs; they are a regression net.
Filenames: `issue-<num>-<keyword>.<ext>`. Downloaded repros keep native size;
authored ones are <=100x100.

Assets:

- `issue-2690-trim-path-order-1.json`, `issue-2690-trim-path-order-2.json` — #2690 — Lottie trim path apply order.
- `issue-2629-rounded-polygon.json` — #2629 — Lottie polygon outerRoundness.
- `issue-3591-time-remap.json` — #3591 — Lottie timeRemap frame boundary.
- `issue-2797-animated-slot-color.json` — #2797 — Lottie animated slot colorstop.
- `issue-3611-ty-parsing.json` — #3611 — Lottie `ty` parse compatibility.
- `issue-3191-gl-stroke-dash.json` — #3191 — GL stroke-dash and dash offset.
- `issue-3699-jpg-decode.json` — #3699 — JPG MCU block offset decode.
- `issue-2765-memory-oob.json` — #2765 — Lottie colorstop out-of-bounds crash (fixed).
- `issue-2153-drop-shadow.json` — #2153 — Lottie drop-shadow layer effect (feature).
- `issue-2915-default-slot.json` — #2915 — Lottie default-slot override (feature).
- `issue-3083-clippath-child-transform.svg` — #3083 — SVG transform on clipPath child.
- `issue-2960-currentcolor-gradient.svg` — #2960 — SVG currentColor in gradient stop.
- `issue-3321-gradient-stops.svg` — #3321 — SVG `<stop>` assignment.
- `issue-3615-nested-use.svg` — #3615 — SVG nested `<use>`.
- `issue-2095-svg-style-class.svg` — #2095 — SVG CSS `<style>` class set (feature).

C++ draw tests: `src/draw_test/v0_issues.cpp` (<=100x100, v1.0.0+ API, issue/problem in comments):

- `v0_2920_degenerate_stroke_dots` — #2920/#3990/#3991 — degenerate path stroked to cap dots.
- `v0_3231_dash_line_join` — #3231 — line join while dashing.
- `v0_3205_odd_dash_array` — #3205 — odd-length dash array repeat.
- `v0_3217_dash_offset` — #3217/#3223/#3191 — dash offset (phase).
- `v0_3954_radial_gradient_focal` — #3954/#4014 — radial gradient offset focal.
- `v0_3118_trimmed_fill` — #3118 — trimpath applied to fill (feature).
- `v0_2153_drop_shadow` — #2153/#2718 — DropShadow scene effect (feature).
- `v0_1367_gaussian_blur` — #1367/#3054 — GaussianBlur scene effect (feature).
- `v0_2718_tritone` — #2718 — Tritone layer effect (feature).
