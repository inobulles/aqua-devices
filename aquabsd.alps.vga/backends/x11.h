// heavily inspired by the accepted answer on this SO question: https://stackoverflow.com/questions/65251904/draw-pixels-in-a-linux-window-without-opengl-in-c
// also, as linked in the SO answer, this link helped a lot in migrating from Xlib to XCB: https://xcb.freedesktop.org/tutorial/basicwindowsanddrawing/

#include <aquabsd.alps.mouse/public.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <xcb/xcb.h>
#include <xcb/shm.h>
#include <xcb/xfixes.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_event.h>

#if __ORDER_LITTLE_ENDIAN == __BYTE_ORDER__
	#define NATIVE_XCB_IMAGE_ORDER XCB_IMAGE_ORDER_LSB_FIRST
#else
	#define NATIVE_XCB_IMAGE_ORDER XCB_IMAGE_ORDER_MSB_FIRST
#endif

// the only mode this backend will report is a standard 800x600x32@60 graphics mode for compatibility, but any mode passed to 'backend_set_mode' will work (within reason)

typedef struct {
	xcb_image_t* image;

	xcb_shm_seg_t shm_seg;
	int shm_id;
} x11_image_t;

static xcb_connection_t* x11_connection = NULL;
static xcb_screen_t* x11_screen = NULL;

static xcb_drawable_t x11_window = (xcb_drawable_t) 0; 
static xcb_gcontext_t x11_context = (xcb_gcontext_t) 0;

static xcb_atom_t x11_wm_delete_window_atom;

static video_mode_t x11_mode;
static x11_image_t x11_image;

static struct timespec x11_last_exposure = { 0, 0 };

static unsigned x11_mouse_buttons[AQUABSD_ALPS_MOUSE_BUTTON_COUNT];
static float x11_mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_COUNT];

static int x11_mouse_x;
static int x11_mouse_y;

static int x11_get_mode_count(void) {
	return 1;
}

static video_mode_t* x11_get_modes(void) {
	video_mode_t* modes = malloc(sizeof *modes);
	
	// 800x600x32@60
	
	video_mode_t* mode = &modes[0];
	memset(mode, 0, sizeof(*mode));

	mode->text = 0;

	mode->width  = 800;
	mode->height = 600;

	mode->bpp = 32;
	mode->fps = 60;

	return modes;
}

static void x11_close_window(void) {
	// destroy image
	
	xcb_shm_detach(x11_connection, x11_image.shm_seg);
	shmdt(x11_image.image->data);
	shmctl(x11_image.shm_id, IPC_RMID, 0);
	xcb_image_destroy(x11_image.image);
	
	// TODO close window
}

// TODO change all asserts to proper errors (returning '-1' from 'x11_set_mode')

