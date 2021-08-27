static int aquabsd_vga_init(void) {
	fprintf(stderr, "[aquabsd.alps.vga] The 'aquabsd_vga' backend is not yet implemented.\n");
	return -1;
}

static int aquabsd_vga_get_mode_count(void) {
	return -1;
}

static video_mode_t* aquabsd_vga_get_modes(void) {
	return NULL;
}

static int aquabsd_vga_set_mode(video_mode_t* mode) {
	return -1;
}

static void* aquabsd_vga_get_framebuffer(void) {
	return NULL;
}
