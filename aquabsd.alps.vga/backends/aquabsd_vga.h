// TODO freeing (possibly a new function for "closing" a mode, i.e. resetting it to its previous state?)

#include <time.h>

#include <sys/mman.h>
#include <sys/fbio.h>
#include <sys/kbio.h>
#include <sys/consio.h>

#include <termios.h>

static int aquabsd_vga_mode_count;
static video_mode_t* aquabsd_vga_modes;

static video_mode_t* aquabsd_vga_mode;
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
	aquabsd_vga_mode = mode;

	video_info_t info = { .vi_mode = mode->id };
	ioctl(0, CONS_MODEINFO, &info);

	ioctl(0, CONS_GET, &aquabsd_vga_prev_mode);

	ioctl(0, KDENABIO, 0);
	ioctl(0, VT_WAITACTIVE, 0);

	ioctl(0, KDSETMODE, info.vi_flags & V_INFO_GRAPHICS ? KD_GRAPHICS : KD_TEXT);
	ioctl(0, _IO('V', info.vi_mode - M_VESA_BASE), 0); // TODO text modes

	video_adapter_info_t adapter_info = { 0 };
	ioctl(0, CONS_ADPINFO, &adapter_info);

	ioctl(0, CONS_SETWINORG, 0);

	aquabsd_vga_framebuffer = mmap(NULL, adapter_info.va_window_size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, 0, 0);

	if (aquabsd_vga_framebuffer == MAP_FAILED) {
		fprintf(stderr, "[aquabsd.alps.vga] ERROR Map failed\n");
		return -1;
	}

	aquabsd_vga_framebuffer_bytes = info.vi_width * info.vi_height * (info.vi_depth / 8);
	aquabsd_vga_doublebuffer = malloc(aquabsd_vga_framebuffer_bytes);

	return 0;
}

static void* aquabsd_vga_get_framebuffer(void) {
	return aquabsd_vga_doublebuffer;
}

static struct timespec aquabsd_vga_last_invalidation = { 0, 0 };

static int aquabsd_vga_flip(void) {
	// if more than '1.0 / aquabsd_vga_mode->fps' seconds have passed, invalidate
	// TODO see if it's feasible to unify this framerate-keeping code between backends?

	struct timespec now = { 0, 0 };
	clock_gettime(CLOCK_MONOTONIC, &now);

	float last_seconds = (float) aquabsd_vga_last_invalidation.tv_sec + 1.0e-9 * aquabsd_vga_last_invalidation.tv_nsec;
	float now_seconds = (float) now.tv_sec + 1.0e-9 * now.tv_nsec;

	if (now_seconds - last_seconds > 1.0 / aquabsd_vga_mode->fps) {
		clock_gettime(CLOCK_MONOTONIC, &aquabsd_vga_last_invalidation);
		memcpy(aquabsd_vga_framebuffer, aquabsd_vga_doublebuffer, aquabsd_vga_framebuffer_bytes);

		return 1; // flipped, redraw
	}

	return 0; // nothing happened
}

static int aquabsd_vga_kbd_mode = -1;
static struct termios aquabsd_vga_tty;

static int aquabsd_vga_reset(void) {
	// set the mode back to some usable mode
	
	ioctl(0, _IO('S', 24 /* hopefully mode 24 exists everywhere */), 0);
	ioctl(0, KDDISABIO, 0);

	struct vt_mode mode = { .mode = VT_AUTO };

	ioctl(0, KDSETMODE, KD_TEXT);
	ioctl(0, VT_SETMODE, &mode);

	// reset the previous keyboard mode & TTY state

	if (aquabsd_vga_kbd_mode < 0) {
		return 0;
	}

	ioctl(0, KDSKBMODE, aquabsd_vga_kbd_mode);
	tcsetattr(0, TCSANOW | TCSAFLUSH, &aquabsd_vga_tty);

	return 0;
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

	// recall the previous keyboard mode & TTY state in case any other devices modify it

	ioctl(0, KDGKBMODE, &aquabsd_vga_kbd_mode); // if this fails, we don't care, 'kbd_mode' will be '-1'
	tcgetattr(0, &aquabsd_vga_tty);

	// set backend functions from 'private.h'

	backend_get_mode_count = aquabsd_vga_get_mode_count;
	backend_get_modes = aquabsd_vga_get_modes;

	backend_set_mode = aquabsd_vga_set_mode;
	backend_get_framebuffer = aquabsd_vga_get_framebuffer;

	backend_flip = aquabsd_vga_flip;

	backend_reset = aquabsd_vga_reset;

	return 0;
}
