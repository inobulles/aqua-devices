// TODO freeing (possibly a new function for "closing" a mode, i.e. resetting it to its previous state?)

#include <sys/mman.h>
#include <sys/fbio.h>
#include <sys/kbio.h>
#include <sys/consio.h>

static int aquabsd_vga_mode_count;
static video_mode_t* aquabsd_vga_modes;

static int aquabsd_vga_prev_mode;

static size_t aquabsd_vga_framebuffer_bytes;

static void* aquabsd_vga_doublebuffer = (void*) 0;
static void* aquabsd_vga_framebuffer = (void*) 0;

static int aquabsd_vga_get_mode_count(void) {
	return aquabsd_vga_mode_count;
}

static video_mode_t* aquabsd_vga_get_modes(void) {
	size_t bytes = aquabsd_vga_mode_count * sizeof(video_mode_t);

	video_mode_t* modes = malloc(bytes);
	memcpy(modes, aquabsd_vga_modes, bytes);

	return modes;
}

static int aquabsd_vga_set_mode(video_mode_t* mode) {
	video_info_t info = { .vi_mode = mode->id };
	ioctl(0, CONS_MODEINFO, &info);

	ioctl(0, CONS_GET, &aquabsd_vga_prev_mode);

	ioctl(0, KDENABIO, 0);
	ioctl(0, VT_WAITACTIVE, 0);

	ioctl(0, KDSETMODE, info.vi_flags & V_INFO_GRAPHICS ? KD_GRAPHICS : KD_TEXT);
	ioctl(0, info.vi_mode, 0);

	video_adapter_info_t adapter_info = { 0 };
	ioctl(0, CONS_ADPINFO, &adapter_info);

	ioctl(0, CONS_SETWINORG, 0);

	aquabsd_vga_framebuffer = mmap((void*) 0, adapter_info.va_window_size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, 0, 0);

	if (aquabsd_vga_framebuffer == MAP_FAILED) {
		return -1;
	}

	aquabsd_vga_framebuffer_bytes = info.vi_width * info.vi_height * (info.vi_depth / 8);
	aquabsd_vga_doublebuffer = malloc(aquabsd_vga_framebuffer_bytes);

	return 0;
}

static void* aquabsd_vga_get_framebuffer(void) {
	return aquabsd_vga_doublebuffer;
}

static int aquabsd_vga_flip(void) {
	memcpy(aquabsd_vga_framebuffer, aquabsd_vga_doublebuffer, aquabsd_vga_framebuffer_bytes);
	return 1; // flipped, redraw
}

static int aquabsd_vga_init(void) {
	// get modes ('show_mode_info' function in 'usr.sbin/vidcontrol/vidcontrol.c' for reference)

	aquabsd_vga_modes = (video_mode_t*) 0;

	for (int i = 0; i <= M_VESA_MODE_MAX; i++) {
		video_info_t info = { .vi_mode = i };

		if (ioctl(0, CONS_MODEINFO, &info) || \
			info.vi_mode != i || \
			!(info.vi_width || info.vi_height || info.vi_cwidth || info.vi_cheight)) {
			continue;
		}

		aquabsd_vga_modes = realloc(aquabsd_vga_modes, (aquabsd_vga_mode_count + 1) * sizeof(*aquabsd_vga_modes));
		video_mode_t* mode = &aquabsd_vga_modes[aquabsd_vga_mode_count++];

		mode->id = info.vi_mode; // should be == i

		mode->text = !(info.vi_flags & V_INFO_GRAPHICS);

		mode->width  = info.vi_width;
		mode->height = info.vi_height;

		mode->bpp = info.vi_depth;
		mode->fps = 60;
	}

	if (!aquabsd_vga_modes) {
		return -1;
	}

	// set backend functions from 'private.h'

	backend_get_mode_count = aquabsd_vga_get_mode_count;
	backend_get_modes = aquabsd_vga_get_modes;

	backend_set_mode = aquabsd_vga_set_mode;
	backend_get_framebuffer = aquabsd_vga_get_framebuffer;

	backend_flip = aquabsd_vga_flip;

	return 0;
}
