#!/bin/sh
cc -shared -fPIC main.c -lm -o device "$@"
