# .ui

This device provides commands for AQUA apps to create user interfaces through a variety of different backends.
This implementation specifically provides the `wgpu` and `fb` backends.

## WebGPU backend (`wgpu`)

WebGPU context on an `aquabsd.alps.win` window.
Relies on the `wgpu-native` library (see `aquabsd.black.wgpu`'s README for more info), but doesn't rely on `.wgpu` (at runtime, at least).

## Framebuffer backend (`fb`)

To be done! 🚧
