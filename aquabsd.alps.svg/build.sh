#!/bin/sh
cc "$@" -shared -fPIC $(pkg-config --cflags librsvg-2.0) $(pkg-config --libs librsvg-2.0) # -DAQUABSD_ALPS_SVG_NEW_RSVG
