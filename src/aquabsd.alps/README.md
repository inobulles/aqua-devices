### aquaBSD 1.0 Alps

`aquabsd.alps` can practically be considered obsolete at this point - all new devices will be in `aquabsd.black` moving forward.
This is what each device does:

|Name|Description|
|-|-|
|`aquabsd.alps.font`|Provides mechanisms for loading fonts and rendering text (through [Pango](https://pango.gnome.org/)).|
|`aquabsd.alps.ftime`|Manages frametimes in a generic way for everything that needs to display stuff to the screen.|
|`aquabsd.alps.kbd`|Provides keyboard input.|
|`aquabsd.alps.mouse`|Provides mouse input, either through X11, either through the `sc`/`vt` virtual consoles on [aquaBSD core](https://github.com/inobulles/aquabsd-core).|
|`aquabsd.alps.ogl`|Provides a way to create OpenGL contexts (with EGL) and load OpenGL API functions, and handles swapping with the help of `aquabsd.alps.ftime`.|
|`aquabsd.alps.png`|Provides mechanisms for loading PNG images and rendering them (through [libpng](http://www.libpng.org/pub/png/libpng.html)).|
|`aquabsd.alps.svg`|Provides mechanisms for loading SVG graphics and rendering them (through [librsvg](https://gitlab.gnome.org/GNOME/librsvg)).|
|`aquabsd.alps.ui`|Provides AQUA apps with a way to create consistent user interfaces. Still very experimental, so not included in this repository.|
|`aquabsd.alps.vga`|Provides mechanisms to output graphics to the `sc`/`vt` virtual consoles on [aquaBSD core](https://github.com/inobulles/aquabsd-core) through the `aquabsd_vga` backend. Can also emulate this with the `x11` backend.|
|`aquabsd.alps.vk`|Provides a way to create Vulkan contexts and load Vulkan API functions, and manages validation layers, extensions, and error handling. (Is kinda already obsoleted, was mostly developed as an experiment for use in [LLN '23](https://github.com/obiwac/lln-gamejam-2023).)|
|`aquabsd.alps.win`|Creates X11 windows and provides callback hooks for window-related event processing. Also provides other devices with mechanisms for displaying stuff inside windows.|
|`aquabsd.alps.wm`|Creates X11 compositing window managers and provides mechanisms for managing windows. Also provides a window through `aquabsd.alps.win` which covers the entire screen (the "overlay" window).|
