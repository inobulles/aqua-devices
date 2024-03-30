# .wm

This device provides methods for managing windows and compositing them on screen.
This implementation does this through [Wayland](https://wayland.freedesktop.org/).

## wlroots

On FreeBSD, currently, you need the `x11-toolkits/wlroots-devel` port.

## Note on X11 support

If you need to use X11, use `aquabsd.alps.wm`.
The current implementation only supports Wayland, and will never support X11 (`aquabsd.black.win` will maybe support X11 windows in the future though).

## Notes

I'd (obviously) like for .wm to be the best compositor it can be, so here are a list of ideas I'd like to explore to improve latency and general perceived smoothness.
[This great article](https://raphlinus.github.io/ui/graphics/2020/09/13/compositor-is-evil.html) by Raph Levien highlights issues with compositors and provides some pistes to solve them.

### Repaint delays

Delay compositing a bit to allow for more inputs before the screen is flipped.
This is something I was already doing with `aquabsd.alps.ftime`.

It seems like [Weston](https://www.collabora.com/about-us/blog/2015/02/12/weston-repaint-scheduling/) and Sway already do this, but only with a fixed delay.
There is an [issue](https://github.com/swaywm/sway/issues/4734) which describes some techniques for dynamically changing this delay.

`aquabsd.alps.ftime` used a rather crude dynamic delay by taking a rolling average of the past few frametimes.
The main issues with this is that it was very resistant to change (especially tending downwards) and thus has a tendency to skip multiple frames.

### Partial redrawing

Something similar to Apple's [CALayers](https://developer.apple.com/documentation/quartzcore/calayer?language=objc) could be explored.
Check out [this blog post](https://mozillagfx.wordpress.com/2019/10/22/dramatically-reduced-power-usage-in-firefox-70-on-macos-with-core-animation/) from Mozilla on using Core Animation on macOS to improve battery life.

### Hardware overlays

Some overlays could skip the compositor entirely by using hardware planes.
`libliftoff` is a library which does this and helps wrangle all the differences between various hardware implementations of hardware planes.
Here's a [talk from FOSDEM](https://archive.fosdem.org/2020/schedule/event/kms_planes/) by Simon Ser, the creator of `libliftoff`, introducing it.

There is also a compositor which uses `libliftoff` also by Simon called [glider](https://github.com/emersion/glider).

This looks like a great way to increase battery life.
In [this experiment](https://octodon.social/@emersion/103300395120210509), power usage drops by over 50%.

### Opaque regions

Like with macOS, Wayland provides a way to define ["opaque" regions](https://wayland-book.com/surfaces-in-depth/surface-regions.html) to localize blurring (I had already thought of doing this through the DWD protocol in `aquabsd.alps.wm` back when I wrote that).
For now though, I think I'll stick to an effect similar to what I had in AQUA 2.X by sampling a blurred version of the wallpaper, and keeping proper blur strictly within the bounds of an application window.
