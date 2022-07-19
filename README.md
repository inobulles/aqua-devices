# aqua-devices

This repository contains the source code for all AQUA device sets and devices for a range of different platforms.
Each top-level directory corresponds to a set of devices (a "device set") which are meant to work together on a specific platform or configuration.
Here is a quick overview of each device set:

## Core devices

`core` is the device set in which core devices are gathered.
Core devices are devices which are meant to be common between all other device sets, and can practically be assumed to exist on any platform.
This is what each device does:

|Name|Description|
|-|-|
|`core.fs`|Handles interactions with the filesystem.|
|`core.math`|Provides complex mathematical functions.|
|`core.pkg`|Provides an interface to retrieve information about installed AQUA apps and read metadata from them.|
|`core.time`|Gives the time.|

## Devices for aquaBSD 1.0 Alps

`aquabsd.alps` is where all the devices needed for aquaBSD 1.0 Alps are gathered.
They are kept in a separate device set as many devices interdepend on eachother and some are platform-specific (though most should work on Linux too).
This is what each device does:

|Name|Description|
|-|-|
|`aquabsd.alps.font`|Provides mechanisms for loading fonts and rendering text (through [Pango](https://pango.gnome.org/)).|
|`aquabsd.alps.ftime`|Manages frametimes in a generic way for everything that needs to display stuff to the screen.|
|`aquabsd.alps.kbd`|Provides keyboard input.|
|`aquabsd.alps.mouse`|Provides mouse input, either through X11, either through the `sc`/`vt` virtual consoles on [aquaBSD core](https://github.com/inobulles/aquabsd-core).|
|`aquabsd.alps.net`|Provides an interface to recieve from/send to the network.|
|`aquabsd.alps.ogl`|Provides a way to create OpenGL contexts (with EGL), to load OpenGL API functions, and handles swapping with the help of `aquabsd.alps.ftime`.|
|`aquabsd.alps.png`|Provides mechanisms for loading PNG images and rendering them (through [libpng](http://www.libpng.org/pub/png/libpng.html)).|
|`aquabsd.alps.svg`|Provides mechanisms for loading SVG graphics and rendering them (through [librsvg](https://gitlab.gnome.org/GNOME/librsvg)).|
|`aquabsd.alps.ui`|Provides AQUA apps with a way to create consistent user interfaces. Still very experimental, so not included in this repository.|
|`aquabsd.alps.vga`|Provides mechanisms to output graphics to the `sc`/`vt` virtual consoles on [aquaBSD core](https://github.com/inobulles/aquabsd-core) through the `aquabsd_vga` backend. Can also emulate this with the `x11` backend.|
|`aquabsd.alps.win`|Creates X11 windows and provides callback hooks for window-related event processing. Also provides other devices with mechanisms for displaying stuff inside windows.|
|`aquabsd.alps.wm`|Creates X11 compositing window managers and provides mechanisms for managing windows. Also provides a window through `aquabsd.alps.win` which covers the entire screen (the "overlay" window).|
