#!/bin/sh
cc -shared -fPIC main.c -o device -lxcb -lxcb-image -lxcb-shm -lxcb-icccm -lxcb-xfixes "$@"
