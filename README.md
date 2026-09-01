[![License](https://img.shields.io/badge/licence-MIT-green.svg?style=flat)](LICENSE)
[![Wikipedia](https://img.shields.io/badge/Wikipedia-000000?style=flat&logo=wikipedia&logoColor=white)](https://en.wikipedia.org/wiki/Thor_Vector_Graphics)
[![Discord](https://img.shields.io/badge/Community-5865f2?style=flat&logo=discord&logoColor=white)](https://discord.gg/n25xj6J6HM)
[![OpenCollective](https://img.shields.io/badge/OpenCollective-84B5FC?style=flat&logo=opencollective&logoColor=white)](https://opencollective.com/thorvg)

# ThorVG Pixel Inspector

<p align="center">
  <img width="550" height="auto" src="https://raw.githubusercontent.com/thorvg/thorvg.site/refs/heads/main/readme/logo/animated_brand.svg">
</p>

**Pixel Inspector** is a rendering inspection tool for ThorVG. It renders
SVG and Lottie assets and compares generated PNGs against golden images with a
weighted RGBA pixel diff evaluator.

## Features

<p align="center">
  <img width="1000" height="auto" src="res/image/reporter.png">
</p>

- Renders SVG and Lottie assets with ThorVG backends.
- Compares rendered PNGs against golden images with weighted RGBA pixel diff.
- Provides HTML reports and a compact markdown summary for CI.

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

Build only the backend required by an isolated CI job:

```sh
meson setup build-cpu -Dbackends=cpu
meson setup build-gl -Dbackends=gl
meson setup build-wg -Dbackends=wg
```

An explicitly selected GL or WG backend makes its platform dependency required.
The default `all` setting keeps the existing best-effort dependency detection.

The helper scripts expose the same split through environment variables:

```sh
THORVG_ENGINES=cpu ./install_thorvg.sh main
PIXEL_BACKENDS=cpu ./build_and_run.sh --backend=cpu
```

By default, target resources are read from `res/default` and generated files are
written under `output`.

## Usage

```sh
./builddir/src/tvg-pixel-inspector --backend=cpu
./builddir/src/tvg-pixel-inspector --backend="gl,wg,cpu"
./builddir/src/tvg-pixel-inspector --update-golden --backend="gl,wg,cpu"
```

Helper scripts are also available:

```sh
./install_thorvg.sh v1.0.5
./build_and_run.sh --update-golden

./install_thorvg.sh main
./build_and_run.sh
```

`build_and_run.sh` configures and builds the inspector in `build/`, then forwards
all arguments to the executable.

```sh
./update_and_evaluate.sh v1.0.5 main
```

`update_and_evaluate.sh` takes two ThorVG refs. The first ref is used to install
ThorVG and update the golden images. The second ref is then installed and
compared against those golden images.

### Compliance

Run all compliance resources against two ThorVG refs:

```sh
./compliance.sh --unzip v1.1.1 main
./compliance.sh main main
```

Compliance tests resources under `res/compliance` using the CPU backend. Use
`--unzip` to extract ZIP assets before testing.

### Options

| Option | Description |
| --- | --- |
| `--backend=<list>` | Render backend list. |
| `--resource=<dir>` | Resource directory. |
| `--output=<dir>` | Output directory. |
| `--max-width=<px>` | PNG fit cell width. Images are scaled to fit this box while preserving aspect ratio. |
| `--shard-index=<index>` | Zero-based shard index. Must be less than `--shard-count`. Default `0`. |
| `--shard-count=<count>` | Total number of shards. Default `1`. |
| `--max-channel-distance-threshold=<value>` | Max-channel distance threshold. A pixel counts as different when its RGBA Chebyshev distance exceeds this. Default `0`. |
| `--diff-ratio-threshold=<value>` | Diff ratio threshold. An image is marked different when its diff ratio exceeds this. Default `0`. |
| `--skip-examples` | Skip registered C++ example tests. |
| `--update-golden` | Update golden images. |
| `--help` | Print command line help. |

### CI Sharding

Use `--shard-index` and `--shard-count` to split tests across CI workers:

```sh
./builddir/src/tvg-pixel-inspector \
  --shard-index=0 \
  --shard-count=3
```

Assets and example tests are assigned deterministically. Lottie JSON and SVG assets
are sharded independently. Use the same shard options for golden and test runs.

### Evaluator

- Pixels are compared by RGBA Chebyshev distance, which uses the largest channel
  delta among R, G, B, and A.
- Fully transparent pixels (alpha 0 in both the golden and the actual) are
  excluded; every other pixel is compared.
- A pixel counts as different when its distance exceeds
  `--max-channel-distance-threshold` (default `0`, so any non-identical pixel
  counts).
- An image is marked as different when its diff ratio exceeds
  `--diff-ratio-threshold` (default `0`, so a single differing pixel fails the
  image). These defaults make comparisons strict.
- The report also lists `Diff Pixel Count`, the number of differing pixels in
  each comparison.

### Example Tests

Copy a ThorVG example unchanged into `src/example/src`, then add it to
`src/example/src/meson.build`:

```meson
# [<test name>, <copied source>, <width>, <height>, <duration in seconds>]
['transform', 'Transform.cpp', '960', '960', '2.0f'],
```

Use `0.0f` for a static example. A positive duration produces evenly sampled
frames in a grid. Meson generates and registers the adapter automatically.

## Output

```text
output/
  reporter.html
  reporter.md
  example/
    viewport.gl.golden.png
    viewport.gl.actual.png
    viewport.gl.diff.png
    viewport.wg.golden.png
    viewport.wg.actual.png
    viewport.wg.diff.png
    viewport.cpu.golden.png
    viewport.cpu.actual.png
    viewport.cpu.diff.png
    ...
  lottie/
    sample.gl.golden.png
    sample.gl.actual.png
    sample.gl.diff.png
    sample.cpu.golden.png
    ...
  svg/
    logo.gl.golden.png
    logo.gl.actual.png
    logo.gl.diff.png
    logo.cpu.golden.png
    ...
```

The report files are written directly under the output directory:
`reporter.html` is the full interactive report, and `reporter.md` is a compact
markdown summary (per-backend `Compared`, `Differences`, `Errors` counts) that
CI can publish as-is.
Generated image paths use each asset path relative to the resource directory:

```text
res/default/lottie/sample.json
  -> output/lottie/sample.cpu.golden.png
  -> output/lottie/sample.cpu.actual.png
  -> output/lottie/sample.cpu.diff.png
```

## Resources

Default assets are stored in `res/default`:

- `res/default/svg`: SVG resources
- `res/default/lottie`: Lottie JSON resources

Use `--resource <dir>` to run another target resource directory.

## License

Pixel Inspector is distributed under the MIT license. See [LICENSE](LICENSE).

PNG encoding and decoding use lodepng. lodepng is distributed under the zlib
license, and its license notice is retained in `src/saver/lodepng.h` and
`src/saver/lodepng.cpp`.
