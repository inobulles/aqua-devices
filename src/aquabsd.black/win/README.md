# .win

This device provides a method for creating and manipulating windows for AQUA programs.
This implementation does this through [Wayland](https://wayland.freedesktop.org/).

## Building XDG shell code

With `wayland-scanner`:

```console
wayland-scanner private-code < /usr/local/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > xdg-shell-protocol.c
wayland-scanner client-header < /usr/local/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > xdg-shell-client-protocol.h
```

On your OS distribution, this `xdg-shell.xml` file may be found elsewhere (e.g. at `/usr/share` instead of `/usr/local/share`).

TODO This should really automatically be done by Bob.

## Note on X11 support

If you need to use X11, use `aquabsd.alps.win`, which still works perfectly well and is (almost) forward-compatible with `aquabsd.black.win`.
The current implementation only supports Wayland.
X11 support may come back in the future, but no promises ;)
