#!/bin/sh
cc -shared -fPIC main.c -o device $(pkg-config --cflags cairo pango) $(pkg-config --libs cairo pango pangocairo) "$@"
