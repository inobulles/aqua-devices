#!/bin/sh
cc "$@" -shared -fPIC -lEGL -lxcb -lxcb-composite
