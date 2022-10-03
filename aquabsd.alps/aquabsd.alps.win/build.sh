#!/bin/sh
cc "$@" -shared -fPIC -lxcb -lxcb-ewmh -lxcb-icccm -lxcb-image -lX11 -lX11-xcb
