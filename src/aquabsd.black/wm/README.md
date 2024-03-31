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

## Findings w.r.t. how to get textures working and whatnot

Let's cave and purely use the OpenGL backend for WebGPU for now.
I already know how textures work with EGL and if anything goes wrong with Vulkan well it's gonna be a lot more work.

Probably a WebGPU extension for this is needed and doesn't exist yet?
Making this play nicely with non-specific shader code is going to be a nightmare 🙂

The two reference functions of interest in wlroots are `gles2_texture_from_buffer` and `vulkan_texture_from_buffer`, which both quite simply return a `struct wlr_texture` to be used elsewhere in the renderer.
`struct wlr_texture` looks like this:

```c
struct wlr_texture {
	const struct wlr_texture_impl *impl;
	uint32_t width, height;

	struct wlr_renderer *renderer;
};
```

The `impl` member contains the following struct which defines the functions we can call on this implementation of `wlr_texture`.
In the case of the GLES2 renderer:

```c
static const struct wlr_texture_impl texture_impl = {
	.update_from_buffer = gles2_texture_update_from_buffer,
	.read_pixels = gles2_texture_read_pixels,
	.preferred_read_format = gles2_texture_preferred_read_format,
	.destroy = handle_gles2_texture_destroy,
};
```

And for Vulkan:

```c
static const struct wlr_texture_impl texture_impl = {
	.update_from_buffer = vulkan_texture_update_from_buffer,
	.read_pixels = vulkan_texture_read_pixels,
	.preferred_read_format = vulkan_texture_preferred_read_format,
	.destroy = vulkan_texture_unref,
};
```

These functions then access custom data for each texture type by getting the container of them with `wl_container_of`.
For GLES2 this container is `struct wlr_gles2_texture`:

```c
struct wlr_gles2_texture {
	struct wlr_texture wlr_texture;
	struct wlr_gles2_renderer *renderer;
	struct wl_list link; // wlr_gles2_renderer.textures

	GLenum target;

	// If this texture is imported from a buffer, the texture is does not own
	// these states. These cannot be destroyed along with the texture in this
	// case.
	GLuint tex;
	GLuint fbo;

	bool has_alpha;

	uint32_t drm_format; // for mutable textures only, used to interpret upload data
	struct wlr_gles2_buffer *buffer; // for DMA-BUF imports only
};
```

And for Vulkan it is `struct wlr_vk_texture`:

```c
// State (e.g. image texture) associated with a surface.
struct wlr_vk_texture {
	struct wlr_texture wlr_texture;
	struct wlr_vk_renderer *renderer;
	uint32_t mem_count;
	VkDeviceMemory memories[WLR_DMABUF_MAX_PLANES];
	VkImage image;
	const struct wlr_vk_format *format;
	enum wlr_vk_texture_transform transform;
	struct wlr_vk_command_buffer *last_used_cb; // to track when it can be destroyed
	bool dmabuf_imported;
	bool owned; // if dmabuf_imported: whether we have ownership of the image
	bool transitioned; // if dma_imported: whether we transitioned it away from preinit
	bool has_alpha; // whether the image is has alpha channel
	bool using_mutable_srgb; // is this accessed through _SRGB format view
	struct wl_list foreign_link; // wlr_vk_renderer.foreign_textures
	struct wl_list destroy_link; // wlr_vk_command_buffer.destroy_textures
	struct wl_list link; // wlr_vk_renderer.textures

	// If imported from a wlr_buffer
	struct wlr_buffer *buffer;
	struct wlr_addon buffer_addon;
	// For DMA-BUF implicit sync interop
	VkSemaphore foreign_semaphores[WLR_DMABUF_MAX_PLANES];

	struct wl_list views; // struct wlr_vk_texture_ds.link
};
```

(Maybe now it's clear why I'm starting by forcing the OpenGL backend to WebGPU 🙂)

For GLES2, once we have our `struct wlr_gles2_texture`, we can just get its `tex` member and do as in `aquabsd.alps.ogl` with the `GL_OES_EGL_image_external` extension.

Now "all" that's left to do is design the WebGPU extension 🥲
