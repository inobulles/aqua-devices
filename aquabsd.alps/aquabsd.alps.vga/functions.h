static int initialized = 0;

dynamic int get_mode_count(void) {
	// initialize the backend if not already done
	
	if (!initialized) {
		initialized = 1;

	#if !defined(WITHOUT_AQUABSD_VGA)
		if (!aquabsd_vga_init()) {
			goto initialized;
		}
	#endif

	#if !defined(WITHOUT_X11)
		if (!x11_init()) {
			goto initialized;
		}
	#endif

		return -1;
	}

initialized:

	// actually get mode count
	
	return backend_get_mode_count();
}

dynamic video_mode_t* get_modes(void) {
	return backend_get_modes();
}

dynamic int set_mode(video_mode_t* mode) {
	return backend_set_mode(mode);
}

dynamic void* get_framebuffer(void) {
	return backend_get_framebuffer();
}

dynamic int flip(void) {
	return backend_flip();
}

dynamic int reset(void) {
	return backend_reset();
}