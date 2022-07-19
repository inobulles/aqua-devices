#!/bin/sh
cc -shared -fPIC main.c -o device -DWITHOUT_X11 "$@"
