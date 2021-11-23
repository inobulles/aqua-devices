#!/bin/sh
cc -shared -fPIC main.c -o device -lEGL -lxcb -lxcb-composite "$@"