static int x11_set_mode(video_mode_t* mode) {
	// check to see if the mode actually makes sense
	
	if (mode->text ||
		mode->bpp != 32 ||
		mode->width < 1 ||
		mode->height < 1) {
		return -1;
	}

	memcpy(&x11_mode, mode, sizeof(x11_mode));

	// make sure we destroy any previous windows
	
	if (x11_window) {
		x11_close_window();
	}
	
	// actually create the window with the specified mode
	
	x11_window = xcb_generate_id(x11_connection);
	xcb_create_window(x11_connection, XCB_COPY_FROM_PARENT, x11_window, x11_screen->root, 0, 0, mode->width, mode->height, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT, x11_screen->root_visual, XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK, (const uint32_t[]) { x11_screen->black_pixel, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_POINTER_MOTION });

	// hide cursor

	xcb_xfixes_query_version(x11_connection, 4, 0); // see: https://stackoverflow.com/questions/57841785/how-to-hide-cursor-in-xcb
	xcb_xfixes_hide_cursor(x11_connection, x11_screen->root);

	// add caption to window

	char caption[256] = { 0 };
	sprintf(caption, "[aquabsd.alps.vga] %ldx%ldx%ld@%ld", mode->width, mode->height, mode->bpp, mode->fps);
	
	xcb_change_property(x11_connection, XCB_PROP_MODE_REPLACE, x11_window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(caption), caption);

	// make it so that the window isn't resizable by user

	xcb_size_hints_t hints;

	xcb_icccm_size_hints_set_min_size(&hints, mode->width, mode->height);
	xcb_icccm_size_hints_set_max_size(&hints, mode->width, mode->height);

	xcb_icccm_set_wm_size_hints(x11_connection, x11_window, XCB_ATOM_WM_NORMAL_HINTS, &hints);

	// create graphics context

	x11_context = xcb_generate_id(x11_connection);
	xcb_create_gc(x11_connection, x11_context, x11_window, XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES, (const uint32_t[]) { x11_screen->black_pixel, 0 });

	memset(&x11_image, 0, sizeof(x11_image));

	// get appropriate format for the mode's depth

	xcb_format_t* format = NULL;
	const xcb_setup_t* setup = xcb_get_setup(x11_connection);

	for (xcb_format_iterator_t it = xcb_setup_pixmap_formats_iterator(setup); it.rem; xcb_format_next(&it)) {
		format = it.data;

		if (format->bits_per_pixel /* NOT 'format->depth' */ == mode->bpp) {
			break;
		}
	}

	assert(format);

	x11_image.image = xcb_image_create(mode->width, mode->height, XCB_IMAGE_FORMAT_Z_PIXMAP, format->scanline_pad, format->depth, format->bits_per_pixel, 0, NATIVE_XCB_IMAGE_ORDER, XCB_IMAGE_ORDER_MSB_FIRST, NULL, ~0, 0);
	assert(x11_image.image);

	x11_image.shm_id = shmget(IPC_PRIVATE, x11_image.image->stride * x11_image.image->height, IPC_CREAT | 0600);
	assert(x11_image.shm_id >= 0);

	x11_image.image->data = shmat(x11_image.shm_id, 0, 0);
	assert((int64_t) x11_image.image->data >= 0);

	x11_image.shm_seg = xcb_generate_id(x11_connection);
	xcb_generic_error_t* error = xcb_request_check(x11_connection, xcb_shm_attach_checked(x11_connection, x11_image.shm_seg, x11_image.shm_id, 0));
	assert(!error);

	memset(x11_image.image->data, 0, mode->width * mode->height * (mode->bpp / 8));
	xcb_map_window(x11_connection, x11_window);

	xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(x11_connection, 1, 12 /* strlen("WM_PROTOCOLS") */, "WM_PROTOCOLS");
	xcb_atom_t wm_protocols_atom = xcb_intern_atom_reply(x11_connection, wm_protocols_cookie, 0)->atom;

	xcb_intern_atom_cookie_t wm_delete_window_cookie = xcb_intern_atom(x11_connection, 0, 16 /* strlen("WM_DELETE_WINDOW") */, "WM_DELETE_WINDOW");
	x11_wm_delete_window_atom = xcb_intern_atom_reply(x11_connection, wm_delete_window_cookie, 0)->atom;

	xcb_icccm_set_wm_protocols(x11_connection, x11_window, wm_protocols_atom, 1, &x11_wm_delete_window_atom);

	xcb_flush(x11_connection);

	// assume the user program sets the cursor to the middle of the screen by default

	x11_mouse_x = mode->width  / 2;
	x11_mouse_y = mode->height / 2;

	return 0;
}

static void* x11_get_framebuffer(void) {
	return (void*) x11_image.image->data;
}

static unsigned x11_invalidated = 1;

