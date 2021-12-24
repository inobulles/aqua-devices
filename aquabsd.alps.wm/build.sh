#!/bin/sh
cc -shared -fPIC main.c -o device -lxcb -lxcb-icccm -lxcb-composite -lxcb-xfixes -lxcb-shape -lxcb-randr -lX11 -lX11-xcb "$@"
