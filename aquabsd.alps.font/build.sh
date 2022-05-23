#!/bin/sh
cc -shared -fPIC $(pkg-config --cflags cairo pango) $(pkg-config --libs cairo pango pangocairo) "$@"
