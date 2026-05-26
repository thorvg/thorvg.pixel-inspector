meson setup build --wipe
ninja -C build
./build/src/tvg-pixel-inspector "$@"