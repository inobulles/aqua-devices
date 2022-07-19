#!/bin/sh
cc "$@" -shared -fPIC -lxcb -lxcb-image -lxcb-shm -lxcb-icccm -lxcb-xfixes -lxcb-xinput
