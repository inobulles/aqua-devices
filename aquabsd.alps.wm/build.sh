#!/bin/sh
cc "$@" -shared -fPIC -lxcb -lxcb-icccm -lxcb-composite -lxcb-xfixes -lxcb-shape -lxcb-randr -lxcb-ewmh -lX11 -lX11-xcb
