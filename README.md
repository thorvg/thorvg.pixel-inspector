[![CodeFactor](https://www.codefactor.io/repository/github/thorvg/thorvg.example/badge)](https://www.codefactor.io/repository/github/thorvg/thorvg.example)
[![License](https://img.shields.io/badge/licence-MIT-green.svg?style=flat)](LICENSE)
[![Discord](https://img.shields.io/badge/Community-5865f2?style=flat&logo=discord&logoColor=white)](https://discord.gg/n25xj6J6HM)
[![OpenCollective](https://img.shields.io/badge/OpenCollective-84B5FC?style=flat&logo=opencollective&logoColor=white)](https://opencollective.com/thorvg)

# ThorVG Pixel Inspector

<p align="center">
  <img width="550" height="auto" src="https://raw.githubusercontent.com/thorvg/thorvg.site/refs/heads/main/readme/logo/animated_brand.svg">
</p>

**Pixel Inspector** is a rendering inspection tool for ThorVG. It renders
SVG and Lottie assets and compares generated PNGs against references with a
weighted RGBA pixel diff evaluator.

## Features

<p align="center">
  <img width="1000" height="auto" src="res/image/reporter.png">
</p>

- Renders SVG and Lottie assets with ThorVG backends.
- Compares rendered PNGs against references with weighted RGBA pixel diff.
- Provides HTML reports.

## Requirements

- C++17 compiler
- Meson and Ninja
- ThorVG installed with the required engines, loaders, and Lottie support
- SDL2 for the macOS and Windows OpenGL test context
- `wgpu-native` when testing the WebGPU backend

ThorVG can be installed from source with the helper script:

```sh
./install_thorvg.sh main
./install_thorvg.sh v1.0.5
```

## Build

```sh
meson setup builddir
meson compile -C builddir
```

By default, target resources are read from `res/target` and generated files are
written under `artifacts`.

## Usage

```sh
./builddir/src/tvg-pixel-inspector --backend=sw
./builddir/src/tvg-pixel-inspector --backend="gl,wg,sw"
./builddir/src/tvg-pixel-inspector --update-reference --backend="gl,wg,sw"
```

Helper scripts are also available:

```sh
./install_thorvg.sh v1.0.5
./build_and_run.sh --update-reference

./install_thorvg.sh main
./build_and_run.sh
```

`build_and_run.sh` configures and builds the inspector in `build/`, then forwards
all arguments to the executable.

```sh
./update_and_evaluate.sh v1.0.5 main
```

`update_and_evaluate.sh` takes two ThorVG refs. The first ref is used to install
ThorVG and update the reference images. The second ref is then installed and
compared against those references.

### Options

| Option | Description |
| --- | --- |
| `--backend=<list>` | Render backend list. |
| `--resource=<dir>` | Resource directory. |
| `--artifacts=<dir>` | Artifacts directory. |
| `--max-width=<px>` | PNG fit cell width. Images are scaled to fit this box while preserving aspect ratio. |
| `--max-channel-distance-threshold=<value>` | Max-channel distance threshold. |
| `--effective-diff-ratio-threshold=<value>` | Effective difference ratio threshold. |
| `--outlier-distance-threshold=<value>` | Outlier distance threshold. |
| `--outlier-ratio-threshold=<value>` | Clustered outlier ratio threshold. |
| `--update-reference` | Update references. |
| `--help` | Print command line help. |

### Evaluator

- Pixels are compared by RGBA Chebyshev distance, which uses the largest channel
  delta among R, G, B, and A.
- Effective pixels exclude fully transparent pixels and detected common
  background pixels.
- An image is marked as different when its effective difference ratio reaches
  `--effective-diff-ratio-threshold`, or when its clustered outlier ratio reaches
  `--outlier-ratio-threshold`.

### Draw Tests

C++ draw tests can be registered with `DRAW_TEST` under `src/draw_test`:

```cpp
DRAW_TEST(name, width, height, canvas)
{
    // Add ThorVG paints to canvas.
    return true;
}
```

For each backend, registered draw tests are rendered after that backend's asset
tests. In update mode, their reference images are updated after that backend's
asset references.

Add new draw test `.cpp` files to `src/draw_test/meson.build` so they are linked
into the inspector and registered at startup.

Current draw tests cover shapes, paths, gradients, gradient strokes, fill rules,
fill spread modes, scenes, opacity, trim paths, text layout, raw picture tiling,
SVG pictures, and clipping.

## Output

```text
artifacts/
  reporter.html
  draw_test/
    viewport.gl.reference.png
    viewport.gl.test.png
    viewport.gl.diff.png
    viewport.wg.reference.png
    viewport.wg.test.png
    viewport.wg.diff.png
    viewport.sw.reference.png
    viewport.sw.test.png
    viewport.sw.diff.png
    ...
  lottie/
    sample.gl.reference.png
    sample.gl.test.png
    sample.gl.diff.png
    sample.sw.reference.png
    ...
  svg/
    logo.gl.reference.png
    logo.gl.test.png
    logo.gl.diff.png
    logo.gl.sw.png
    ...
```

The report file is written directly under the artifacts directory.
Generated image paths use each asset path relative to the resource directory:

```text
res/target/lottie/sample.json
  -> artifacts/lottie/sample.sw.reference.png
  -> artifacts/lottie/sample.sw.test.png
  -> artifacts/lottie/sample.sw.diff.png
```

## Resources

Default assets are stored in `res/target`:

- `res/target/svg`: SVG resources
- `res/target/lottie`: Lottie JSON resources

Use `--resource <dir>` to run another target resource directory.

## License

Pixel Inspector is distributed under the MIT license. See [LICENSE](LICENSE).

PNG encoding and decoding use lodepng. lodepng is distributed under the zlib
license, and its license notice is retained in `src/saver/lodepng.h` and
`src/saver/lodepng.cpp`.
