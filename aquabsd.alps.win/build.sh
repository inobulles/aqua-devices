#!/bin/sh
cc -shared -fPIC main.c -o device -lxcb -lxcb-icccm -lX11 -lX11-xcb "$@"
