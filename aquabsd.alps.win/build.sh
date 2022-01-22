#!/bin/sh
cc -shared -fPIC main.c -o device -lxcb -lxcb-icccm -lxcb-ewmh -lX11 -lX11-xcb "$@"