static int x11_flip(void) {
	// if more than '1.0 / x11_mode.fps' seconds have passed, invalidate

	struct timespec now = { 0, 0 };
	clock_gettime(CLOCK_MONOTONIC, &now);

	float last_seconds = (float) x11_last_exposure.tv_sec + 1.0e-9 * x11_last_exposure.tv_nsec;
	float now_seconds = (float) now.tv_sec + 1.0e-9 * now.tv_nsec;

	if (now_seconds - last_seconds > 1.0 / x11_mode.fps && !x11_invalidated) {
		xcb_expose_event_t invalidate_event;

		invalidate_event.window = x11_window;
		invalidate_event.response_type = XCB_EXPOSE;

		invalidate_event.x = 0;
		invalidate_event.y = 0;

		invalidate_event.width  = x11_mode.width;
		invalidate_event.height = x11_mode.height;

		xcb_send_event(x11_connection, 0, x11_window, XCB_EVENT_MASK_EXPOSURE, (const char*) &invalidate_event);
		xcb_flush(x11_connection);

		x11_invalidated = 1;
	}

	// process events

	int return_value = 0; // nothing happened

	for (xcb_generic_event_t* event; (event = xcb_poll_for_event(x11_connection)); free(event)) {
		int event_type = event->response_type & XCB_EVENT_RESPONSE_TYPE_MASK;

		// general events

		if (event_type == XCB_EXPOSE) {
			xcb_shm_put_image(x11_connection, x11_window, x11_context, x11_image.image->width, x11_image.image->height, 0, 0, x11_image.image->width, x11_image.image->height, 0, 0, x11_image.image->depth, x11_image.image->format, 0, x11_image.shm_seg, 0);
			xcb_flush(x11_connection);

			x11_invalidated = 0;
			clock_gettime(CLOCK_MONOTONIC, &x11_last_exposure);

			return_value = 1; // flipped, redraw
		}

		else if (event_type == XCB_CLIENT_MESSAGE) {
			xcb_client_message_event_t* client_message_event = (xcb_client_message_event_t*) event;

			if (client_message_event->data.data32[0] == x11_wm_delete_window_atom) {
				return_value = -1; // quit
			}
		}

		// mouse button press/release events

		else if (event_type == XCB_BUTTON_PRESS) {
			xcb_button_press_event_t* button_press_event = (xcb_button_press_event_t*) event;
			xcb_button_t button = button_press_event->detail;

			if (button == 1) x11_mouse_buttons[AQUABSD_ALPS_MOUSE_BUTTON_LEFT  ] = 1;
			if (button == 3) x11_mouse_buttons[AQUABSD_ALPS_MOUSE_BUTTON_RIGHT ] = 1;
			if (button == 2) x11_mouse_buttons[AQUABSD_ALPS_MOUSE_BUTTON_MIDDLE] = 1;

			if (button == 5) x11_mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_Z] = -1.0;
			if (button == 4) x11_mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_Z] =  1.0;
		}

		else if (event_type == XCB_BUTTON_RELEASE) {
			xcb_button_release_event_t* button_release_event = (xcb_button_release_event_t*) event;
			xcb_button_t button = button_release_event->detail;

			if (button == 1) x11_mouse_buttons[AQUABSD_ALPS_MOUSE_BUTTON_LEFT  ] = 0;
			if (button == 3) x11_mouse_buttons[AQUABSD_ALPS_MOUSE_BUTTON_RIGHT ] = 0;
			if (button == 2) x11_mouse_buttons[AQUABSD_ALPS_MOUSE_BUTTON_MIDDLE] = 0;
		}

		// mouse motion events

		else if (event_type == XCB_ENTER_NOTIFY) {
			xcb_enter_notify_event_t* enter_notify_event = (xcb_enter_notify_event_t*) event;

			x11_mouse_x = enter_notify_event->event_x;
			x11_mouse_y = enter_notify_event->event_y;
		}

		else if (event_type == XCB_LEAVE_NOTIFY) {
			xcb_leave_notify_event_t* leave_notify_event = (xcb_leave_notify_event_t*) event;
			
			x11_mouse_x = leave_notify_event->event_x;
			x11_mouse_y = leave_notify_event->event_y;
		}

		else if (event_type == XCB_MOTION_NOTIFY) {
			xcb_motion_notify_event_t* motion_notify_event = (xcb_motion_notify_event_t*) event;
			
			x11_mouse_x = motion_notify_event->event_x;
			x11_mouse_y = motion_notify_event->event_y;
		}
	}

	return return_value;
}

static int x11_mouse_update_callback(aquabsd_alps_mouse_t* mouse) {
	x11_mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_X] =       (float) x11_mouse_x / x11_mode.width;
	x11_mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_Y] = 1.0 - (float) x11_mouse_y / x11_mode.height;

	memcpy(mouse->buttons, x11_mouse_buttons, sizeof mouse->buttons);
	memcpy(mouse->axes, x11_mouse_axes, sizeof mouse->axes);

	memset(x11_mouse_axes, 0, sizeof x11_mouse_axes);

	return 0;
}

static int x11_init(void) {
	x11_connection = xcb_connect(NULL /* default to the 'DISPLAY' environment variable */, NULL);

	if (!x11_connection) {
		return -1;
	}

	const xcb_query_extension_reply_t* shm_extension = xcb_get_extension_data(x11_connection, &xcb_shm_id);

	if (!shm_extension || !shm_extension->present) {
		return -1;
	}

	// set backend functions from 'private.h'
	
	backend_get_mode_count = x11_get_mode_count;
	backend_get_modes = x11_get_modes;

	backend_set_mode = x11_set_mode;
	backend_get_framebuffer = x11_get_framebuffer;

	backend_flip = x11_flip;

	// finish init process
	
	x11_screen = xcb_setup_roots_iterator(xcb_get_setup(x11_connection)).data;

	// register new mouse

	memset(x11_mouse_buttons, 0, sizeof x11_mouse_buttons);
	memset(x11_mouse_axes, 0, sizeof x11_mouse_axes);

	uint64_t mouse_device = kos_query_device(0, (uint64_t) "aquabsd.alps.mouse");

	if (mouse_device != -1) {
		aquabsd_alps_mouse_register_mouse = kos_load_device_function(mouse_device, "register_mouse");
		aquabsd_alps_mouse_register_mouse("aquabsd.alps.vga X11 backend mouse", x11_mouse_update_callback, 1);
	}
	
	return 0;
}
