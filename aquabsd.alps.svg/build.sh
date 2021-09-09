#!/bin/sh
cc -shared -fPIC main.c -o device $(pkg-config --cflags librsvg-2.0) $(pkg-config --libs librsvg-2.0) "$@"
