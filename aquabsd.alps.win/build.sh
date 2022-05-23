#!/bin/sh
cc -shared -fPIC -lxcb -lxcb-icccm -lxcb-ewmh -lX11 -lX11-xcb "$@"